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

  auto registry = std::make_shared<Registry>();

  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  auto& counter_family = BuildCounter()
                             .Name("time_running_seconds_total")
                             .Help("How many seconds is this server running?")
                             .Register(*registry);

  // add a counter to the metric family
  auto& seconds_counter = counter_family.Add(
      {{"another_label", "bar"}, {"yet_another_label", "baz"}});

  // ask the exposer to scrape registry on incoming scrapes for "/metrics"
  exposer.RegisterCollectable(registry, "/metrics");
  exposer.RegisterAuth(
      [](const std::string& user, const std::string& password) {
        return user == "test_user" && password == "test_password";
      },
      "Some Auth Realm");

  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // increment the counters by one (second)
    seconds_counter.Increment(1.0);
  }
  return 0;
}
