#include "storage/disk/disk_scheduler.h"

#include <cstdint>
#include <span>
#include <string>
#include <utility>

#include "common/exception.h"

namespace minitub {

DiskScheduler::DiskScheduler(DiskManager *disk_manager, int num_workers) : disk_manager_(disk_manager) {
  if (disk_manager_ == nullptr) {
    throw Exception(ExceptionType::Invalid, "DiskScheduler needs a DiskManager");
  }
  if (num_workers < 1) {
    throw Exception(ExceptionType::Invalid, "DiskScheduler needs >= 1 worker, got " + std::to_string(num_workers));
  }
  const auto n = static_cast<std::size_t>(num_workers);

  // Every queue must exist before any worker starts, because a worker reads queues_[i].
  queues_.reserve(n);
  for (std::size_t i = 0; i < n; i++) {
    queues_.push_back(std::make_unique<Channel<std::optional<DiskRequest>>>());
  }

  workers_.reserve(n);
  try {
    for (std::size_t i = 0; i < n; i++) {
      workers_.emplace_back([this, i] { RunWorker(i); });
    }
  } catch (...) {
    // Starting a thread can fail (std::system_error). A throwing constructor never runs the
    // destructor, and destroying a still-joinable std::thread calls std::terminate, so stop
    // and join the workers that did start before passing the error on.
    for (std::size_t i = 0; i < workers_.size(); i++) {
      queues_[i]->Put(std::nullopt);
    }
    for (auto &worker : workers_) {
      worker.join();
    }
    throw;
  }
}

DiskScheduler::~DiskScheduler() {
  // The stop signal goes to the BACK of each queue, so every request scheduled before it
  // is still executed: nothing already scheduled is lost.
  for (auto &queue : queues_) {
    queue->Put(std::nullopt);
  }
  for (auto &worker : workers_) {
    worker.join();  // wait until the worker has drained its queue and returned
  }
}

void DiskScheduler::Schedule(DiskRequest request) {
  // Same page -> same worker -> same FIFO queue, so requests for one page keep their order.
  // Go through uint32_t first: page_id is signed and % of a negative number is negative.
  const auto page = static_cast<std::uint32_t>(request.page_id);
  queues_[page % queues_.size()]->Put(std::move(request));
}

void DiskScheduler::RunWorker(std::size_t worker) {
  Channel<std::optional<DiskRequest>> &queue = *queues_[worker];
  while (true) {
    std::optional<DiskRequest> request = queue.Get();  // sleeps until something arrives
    if (!request.has_value()) {
      return;  // stop signal
    }

    bool ok = false;
    // An exception escaping a thread's function calls std::terminate and kills the whole
    // process, so every failure is caught and reported through the callback instead.
    try {
      if (request->data == nullptr) {
        throw Exception(ExceptionType::Invalid, "DiskRequest without a buffer");
      }
      if (request->is_write) {
        disk_manager_->WritePage(request->page_id, std::span<const std::byte, PAGE_SIZE>(request->data, PAGE_SIZE));
      } else {
        disk_manager_->ReadPage(request->page_id, std::span<std::byte, PAGE_SIZE>(request->data, PAGE_SIZE));
      }
      ok = true;
    } catch (...) {
      ok = false;  // e.g. OutOfRange (page never allocated) or Io (disk full)
    }
    // Wakes up whoever waits on the matching std::future.
    request->callback.set_value(ok);
  }
}

}  // namespace minitub
