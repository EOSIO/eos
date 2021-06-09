#pragma once

#include <string>
#include <vector>

#include "prometheus/client_metric.h"
#include "prometheus/detail/core_export.h"
#include "prometheus/metric_type.h"

namespace prometheus {

struct PROMETHEUS_CPP_CORE_EXPORT MetricFamily {
  std::string name;
  std::string help;
  MetricType type = MetricType::Untyped;
  std::vector<ClientMetric> metric;
};
}  // namespace prometheus
