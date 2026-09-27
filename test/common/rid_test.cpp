#include "common/rid.h"

#include <gtest/gtest.h>

namespace minitub {

TEST(RIDTest, DefaultIsInvalid) {
  RID rid;
  EXPECT_FALSE(rid.IsValid());
  EXPECT_EQ(rid.GetPageId(), INVALID_PAGE_ID);
}

TEST(RIDTest, StoresPageAndSlot) {
  RID rid(7, 3);
  EXPECT_TRUE(rid.IsValid());
  EXPECT_EQ(rid.GetPageId(), 7);
  EXPECT_EQ(rid.GetSlotNum(), 3U);
  EXPECT_EQ(rid.ToString(), "RID(7, 3)");
}

TEST(RIDTest, Equality) {
  EXPECT_EQ(RID(1, 2), RID(1, 2));
  EXPECT_NE(RID(1, 2), RID(1, 3));
  EXPECT_NE(RID(1, 2), RID(2, 2));
}

}  // namespace minitub
