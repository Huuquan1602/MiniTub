#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <span>
#include <unordered_set>

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

  std::filesystem::path path_;  // kept for error messages
  int fd_{-1};                  // POSIX file descriptor from open()

  // Pages [0, num_pages_) exist in the file. Atomic so ReadPage/WritePage/NumPages can
  // check it without taking the latch; only AllocatePage changes it (under latch_).
  std::atomic<page_id_t> num_pages_{0};

  // Serializes AllocatePage (read count, grow file, bump count) and guards deallocated_.
  std::mutex latch_;
  std::unordered_set<page_id_t> deallocated_;  // M1: recorded only, never reused
};

}  // namespace minitub
