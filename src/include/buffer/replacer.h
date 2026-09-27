#pragma once

#include <cstddef>
#include <optional>

#include "common/config.h"

namespace minitub {

/**
 * Chooses which buffer pool frame to evict. Tracks frames, not pages.
 * Implementations: LRUKReplacer (M1); LRU and Clock later (experiment E1).
 */
class Replacer {
 public:
  virtual ~Replacer() = default;

  /** Removes and returns the best victim among evictable frames, or nullopt. */
  virtual auto Evict() -> std::optional<frame_id_t> = 0;
  /** Records that `frame_id` was accessed now. A new frame starts non-evictable. */
  virtual void RecordAccess(frame_id_t frame_id) = 0;
  virtual void SetEvictable(frame_id_t frame_id, bool evictable) = 0;
  /** Forgets an evictable frame (its page was deleted). */
  virtual void Remove(frame_id_t frame_id) = 0;
  /** Number of evictable frames. */
  virtual auto Size() const -> std::size_t = 0;
};

}  // namespace minitub
