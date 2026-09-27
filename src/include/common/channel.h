#pragma once

#include "common/exception.h"
#include "common/macros.h"

namespace minitub {

/**
 * Unbounded blocking FIFO queue for many producers and consumers.
 * Put() never blocks; Get() waits until an item is available.
 * Every item is delivered to exactly one Get(). Rules: design.md section 7.2.
 */
template <typename T>
class Channel {
 public:
  Channel() = default;
  DISALLOW_COPY_AND_MOVE(Channel);

  void Put(T /*value*/) { throw Exception(ExceptionType::NotImplemented, "Channel::Put"); }

  auto Get() -> T { throw Exception(ExceptionType::NotImplemented, "Channel::Get"); }

 private:
  // TODO(M1): your members (queue, mutex, condition variable, ...).
};

}  // namespace minitub
