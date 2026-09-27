#include "storage/disk/disk_scheduler.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "storage/disk/channel.h"
#include "storage/disk/disk_manager.h"
#include "test_util.h"

namespace minitub {

using test::PageBuf;
using test::TempFile;
using namespace std::chrono_literals;

namespace {

/** Builds a request and hands back the future that its callback completes. */
auto MakeRequest(bool is_write, std::byte *data, page_id_t page_id) -> std::pair<DiskRequest, std::future<bool>> {
  DiskRequest request;
  request.is_write = is_write;
  request.data = data;
  request.page_id = page_id;
  std::future<bool> done = request.callback.get_future();
  return {std::move(request), std::move(done)};
}

}  // namespace

// ---------------- Channel ----------------

// checks: items come out in the order they were put in (FIFO).
TEST(ChannelTest, FifoOrder) {
  Channel<int> ch;
  for (int i = 0; i < 100; i++) {
    ch.Put(i);
  }
  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(ch.Get(), i);
  }
}

// checks: Get() blocks while the channel is empty and wakes up when an item arrives.
TEST(ChannelTest, GetBlocksUntilPut) {
  Channel<int> ch;
  std::atomic<bool> got{false};
  int value = 0;
  std::thread consumer([&] {
    value = ch.Get();
    got = true;
  });
  std::this_thread::sleep_for(50ms);
  EXPECT_FALSE(got.load()) << "Get() returned before anything was Put()";
  ch.Put(42);
  consumer.join();
  EXPECT_TRUE(got.load());
  EXPECT_EQ(value, 42);
}

// checks: with 4 producers and 4 consumers, every item is delivered exactly once.
TEST(ChannelTest, ManyProducersAndConsumersDeliverEachItemOnce) {
  constexpr int kProducers = 4;
  constexpr int kConsumers = 4;
  constexpr int kItemsPerProducer = 1000;
  Channel<int> ch;

  std::vector<std::vector<int>> received(kConsumers);
  std::vector<std::thread> consumers;
  for (int c = 0; c < kConsumers; c++) {
    consumers.emplace_back([&, c] {
      for (int v = ch.Get(); v != -1; v = ch.Get()) {  // -1 = stop
        received[c].push_back(v);
      }
    });
  }
  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; p++) {
    producers.emplace_back([&, p] {
      for (int i = 0; i < kItemsPerProducer; i++) {
        ch.Put(p * kItemsPerProducer + i);
      }
    });
  }
  for (auto &t : producers) {
    t.join();
  }
  for (int c = 0; c < kConsumers; c++) {
    ch.Put(-1);
  }
  for (auto &t : consumers) {
    t.join();
  }

  std::vector<int> all;
  for (const auto &r : received) {
    all.insert(all.end(), r.begin(), r.end());
  }
  std::sort(all.begin(), all.end());
  std::vector<int> expected(kProducers * kItemsPerProducer);
  std::iota(expected.begin(), expected.end(), 0);
  EXPECT_EQ(all, expected);
}

// ---------------- DiskScheduler (run with 1 and with 4 workers) ----------------

class DiskSchedulerTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override { dm_ = std::make_unique<DiskManager>(file_.Path()); }

  auto Workers() const -> int { return GetParam(); }

  TempFile file_;
  std::unique_ptr<DiskManager> dm_;
};

// checks: a read scheduled right after a write to the same page (without waiting)
// sees the written data: per-page ordering holds.
TEST_P(DiskSchedulerTest, ReadAfterWriteOnSamePageSeesWrite) {
  dm_->AllocatePage();
  DiskScheduler scheduler(dm_.get(), Workers());
  PageBuf in;
  PageBuf out;
  test::Fill(in.Span(), 3);

  auto [write, write_done] = MakeRequest(true, in.Data(), 0);
  auto [read, read_done] = MakeRequest(false, out.Data(), 0);
  scheduler.Schedule(std::move(write));
  scheduler.Schedule(std::move(read));

  EXPECT_TRUE(write_done.get());
  EXPECT_TRUE(read_done.get());
  EXPECT_EQ(in.bytes, out.bytes);
}

