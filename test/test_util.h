#pragma once

// Helpers shared by the storage and buffer tests.

#include <unistd.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"

namespace minitub::test {

/** A unique path in the system temp dir (native FS in WSL, so O_DIRECT works). Deleted on destruction. */
class TempFile {
 public:
  TempFile() {
    static std::atomic<int> counter{0};
    path_ = std::filesystem::temp_directory_path() /
            ("minitub_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".db");
    std::filesystem::remove(path_);
  }
  ~TempFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }
  DISALLOW_COPY_AND_MOVE(TempFile);

  auto Path() const -> const std::filesystem::path & { return path_; }

 private:
  std::filesystem::path path_;
};

/** One page of bytes, aligned for O_DIRECT. */
struct alignas(PAGE_SIZE) PageBuf {
  std::array<std::byte, PAGE_SIZE> bytes{};

  auto Span() -> std::span<std::byte, PAGE_SIZE> { return bytes; }
  auto Data() -> std::byte * { return bytes.data(); }
};

/** Fills a page with a pattern that depends on `seed`, so different seeds give different pages. */
inline void Fill(std::span<std::byte, PAGE_SIZE> page, std::uint32_t seed) {
  for (std::size_t i = 0; i < PAGE_SIZE; i++) {
    page[i] = static_cast<std::byte>((seed * 131 + i * 7 + 1) & 0xFF);
  }
}

inline auto IsZero(const std::byte *data) -> bool {
  for (std::size_t i = 0; i < PAGE_SIZE; i++) {
    if (data[i] != std::byte{0}) {
      return false;
    }
  }
  return true;
}

// Pages in buffer pool tests carry a "stamp" (their own page id) at offset 0
// and a counter at offset 8, so a test can detect frames mixed up between pages.
inline void PutU64(std::byte *data, std::size_t offset, std::uint64_t value) {
  std::memcpy(data + offset, &value, sizeof(value));
}
inline auto GetU64(const std::byte *data, std::size_t offset) -> std::uint64_t {
  std::uint64_t value = 0;
  std::memcpy(&value, data + offset, sizeof(value));
  return value;
}
inline constexpr std::size_t kStampOffset = 0;
inline constexpr std::size_t kCounterOffset = 8;

/** Runs `fn` and expects a minitub::Exception of the given type. */
template <typename Fn>
void ExpectThrowsType(Fn &&fn, ExceptionType type) {
  try {
    fn();
    ADD_FAILURE() << "expected Exception(" << ExceptionTypeToString(type) << "), nothing was thrown";
  } catch (const Exception &e) {
    EXPECT_EQ(e.GetType(), type) << e.what();
  }
}

}  // namespace minitub::test
