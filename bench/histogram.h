#pragma once

#include <cstdint>

#include "common/macros.h"

struct hdr_histogram;  // NOLINT(readability-identifier-naming): C library type

namespace minitub::bench {

/** Latency histogram (nanoseconds) backed by HdrHistogram. Not thread-safe: use one per thread. */
class Histogram {
 public:
  Histogram();
  ~Histogram();
  DISALLOW_COPY(Histogram);
  Histogram(Histogram &&other) noexcept;
  auto operator=(Histogram &&other) noexcept -> Histogram &;

  void Record(std::int64_t ns);
  void Merge(const Histogram &other);

  auto Count() const -> std::int64_t;
  auto Percentile(double p) const -> std::int64_t;
  auto Mean() const -> double;
  auto Min() const -> std::int64_t;
  auto Max() const -> std::int64_t;

 private:
  hdr_histogram *h_{nullptr};
};

}  // namespace minitub::bench
