#include "system_info.h"

#include <fstream>
#include <sstream>
#include <thread>

#if defined(__linux__)
#include <sys/utsname.h>
#endif

namespace minitub::bench {

namespace {

// Returns the text after "key<spaces>:" on the first matching line of a /proc file.
auto ProcField(const char *path, const std::string &key) -> std::string {
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    if (line.starts_with(key)) {
      auto colon = line.find(':');
      auto start = line.find_first_not_of(" \t", colon + 1);
      return start == std::string::npos ? "" : line.substr(start);
    }
  }
  return "";
}

}  // namespace

auto GetSystemInfo() -> SystemInfo {
  SystemInfo info;
  info.logical_cores = static_cast<int>(std::thread::hardware_concurrency());
#if defined(__linux__)
  if (auto model = ProcField("/proc/cpuinfo", "model name"); !model.empty()) {
    info.cpu_model = model;
  }
  // /proc/meminfo reports "MemTotal:  16303284 kB".
  std::istringstream mem(ProcField("/proc/meminfo", "MemTotal"));
  std::int64_t kib = 0;
  if (mem >> kib) {
    info.ram_bytes = kib * 1024;
  }
  utsname uts{};
  if (uname(&uts) == 0) {
    info.os = std::string(uts.sysname) + " " + uts.release;
  }
#endif
  return info;
}

}  // namespace minitub::bench
