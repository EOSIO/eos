#include "prometheus/check_names.h"

#include <gtest/gtest.h>

namespace prometheus {
namespace {

TEST(CheckNamesTest, empty_metric_name) { EXPECT_FALSE(CheckMetricName("")); }
TEST(CheckNamesTest, good_metric_name) {
  EXPECT_TRUE(CheckMetricName("prometheus_notifications_total"));
}
TEST(CheckNamesTest, reserved_metric_name) {
  EXPECT_FALSE(CheckMetricName("__some_reserved_metric"));
}
TEST(CheckNamesTest, malformed_metric_name) {
  EXPECT_FALSE(CheckMetricName("fa mi ly with space in name or |"));
}
TEST(CheckNamesTest, empty_label_name) { EXPECT_FALSE(CheckLabelName("")); }
TEST(CheckNamesTest, invalid_label_name) {
  EXPECT_FALSE(CheckLabelName("log-level"));
}
TEST(CheckNamesTest, leading_invalid_label_name) {
  EXPECT_FALSE(CheckLabelName("-abcd"));
}
TEST(CheckNamesTest, trailing_invalid_label_name) {
  EXPECT_FALSE(CheckLabelName("abcd-"));
}
TEST(CheckNamesTest, good_label_name) { EXPECT_TRUE(CheckLabelName("type")); }
TEST(CheckNamesTest, reserved_label_name) {
  EXPECT_FALSE(CheckMetricName("__some_reserved_label"));
}

}  // namespace
}  // namespace prometheus
