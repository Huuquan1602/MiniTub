#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <unordered_map>

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
  /** What the replacer knows about one tracked frame. */
  struct FrameInfo {
    // Timestamps of the last (up to) k accesses, oldest first. Because only the last k are
    // kept, front() is both "the k-th most recent access" (when size() == k) and
    // "the oldest recorded access" (when size() < k): exactly what Evict() compares.
    std::deque<std::size_t> history;
    bool evictable{false};
  };

  void CheckFrameId(frame_id_t frame_id) const;

  const std::size_t num_frames_;
  const std::size_t k_;

  mutable std::mutex latch_;                          // guards everything below
  std::size_t current_timestamp_{0};                  // logical clock: +1 per RecordAccess
  std::size_t evictable_count_{0};                    // what Size() returns
  std::unordered_map<frame_id_t, FrameInfo> frames_;  // only frames seen by RecordAccess
};

}  // namespace minitub
