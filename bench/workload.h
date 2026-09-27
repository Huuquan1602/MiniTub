#pragma once

#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "common/engine_config.h"

namespace minitub::bench {

/** One benchmark workload. The runner calls Op() in a loop from every thread. */
class Workload {
 public:
  virtual ~Workload() = default;

  /** Called once before the timed run, e.g. to build a buffer pool and load data. */
  virtual void Setup(const EngineConfig & /*config*/, int /*threads*/) {}

  /** One timed operation. Must be thread-safe; `rng` belongs to the calling thread. */
  virtual void Op(int thread_idx, std::mt19937_64 &rng) = 0;
};

/** Returns nullptr for an unknown name. */
auto MakeWorkload(std::string_view name) -> std::unique_ptr<Workload>;
auto WorkloadNames() -> std::vector<std::string>;

}  // namespace minitub::bench