// checks: 100 back-to-back writes to one page are applied in order, so the last one wins,
// even when several workers could run them in parallel.
TEST_P(DiskSchedulerTest, ManyWritesToOnePageKeepOrder) {
  constexpr int kWrites = 100;
  dm_->AllocatePage();
  std::vector<PageBuf> bufs(kWrites);
  std::vector<std::future<bool>> done;
  {
    DiskScheduler scheduler(dm_.get(), Workers());
    for (int i = 0; i < kWrites; i++) {
      test::Fill(bufs[i].Span(), static_cast<std::uint32_t>(i));
      auto [req, fut] = MakeRequest(true, bufs[i].Data(), 0);
      scheduler.Schedule(std::move(req));
      done.push_back(std::move(fut));
    }
    for (auto &f : done) {
      EXPECT_TRUE(f.get());
    }
  }
  PageBuf out;
  dm_->ReadPage(0, out.Span());
  EXPECT_EQ(out.bytes, bufs[kWrites - 1].bytes);
}

// checks: 8 threads scheduling requests at once all get correct results (thread safety).
TEST_P(DiskSchedulerTest, ConcurrentRequestsFromManyThreads) {
  constexpr int kThreads = 8;
  constexpr int kPagesPerThread = 50;
  for (int i = 0; i < kThreads * kPagesPerThread; i++) {
    dm_->AllocatePage();
  }
  DiskScheduler scheduler(dm_.get(), Workers());
  std::atomic<int> failures{0};

  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; t++) {
    threads.emplace_back([&, t] {
      std::vector<PageBuf> in(kPagesPerThread);
      std::vector<PageBuf> out(kPagesPerThread);
      std::vector<std::future<bool>> done;
      for (int i = 0; i < kPagesPerThread; i++) {
        page_id_t id = t * kPagesPerThread + i;
        test::Fill(in[i].Span(), static_cast<std::uint32_t>(id));
        auto [w, wf] = MakeRequest(true, in[i].Data(), id);
        auto [r, rf] = MakeRequest(false, out[i].Data(), id);
        scheduler.Schedule(std::move(w));
        scheduler.Schedule(std::move(r));
        done.push_back(std::move(wf));
        done.push_back(std::move(rf));
      }
      for (auto &f : done) {
        failures += f.get() ? 0 : 1;
      }
      for (int i = 0; i < kPagesPerThread; i++) {
        failures += in[i].bytes == out[i].bytes ? 0 : 1;
      }
    });
  }
  for (auto &th : threads) {
    th.join();
  }
  EXPECT_EQ(failures.load(), 0);
}

// checks: when the DiskManager throws (page never allocated), the callback reports false
// instead of the worker thread dying.
TEST_P(DiskSchedulerTest, FailedRequestReportsFalse) {
  DiskScheduler scheduler(dm_.get(), Workers());
  PageBuf buf;
  auto [req, done] = MakeRequest(false, buf.Data(), 999);
  scheduler.Schedule(std::move(req));
  ASSERT_EQ(done.wait_for(10s), std::future_status::ready);
  EXPECT_FALSE(done.get());
}

// checks: destroying the scheduler finishes every request already scheduled
// (clean shutdown: nothing lost, no hang).
TEST_P(DiskSchedulerTest, DestructorFinishesPendingRequests) {
  constexpr int kPages = 200;
  for (int i = 0; i < kPages; i++) {
    dm_->AllocatePage();
  }
  std::vector<PageBuf> bufs(kPages);
  std::vector<std::future<bool>> done;
  {
    DiskScheduler scheduler(dm_.get(), Workers());
    for (int i = 0; i < kPages; i++) {
      test::Fill(bufs[i].Span(), static_cast<std::uint32_t>(i + 1));
      auto [req, fut] = MakeRequest(true, bufs[i].Data(), i);
      scheduler.Schedule(std::move(req));
      done.push_back(std::move(fut));
    }
  }  // destructor runs with requests still queued

  for (int i = 0; i < kPages; i++) {
    ASSERT_EQ(done[i].wait_for(0s), std::future_status::ready) << "request " << i << " was dropped";
    EXPECT_TRUE(done[i].get());
    PageBuf out;
    dm_->ReadPage(i, out.Span());
    EXPECT_EQ(out.bytes, bufs[i].bytes);
  }
}

// checks: creating and destroying an idle scheduler repeatedly never hangs
// (workers blocked in Get() must still receive the stop signal).
TEST_P(DiskSchedulerTest, IdleSchedulerShutsDownCleanly) {
  for (int i = 0; i < 20; i++) {
    DiskScheduler scheduler(dm_.get(), Workers());
  }
  SUCCEED();
}

INSTANTIATE_TEST_SUITE_P(Workers, DiskSchedulerTest, ::testing::Values(1, 4));

}  // namespace minitub
