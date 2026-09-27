#pragma once

#include <string>

#include "common/engine_config.h"
#include "runner.h"

namespace minitub::bench {

/** Everything that describes one benchmark run. */
struct ReportInput {
  std::string workload;
  RunOptions options;
  const EngineConfig *config;
  const RunResult *result;
};

/** The result as pretty-printed JSON (schema in bench/README section of the main README). */
auto ToJson(const ReportInput &input) -> std::string;

}  // namespace minitub::bench
