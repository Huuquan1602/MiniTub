#include "buffer/lru_k_replacer.h"

#include <string>

#include "common/exception.h"

namespace minitub {

LRUKReplacer::LRUKReplacer(std::size_t num_frames, std::size_t k) : num_frames_(num_frames), k_(k) {
  if (k_ == 0) {
    throw Exception(ExceptionType::Invalid, "LRU-K needs k >= 1");
  }
}

void LRUKReplacer::CheckFrameId(frame_id_t frame_id) const {
  // num_frames_ is const, so this check needs no latch.
  if (frame_id < 0 || static_cast<std::size_t>(frame_id) >= num_frames_) {
    throw Exception(ExceptionType::OutOfRange,
                    "frame " + std::to_string(frame_id) + " outside [0, " + std::to_string(num_frames_) + ")");
  }
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
  CheckFrameId(frame_id);
  std::scoped_lock lock(latch_);
  // operator[] creates the entry on first access with evictable = false, as the spec requires.
  FrameInfo &info = frames_[frame_id];
  info.history.push_back(current_timestamp_++);
  if (info.history.size() > k_) {
    info.history.pop_front();  // keep only the last k accesses
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool evictable) {
  CheckFrameId(frame_id);
  std::scoped_lock lock(latch_);
  auto it = frames_.find(frame_id);
  if (it == frames_.end() || it->second.evictable == evictable) {
    return;  // untracked frame, or no change: Size() must not move
  }
  it->second.evictable = evictable;
  if (evictable) {
    evictable_count_++;
  } else {
    evictable_count_--;
  }
}

auto LRUKReplacer::Evict() -> std::optional<frame_id_t> {
  std::scoped_lock lock(latch_);
  // Victim rule (design.md 7.4), using front() = oldest kept timestamp:
  //  1. a frame with < k accesses (+inf distance) beats any frame with k accesses;
  //  2. inside the same group, the smaller front() wins:
  //     +inf group  -> earliest oldest access (the tie-break rule),
  //     finite group -> earliest k-th most recent access = largest backward k-distance.
  auto victim = frames_.end();
  bool victim_infinite = false;
  for (auto it = frames_.begin(); it != frames_.end(); ++it) {
    const FrameInfo &info = it->second;
    if (!info.evictable) {
      continue;  // pinned in the buffer pool
    }
    const bool infinite = info.history.size() < k_;
    const bool better = victim == frames_.end() || (infinite && !victim_infinite) ||
                        (infinite == victim_infinite && info.history.front() < victim->second.history.front());
    if (better) {
      victim = it;
      victim_infinite = infinite;
    }
  }
  if (victim == frames_.end()) {
    return std::nullopt;  // nothing evictable
  }
  const frame_id_t frame_id = victim->first;
  frames_.erase(victim);  // drop the history: a later access starts fresh
  evictable_count_--;
  return frame_id;
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  CheckFrameId(frame_id);
  std::scoped_lock lock(latch_);
  auto it = frames_.find(frame_id);
  if (it == frames_.end()) {
    return;  // untracked: nothing to do
  }
  if (!it->second.evictable) {
    throw Exception(ExceptionType::Invalid, "cannot remove non-evictable frame " + std::to_string(frame_id));
  }
  frames_.erase(it);
  evictable_count_--;
}

auto LRUKReplacer::Size() const -> std::size_t {
  std::scoped_lock lock(latch_);
  return evictable_count_;
}

}  // namespace minitub
