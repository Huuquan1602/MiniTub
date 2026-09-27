#include "common/macros.h"

#include <type_traits>

#include <gtest/gtest.h>

namespace minitub {

namespace {

class NoCopy {
 public:
  NoCopy() = default;
  DISALLOW_COPY(NoCopy);
  NoCopy(NoCopy &&) = default;
  auto operator=(NoCopy &&) -> NoCopy & = default;
};

class Pinned {
 public:
  Pinned() = default;
  DISALLOW_COPY_AND_MOVE(Pinned);
};

}  // namespace

TEST(MacrosTest, DisallowCopyKeepsMove) {
  static_assert(!std::is_copy_constructible_v<NoCopy>);
  static_assert(!std::is_copy_assignable_v<NoCopy>);
  static_assert(std::is_move_constructible_v<NoCopy>);
  static_assert(std::is_move_assignable_v<NoCopy>);
}

TEST(MacrosTest, DisallowCopyAndMove) {
  static_assert(!std::is_copy_constructible_v<Pinned>);
  static_assert(!std::is_copy_assignable_v<Pinned>);
  static_assert(!std::is_move_constructible_v<Pinned>);
  static_assert(!std::is_move_assignable_v<Pinned>);
}

TEST(MacrosTest, AssertPassesOnTrue) { MT_ASSERT(1 + 1 == 2, "math works"); }

#ifndef NDEBUG
TEST(MacrosDeathTest, AssertAbortsWithMessage) {
  // "threadsafe" re-runs the test binary for the child; safe with sanitizers.
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEATH(MT_ASSERT(1 + 1 == 3, "math is broken"), "math is broken");
}
#endif

}  // namespace minitub
