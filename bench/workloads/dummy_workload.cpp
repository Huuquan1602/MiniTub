#include "workloads/dummy_workload.h"

namespace minitub::bench {

void DummyWorkload::Op(int /*thread_idx*/, std::mt19937_64 & /*rng*/) {
  counter_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace minitub::bench
