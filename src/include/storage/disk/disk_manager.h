#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

#include "common/config.h"
#include "common/engine_config.h"
#include "common/macros.h"

namespace minitub {

/** Snapshot of DiskManager counters (experiment E2). */
struct DiskStats {
  std::uint64_t reads{0};   // ReadPage calls
  std::uint64_t writes{0};  // WritePage calls
};

/**
 * Reads and writes fixed-size pages in one database file.
 * Rules: docs/design.md section 7.1. Thread-safe.
 */
class DiskManager {
 public:
  /** Opens `file`, creating it if missing. Throws Exception(Io) on failure. */
  explicit DiskManager(const std::filesystem::path &file, IoMode mode = IoMode::Buffered);
  ~DiskManager();
  DISALLOW_COPY_AND_MOVE(DiskManager);

  /** Copies page `page_id` into `out`. Throws OutOfRange if the page was never allocated. */
  void ReadPage(page_id_t page_id, std::span<std::byte, PAGE_SIZE> out);

  /** Writes `data` to page `page_id`. Throws OutOfRange if the page was never allocated. */
  void WritePage(page_id_t page_id, std::span<const std::byte, PAGE_SIZE> data);

  /** Returns the next page id (== NumPages()) and grows the file by one zeroed page. */
  auto AllocatePage() -> page_id_t;

  /** Marks a page as free. M1: ids are never reused. */
  void DeallocatePage(page_id_t page_id);

  /** Number of allocated pages (file size / PAGE_SIZE). */
  auto NumPages() const -> page_id_t;

  auto GetStats() const -> DiskStats { return {num_reads_.load(), num_writes_.load()}; }

 private:
  // Increment these in ReadPage / WritePage.
  std::atomic<std::uint64_t> num_reads_{0};
  std::atomic<std::uint64_t> num_writes_{0};

  // TODO(M1): your members (file descriptor, page count, latch, ...).
};

}  // namespace minitub
