#pragma once

#include <vector>

#include "prometheus/detail/core_export.h"

namespace prometheus {
struct MetricFamily;
}

namespace prometheus {

/// @brief Interface implemented by anything that can be used by Prometheus to
/// collect metrics.
///
/// A Collectable has to be registered for collection. See Registry.
class PROMETHEUS_CPP_CORE_EXPORT Collectable {
 public:
  virtual ~Collectable() = default;

  /// \brief Returns a list of metrics and their samples.
  virtual std::vector<MetricFamily> Collect() const = 0;
};

}  // namespace prometheus
