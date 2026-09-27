#pragma once

#include <atomic>
#include <cstdint>

#include "workload.h"

namespace minitub::bench {

/** Increments a shared counter. Exists only to test the benchmark pipeline end to end. */
class DummyWorkload : public Workload {
 public:
  void Op(int thread_idx, std::mt19937_64 &rng) override;

 private:
  std::atomic<std::uint64_t> counter_{0};
};

}  // namespace minitub::bench
