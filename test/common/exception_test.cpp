#include "common/exception.h"

#include <gtest/gtest.h>

namespace minitub {

TEST(ExceptionTest, DefaultTypeIsInvalid) {
  Exception e("bad thing");
  EXPECT_EQ(e.GetType(), ExceptionType::Invalid);
  EXPECT_STREQ(e.what(), "Invalid: bad thing");
}

TEST(ExceptionTest, CarriesTypeInMessage) {
  Exception e(ExceptionType::Config, "unknown key 'x'");
  EXPECT_EQ(e.GetType(), ExceptionType::Config);
  EXPECT_STREQ(e.what(), "Config: unknown key 'x'");
}

TEST(ExceptionTest, IoTypeName) { EXPECT_STREQ(Exception(ExceptionType::Io, "disk full").what(), "Io: disk full"); }

TEST(ExceptionTest, CatchableAsStdException) {
  EXPECT_THROW(throw Exception(ExceptionType::NotImplemented, "later"), std::runtime_error);
}

}  // namespace minitub
