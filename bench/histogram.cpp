#include "histogram.h"

#include <algorithm>
#include <new>
#include <utility>

#include <hdr/hdr_histogram.h>

namespace minitub::bench {

namespace {
// Track 1 ns .. 60 s with 3 significant digits (0.1% precision).
constexpr std::int64_t kMaxNs = 60'000'000'000;
constexpr int kSignificantDigits = 3;
}  // namespace

Histogram::Histogram() {
  if (hdr_init(1, kMaxNs, kSignificantDigits, &h_) != 0) {
    throw std::bad_alloc();
  }
}

Histogram::~Histogram() { hdr_close(h_); }

Histogram::Histogram(Histogram &&other) noexcept : h_(std::exchange(other.h_, nullptr)) {}

auto Histogram::operator=(Histogram &&other) noexcept -> Histogram & {
  std::swap(h_, other.h_);
  return *this;
}

void Histogram::Record(std::int64_t ns) { hdr_record_value(h_, std::clamp<std::int64_t>(ns, 1, kMaxNs)); }

void Histogram::Merge(const Histogram &other) { hdr_add(h_, other.h_); }

auto Histogram::Count() const -> std::int64_t { return h_->total_count; }
auto Histogram::Percentile(double p) const -> std::int64_t { return hdr_value_at_percentile(h_, p); }
auto Histogram::Mean() const -> double { return hdr_mean(h_); }
auto Histogram::Min() const -> std::int64_t { return hdr_min(h_); }
auto Histogram::Max() const -> std::int64_t { return hdr_max(h_); }

}  // namespace minitub::bench
