// Placeholder micro-benchmark proving Google Benchmark is wired in.
// Real ones (e.g. replacer Evict()) arrive with their modules.
#include <atomic>
#include <cstdint>

#include <benchmark/benchmark.h>

namespace {

void BM_AtomicIncrement(benchmark::State &state) {
  std::atomic<std::uint64_t> counter{0};
  for (auto _ : state) {
    counter.fetch_add(1, std::memory_order_relaxed);
  }
  benchmark::DoNotOptimize(counter.load());
}
BENCHMARK(BM_AtomicIncrement);

}  // namespace
