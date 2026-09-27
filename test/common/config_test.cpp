#include "common/config.h"

#include <type_traits>

#include <gtest/gtest.h>

namespace minitub {

TEST(ConfigTest, PageSizeIs4KiB) { EXPECT_EQ(PAGE_SIZE, 4096U); }

TEST(ConfigTest, IdTypesAreSignedSoInvalidCanBeNegative) {
  static_assert(std::is_signed_v<page_id_t>);
  static_assert(std::is_signed_v<frame_id_t>);
  static_assert(std::is_signed_v<txn_id_t>);
  static_assert(std::is_signed_v<lsn_t>);
  EXPECT_EQ(INVALID_PAGE_ID, -1);
}

TEST(ConfigTest, WideIdsAre64Bit) {
  EXPECT_EQ(sizeof(txn_id_t), 8U);
  EXPECT_EQ(sizeof(lsn_t), 8U);
}

}  // namespace minitub
