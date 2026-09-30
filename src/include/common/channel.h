#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

#include "common/macros.h"

namespace minitub {

/**
 * Unbounded blocking FIFO queue for many producers and consumers.
 * Put() never blocks; Get() waits until an item is available.
 * Every item is delivered to exactly one Get(). Rules: design.md section 7.2.
 * T only needs to be movable (DiskRequest holds a std::promise, which cannot be copied).
 */
template <typename T>
class Channel {
 public:
  Channel() = default;
  DISALLOW_COPY_AND_MOVE(Channel);

  void Put(T value) {
    {
      std::scoped_lock lock(latch_);
      queue_.push(std::move(value));
    }
    // Notify after unlocking, so the woken thread does not immediately block on latch_.
    not_empty_.notify_one();
  }

  auto Get() -> T {
    // unique_lock (not scoped_lock): wait() must unlock while sleeping and relock on wake-up.
    std::unique_lock lock(latch_);
    // The predicate handles spurious wake-ups and items taken by another consumer first.
    not_empty_.wait(lock, [this] { return !queue_.empty(); });
    T value = std::move(queue_.front());
    queue_.pop();
    return value;
  }

 private:
  std::mutex latch_;                   // guards queue_
  std::condition_variable not_empty_;  // signalled by Put(), waited on by Get()
  std::queue<T> queue_;
};

}  // namespace minitub
