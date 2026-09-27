#include "report.h"

#include <chrono>
#include <format>
#include <sstream>

#include "common/engine_config_toml.h"
#include "git_version.h"
#include "system_info.h"

namespace minitub::bench {

namespace {

auto CompilerName() -> std::string {
#if defined(__clang__)
  return "clang " __clang_version__;
#elif defined(__GNUC__)
  return "gcc " __VERSION__;
#else
  return "unknown";
#endif
}

auto UtcNow() -> std::string {
  auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", now);
}

}  // namespace

auto ToJson(const ReportInput &in) -> std::string {
  const Histogram &lat = in.result->latency_ns;
  SystemInfo sys = GetSystemInfo();

  // Built as a TOML table, printed as JSON: toml++ does both, so no extra JSON library.
  toml::table report{
      {"workload", in.workload},
      {"timestamp_utc", UtcNow()},
      {"params", toml::table{{"threads", in.options.threads},
                             {"seconds", in.options.seconds},
                             {"seed", static_cast<std::int64_t>(in.options.seed)}}},
      {"results",
       toml::table{{"total_ops", in.result->total_ops},
                   {"elapsed_seconds", in.result->elapsed_seconds},
                   {"throughput_ops_per_sec", static_cast<double>(in.result->total_ops) / in.result->elapsed_seconds},
                   {"latency_ns", toml::table{{"p50", lat.Percentile(50.0)},
                                              {"p95", lat.Percentile(95.0)},
                                              {"p99", lat.Percentile(99.0)},
                                              {"min", lat.Min()},
                                              {"max", lat.Max()},
                                              {"mean", lat.Mean()}}}}},
      {"config", ToToml(*in.config)},
      {"build", toml::table{{"git_commit", kGitCommit},
                            {"git_dirty", kGitDirty},
                            {"build_type", MINITUB_BUILD_TYPE},
                            {"sanitizer", MINITUB_SANITIZER_NAME},
                            {"compiler", CompilerName()}}},
      {"machine", toml::table{{"cpu_model", sys.cpu_model},
                              {"logical_cores", sys.logical_cores},
                              {"ram_bytes", sys.ram_bytes},
                              {"os", sys.os}}},
  };

  std::ostringstream out;
  out << toml::json_formatter{report} << "\n";
  return out.str();
}

}  // namespace minitub::bench
