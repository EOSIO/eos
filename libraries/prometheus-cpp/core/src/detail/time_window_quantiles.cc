#include "prometheus/detail/time_window_quantiles.h"  // IWYU pragma: export

#include <memory>
#include <ratio>

namespace prometheus {
namespace detail {

TimeWindowQuantiles::TimeWindowQuantiles(
    const std::vector<CKMSQuantiles::Quantile>& quantiles,
    const Clock::duration max_age, const int age_buckets)
    : quantiles_(quantiles),
      ckms_quantiles_(age_buckets, CKMSQuantiles(quantiles_)),
      current_bucket_(0),
      last_rotation_(Clock::now()),
      rotation_interval_(max_age / age_buckets) {}

double TimeWindowQuantiles::get(double q) const {
  CKMSQuantiles& current_bucket = rotate();
  return current_bucket.get(q);
}

void TimeWindowQuantiles::insert(double value) {
  rotate();
  for (auto& bucket : ckms_quantiles_) {
    bucket.insert(value);
  }
}

CKMSQuantiles& TimeWindowQuantiles::rotate() const {
  auto delta = Clock::now() - last_rotation_;
  while (delta > rotation_interval_) {
    ckms_quantiles_[current_bucket_].reset();

    if (++current_bucket_ >= ckms_quantiles_.size()) {
      current_bucket_ = 0;
    }

    delta -= rotation_interval_;
    last_rotation_ += rotation_interval_;
  }
  return ckms_quantiles_[current_bucket_];
}

}  // namespace detail
}  // namespace prometheus
