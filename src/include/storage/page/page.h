#pragma once

#include <array>
#include <cstddef>
#include <shared_mutex>

#include "common/config.h"
#include "common/macros.h"

namespace minitub {

class BufferPoolManager;

/**
 * One frame of the buffer pool: the page bytes plus metadata.
 * Metadata is changed only by BufferPoolManager (under its latch).
 * The data is aligned to PAGE_SIZE so it can be used with O_DIRECT.
 */
class Page {
 public:
  Page() = default;
  DISALLOW_COPY_AND_MOVE(Page);

  auto GetData() -> std::byte * { return data_.data(); }
  auto GetData() const -> const std::byte * { return data_.data(); }
  auto GetPageId() const -> page_id_t { return page_id_; }
  auto GetPinCount() const -> int { return pin_count_; }
  auto IsDirty() const -> bool { return is_dirty_; }

  /** LSN of the last log record that changed this page. Unused until M9 (WAL). */
  auto GetLsn() const -> lsn_t { return page_lsn_; }
  void SetLsn(lsn_t lsn) { page_lsn_ = lsn; }

  // Page latch: protects the page *contents* (not the metadata above).
  void RLatch() { latch_.lock_shared(); }
  void RUnlatch() { latch_.unlock_shared(); }
  void WLatch() { latch_.lock(); }
  void WUnlatch() { latch_.unlock(); }

 private:
  friend class BufferPoolManager;

  alignas(PAGE_SIZE) std::array<std::byte, PAGE_SIZE> data_{};
  page_id_t page_id_{INVALID_PAGE_ID};
  int pin_count_{0};
  bool is_dirty_{false};
  lsn_t page_lsn_{0};
  std::shared_mutex latch_;
};

}  // namespace minitub
