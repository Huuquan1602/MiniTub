#pragma once

#include <cstdint>
#include <string>

namespace minitub::bench {

/** Machine description stored with every result. No hostname or user name on purpose. */
struct SystemInfo {
  std::string cpu_model{"unknown"};
  int logical_cores{0};
  std::int64_t ram_bytes{0};
  std::string os{"unknown"};
};

auto GetSystemInfo() -> SystemInfo;

}  // namespace minitub::bench
