#include "metrics_collector.h"

#include <iterator>

#include "prometheus/collectable.h"

namespace prometheus {
namespace detail {

std::vector<MetricFamily> CollectMetrics(
    const std::vector<std::weak_ptr<prometheus::Collectable>>& collectables) {
  auto collected_metrics = std::vector<MetricFamily>{};

  for (auto&& wcollectable : collectables) {
    auto collectable = wcollectable.lock();
    if (!collectable) {
      continue;
    }

    auto&& metrics = collectable->Collect();
    collected_metrics.insert(collected_metrics.end(),
                             std::make_move_iterator(metrics.begin()),
                             std::make_move_iterator(metrics.end()));
  }

  return collected_metrics;
}

}  // namespace detail
}  // namespace prometheus
