#pragma once

#include <memory>
#include <vector>

#include "prometheus/metric_family.h"

namespace prometheus {
class Collectable;
namespace detail {
std::vector<prometheus::MetricFamily> CollectMetrics(
    const std::vector<std::weak_ptr<prometheus::Collectable>>& collectables);
}  // namespace detail
}  // namespace prometheus
