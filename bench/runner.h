#pragma once

#include <cstdint>

#include "histogram.h"
#include "workload.h"

namespace minitub::bench {

struct RunOptions {
  int threads{1};
  double seconds{5.0};
  std::uint64_t seed{42};
};

struct RunResult {
  std::int64_t total_ops{0};
  double elapsed_seconds{0};
  Histogram latency_ns;  // merged across threads
};

/** Runs `workload` on `options.threads` threads for `options.seconds`, timing every Op(). */
auto Run(Workload &workload, const RunOptions &options) -> RunResult;

}  // namespace minitub::bench
