#include "prometheus/registry.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <tuple>

#include "prometheus/counter.h"
#include "prometheus/detail/future_std.h"
#include "prometheus/gauge.h"
#include "prometheus/histogram.h"
#include "prometheus/summary.h"

namespace prometheus {

namespace {
template <typename T>
void CollectAll(std::vector<MetricFamily>& results, const T& families) {
  for (auto&& collectable : families) {
    auto metrics = collectable->Collect();
    results.insert(results.end(), std::make_move_iterator(metrics.begin()),
                   std::make_move_iterator(metrics.end()));
  }
}

bool FamilyNameExists(const std::string& /* name */) { return false; }

template <typename T, typename... Args>
bool FamilyNameExists(const std::string& name, const T& families,
                      Args&&... args) {
  auto sameName = [&name](const typename T::value_type& entry) {
    return name == entry->GetName();
  };
  auto exists = std::find_if(std::begin(families), std::end(families),
                             sameName) != std::end(families);
  return exists || FamilyNameExists(name, args...);
}
}  // namespace

Registry::Registry(InsertBehavior insert_behavior)
    : insert_behavior_{insert_behavior} {}

Registry::~Registry() = default;

std::vector<MetricFamily> Registry::Collect() const {
  std::lock_guard<std::mutex> lock{mutex_};
  auto results = std::vector<MetricFamily>{};

  CollectAll(results, counters_);
  CollectAll(results, gauges_);
  CollectAll(results, histograms_);
  CollectAll(results, summaries_);

  return results;
}

template <>
std::vector<std::unique_ptr<Family<Counter>>>& Registry::GetFamilies() {
  return counters_;
}

template <>
std::vector<std::unique_ptr<Family<Gauge>>>& Registry::GetFamilies() {
  return gauges_;
}

template <>
std::vector<std::unique_ptr<Family<Histogram>>>& Registry::GetFamilies() {
  return histograms_;
}

template <>
std::vector<std::unique_ptr<Family<Summary>>>& Registry::GetFamilies() {
  return summaries_;
}

template <>
bool Registry::NameExistsInOtherType<Counter>(const std::string& name) const {
  return FamilyNameExists(name, gauges_, histograms_, summaries_);
}

template <>
bool Registry::NameExistsInOtherType<Gauge>(const std::string& name) const {
  return FamilyNameExists(name, counters_, histograms_, summaries_);
}

template <>
bool Registry::NameExistsInOtherType<Histogram>(const std::string& name) const {
  return FamilyNameExists(name, counters_, gauges_, summaries_);
}

template <>
bool Registry::NameExistsInOtherType<Summary>(const std::string& name) const {
  return FamilyNameExists(name, counters_, gauges_, histograms_);
}

template <typename T>
Family<T>& Registry::Add(const std::string& name, const std::string& help,
                         const std::map<std::string, std::string>& labels) {
  std::lock_guard<std::mutex> lock{mutex_};

  if (NameExistsInOtherType<T>(name)) {
    throw std::invalid_argument(
        "Family name already exists with different type");
  }

  auto& families = GetFamilies<T>();

  if (insert_behavior_ == InsertBehavior::Merge) {
    auto same_name_and_labels =
        [&name, &labels](const std::unique_ptr<Family<T>>& family) {
          return std::tie(name, labels) ==
                 std::tie(family->GetName(), family->GetConstantLabels());
        };

    auto it =
        std::find_if(families.begin(), families.end(), same_name_and_labels);
    if (it != families.end()) {
      return **it;
    }
  }

  if (insert_behavior_ != InsertBehavior::NonStandardAppend) {
    auto same_name = [&name](const std::unique_ptr<Family<T>>& family) {
      return name == family->GetName();
    };

    auto it = std::find_if(families.begin(), families.end(), same_name);
    if (it != families.end()) {
      throw std::invalid_argument("Family name already exists");
    }
  }

  auto family = detail::make_unique<Family<T>>(name, help, labels);
  auto& ref = *family;
  families.push_back(std::move(family));
  return ref;
}

template Family<Counter>& Registry::Add(
    const std::string& name, const std::string& help,
    const std::map<std::string, std::string>& labels);

template Family<Gauge>& Registry::Add(
    const std::string& name, const std::string& help,
    const std::map<std::string, std::string>& labels);

template Family<Summary>& Registry::Add(
    const std::string& name, const std::string& help,
    const std::map<std::string, std::string>& labels);

template Family<Histogram>& Registry::Add(
    const std::string& name, const std::string& help,
    const std::map<std::string, std::string>& labels);

}  // namespace prometheus
