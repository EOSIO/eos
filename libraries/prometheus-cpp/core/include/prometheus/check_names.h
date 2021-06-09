#pragma once

#include <string>

#include "prometheus/detail/core_export.h"

namespace prometheus {

PROMETHEUS_CPP_CORE_EXPORT bool CheckMetricName(const std::string& name);
PROMETHEUS_CPP_CORE_EXPORT bool CheckLabelName(const std::string& name);
}  // namespace prometheus
