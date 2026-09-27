#pragma once

#include <cstddef>
#include <optional>

#include "buffer/replacer.h"
#include "common/macros.h"

namespace minitub {

/**
 * LRU-K: evicts the frame with the largest backward k-distance.
 * Frames with fewer than k accesses have distance +inf; ties among them go to the
 * frame whose oldest recorded access is earliest. Rules: design.md section 7.4.
 * Thread-safe.
 */
class LRUKReplacer : public Replacer {
 public:
  /** Tracks frames 0 .. num_frames-1, keeping the last `k` accesses of each. */
  LRUKReplacer(std::size_t num_frames, std::size_t k);
  DISALLOW_COPY_AND_MOVE(LRUKReplacer);

  auto Evict() -> std::optional<frame_id_t> override;
  void RecordAccess(frame_id_t frame_id) override;
  void SetEvictable(frame_id_t frame_id, bool evictable) override;
  void Remove(frame_id_t frame_id) override;
  auto Size() const -> std::size_t override;

 private:
  // TODO(M1): your members (per-frame access history, logical clock, latch, ...).
};

}  // namespace minitub
