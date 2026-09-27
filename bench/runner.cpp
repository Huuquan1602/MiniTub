#include "runner.h"

#include <atomic>
#include <chrono>
#include <latch>
#include <thread>
#include <vector>

namespace minitub::bench {

auto Run(Workload &workload, const RunOptions &options) -> RunResult {
  using Clock = std::chrono::steady_clock;

  std::vector<Histogram> histograms(options.threads);  // one per thread: no locking while timing
  std::atomic<bool> stop{false};
  std::latch start(options.threads + 1);  // workers + this thread start together

  std::vector<std::thread> workers;
  for (int t = 0; t < options.threads; t++) {
    workers.emplace_back([&, t] {
      std::mt19937_64 rng(options.seed + t);  // per-thread seed: reproducible, but threads differ
      start.arrive_and_wait();
      while (!stop.load(std::memory_order_relaxed)) {
        auto begin = Clock::now();
        workload.Op(t, rng);
        histograms[t].Record(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count());
      }
    });
  }

  start.arrive_and_wait();
  auto begin = Clock::now();
  std::this_thread::sleep_for(std::chrono::duration<double>(options.seconds));
  stop.store(true, std::memory_order_relaxed);
  for (auto &w : workers) {
    w.join();
  }

  RunResult result;
  result.elapsed_seconds = std::chrono::duration<double>(Clock::now() - begin).count();
  for (const auto &h : histograms) {
    result.latency_ns.Merge(h);
  }
  result.total_ops = result.latency_ns.Count();
  return result;
}

}  // namespace minitub::bench
