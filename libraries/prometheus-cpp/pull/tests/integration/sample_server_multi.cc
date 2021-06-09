#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "prometheus/client_metric.h"
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/family.h"
#include "prometheus/registry.h"

int main() {
  using namespace prometheus;

  // create an http server running on port 8080
  Exposer exposer{"127.0.0.1:8080", 1};

  auto registryA = std::make_shared<Registry>();

  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  auto& counter_familyA = BuildCounter()
                              .Name("time_running_seconds_total")
                              .Help("How many seconds is this server running?")
                              .Register(*registryA);

  // add a counter to the metric family
  auto& seconds_counterA = counter_familyA.Add(
      {{"another_label", "bar"}, {"yet_another_label", "baz"}});

  // ask the exposer to scrape registryA on incoming scrapes for "/metricsA"
  exposer.RegisterCollectable(registryA, "/metricsA");

  auto registryB = std::make_shared<Registry>();

  auto& counter_familyB =
      BuildCounter()
          .Name("other_time_running_seconds_total")
          .Help("How many seconds has something else been running?")
          .Register(*registryB);

  auto& seconds_counterB = counter_familyB.Add(
      {{"another_label", "not_bar"}, {"yet_another_label", "not_baz"}});

  // This endpoint exposes registryB.
  exposer.RegisterCollectable(registryB, "/metricsB");

  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // increment the counters by one (second)
    seconds_counterA.Increment(1.0);
    seconds_counterB.Increment(1.5);
  }
  return 0;
}
