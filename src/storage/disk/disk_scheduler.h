#pragma once

#include <cstddef>
#include <future>

#include "common/config.h"
#include "common/macros.h"
#include "storage/disk/channel.h"
#include "storage/disk/disk_manager.h"

namespace minitub {

/** One page read or write. `data` (PAGE_SIZE bytes) must stay valid until `callback` is set. */
struct DiskRequest {
  bool is_write{false};
  std::byte *data{nullptr};
  page_id_t page_id{INVALID_PAGE_ID};
  std::promise<bool> callback;  // true = done, false = the DiskManager threw
};

/**
 * Runs disk requests on background worker threads.
 * Requests for the same page complete in scheduling order, for any number of workers.
 * Rules: design.md section 7.2.
 */
class DiskScheduler {
 public:
  explicit DiskScheduler(DiskManager *disk_manager, int num_workers = 1);
  /** Finishes every request already scheduled, then stops and joins the workers. */
  ~DiskScheduler();
  DISALLOW_COPY_AND_MOVE(DiskScheduler);

  /** Queues `request` and returns immediately. */
  void Schedule(DiskRequest request);

 private:
  // TODO(M1): your members (disk manager, one Channel<std::optional<DiskRequest>> per worker, threads, ...).
};

}  // namespace minitub
