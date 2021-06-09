#include "detail/base64.h"

#include <gtest/gtest.h>

#include <string>

namespace prometheus {
namespace {

struct TestVector {
  const std::string decoded;
  const std::string encoded;
};

const TestVector testVector[] = {
    {"", ""},
    {"f", "Zg=="},
    {"fo", "Zm8="},
    {"foo", "Zm9v"},
    {"foob", "Zm9vYg=="},
    {"fooba", "Zm9vYmE="},
    {"foobar", "Zm9vYmFy"},
};

const unsigned nVectors = sizeof(testVector) / sizeof(testVector[0]);

using namespace testing;

TEST(Base64Test, decodeTest) {
  for (unsigned i = 0; i < nVectors; ++i) {
    std::string decoded = detail::base64_decode(testVector[i].encoded);
    EXPECT_EQ(testVector[i].decoded, decoded);
  }
}

TEST(Base64Test, rejectInvalidSymbols) {
  EXPECT_ANY_THROW(detail::base64_decode("...."));
}

TEST(Base64Test, rejectInvalidInputSize) {
  EXPECT_ANY_THROW(detail::base64_decode("ABC"));
}

TEST(Base64Test, rejectInvalidPadding) {
  EXPECT_ANY_THROW(detail::base64_decode("A==="));
}

}  // namespace
}  // namespace prometheus
