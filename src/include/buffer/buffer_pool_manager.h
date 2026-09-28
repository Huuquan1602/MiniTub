#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "common/config.h"
#include "common/macros.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"

namespace minitub {

class DiskManager;

/** Snapshot of BufferPoolManager counters (experiments E1/E2). */
struct BufferPoolStats {
  std::uint64_t hits{0};       // FetchPage found the page in memory
  std::uint64_t misses{0};     // FetchPage had to read the page from disk
  std::uint64_t evictions{0};  // frames taken from the replacer
};

/**
 * Caches disk pages in a fixed number of frames. Rules: design.md section 7.5.
 * Thread-safe. All disk I/O goes through an owned DiskScheduler.
 */
class BufferPoolManager {
 public:
  BufferPoolManager(std::size_t pool_size, DiskManager *disk_manager, std::size_t replacer_k = 2, int disk_workers = 1);
  ~BufferPoolManager();
  DISALLOW_COPY_AND_MOVE(BufferPoolManager);

  /** Allocates a new zeroed page, pinned once. nullptr if no frame is free or evictable. */
  auto NewPage() -> Page *;
  /** Pins page `page_id`, reading it from disk if needed. nullptr if no frame is free or evictable. */
  auto FetchPage(page_id_t page_id) -> Page *;
  /** Drops one pin; `is_dirty` is sticky. false if not resident or pin count already 0. */
  auto UnpinPage(page_id_t page_id, bool is_dirty) -> bool;
  /** Writes the page to disk (dirty or not) and clears its dirty flag. false if not resident. */
  auto FlushPage(page_id_t page_id) -> bool;
  void FlushAllPages();
  /** Frees the frame and deallocates the id. false if pinned; true if not resident. */
  auto DeletePage(page_id_t page_id) -> bool;

  // Guard versions: the guard unpins (and unlatches) when it goes out of scope.
  // On failure (no frame) the returned guard is empty (IsValid() == false).
  auto NewPageGuarded() -> BasicPageGuard;
  auto FetchPageBasic(page_id_t page_id) -> BasicPageGuard;
  auto FetchPageRead(page_id_t page_id) -> ReadPageGuard;
  auto FetchPageWrite(page_id_t page_id) -> WritePageGuard;

  /** Number of frames on the free list (never used, or freed by DeletePage). */
  auto FreeFrameCount() const -> std::size_t;

  auto GetStats() const -> BufferPoolStats { return {num_hits_.load(), num_misses_.load(), num_evictions_.load()}; }

 private:
  // Increment these where the event happens.
  std::atomic<std::uint64_t> num_hits_{0};
  std::atomic<std::uint64_t> num_misses_{0};
  std::atomic<std::uint64_t> num_evictions_{0};

  // TODO(M1): your members (frames, page table, free list, replacer, disk scheduler, latch, ...).
};

}  // namespace minitub
