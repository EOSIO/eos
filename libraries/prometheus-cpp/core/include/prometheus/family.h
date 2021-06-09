#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "prometheus/client_metric.h"
#include "prometheus/collectable.h"
#include "prometheus/detail/core_export.h"
#include "prometheus/detail/future_std.h"
#include "prometheus/metric_family.h"

// IWYU pragma: no_include "prometheus/counter.h"
// IWYU pragma: no_include "prometheus/gauge.h"
// IWYU pragma: no_include "prometheus/histogram.h"
// IWYU pragma: no_include "prometheus/summary.h"

namespace prometheus {

/// \brief A metric of type T with a set of labeled dimensions.
///
/// One of Prometheus main feature is a multi-dimensional data model with time
/// series data identified by metric name and key/value pairs, also known as
/// labels. A time series is a series of data points indexed (or listed or
/// graphed) in time order (https://en.wikipedia.org/wiki/Time_series).
///
/// An instance of this class is exposed as multiple time series during
/// scrape, i.e., one time series for each set of labels provided to Add().
///
/// For example it is possible to collect data for a metric
/// `http_requests_total`, with two time series:
///
/// - all HTTP requests that used the method POST
/// - all HTTP requests that used the method GET
///
/// The metric name specifies the general feature of a system that is
/// measured, e.g., `http_requests_total`. Labels enable Prometheus's
/// dimensional data model: any given combination of labels for the same
/// metric name identifies a particular dimensional instantiation of that
/// metric. For example a label for 'all HTTP requests that used the method
/// POST' can be assigned with `method= "POST"`.
///
/// Given a metric name and a set of labels, time series are frequently
/// identified using this notation:
///
///     <metric name> { < label name >= <label value>, ... }
///
/// It is required to follow the syntax of metric names and labels given by:
/// https://prometheus.io/docs/concepts/data_model/#metric-names-and-labels
///
/// The following metric and label conventions are not required for using
/// Prometheus, but can serve as both a style-guide and a collection of best
/// practices: https://prometheus.io/docs/practices/naming/
///
/// \tparam T One of the metric types Counter, Gauge, Histogram or Summary.
template <typename T>
class PROMETHEUS_CPP_CORE_EXPORT Family : public Collectable {
 public:
  /// \brief Create a new metric.
  ///
  /// Every metric is uniquely identified by its name and a set of key-value
  /// pairs, also known as labels. Prometheus's query language allows filtering
  /// and aggregation based on metric name and these labels.
  ///
  /// This example selects all time series that have the `http_requests_total`
  /// metric name:
  ///
  ///     http_requests_total
  ///
  /// It is possible to assign labels to the metric name. These labels are
  /// propagated to each dimensional data added with Add(). For example if a
  /// label `job= "prometheus"` is provided to this constructor, it is possible
  /// to filter this time series with Prometheus's query language by appending
  /// a set of labels to match in curly braces ({})
  ///
  ///     http_requests_total{job= "prometheus"}
  ///
  /// For further information see: [Quering Basics]
  /// (https://prometheus.io/docs/prometheus/latest/querying/basics/)
  ///
  /// \param name Set the metric name.
  /// \param help Set an additional description.
  /// \param constant_labels Assign a set of key-value pairs (= labels) to the
  /// metric. All these labels are propagated to each time series within the
  /// metric.
  /// \throw std::runtime_exception on invalid metric or label names.
  Family(const std::string& name, const std::string& help,
         const std::map<std::string, std::string>& constant_labels);

  /// \brief Add a new dimensional data.
  ///
  /// Each new set of labels adds a new dimensional data and is exposed in
  /// Prometheus as a time series. It is possible to filter the time series
  /// with Prometheus's query language by appending a set of labels to match in
  /// curly braces ({})
  ///
  ///     http_requests_total{job= "prometheus",method= "POST"}
  ///
  /// \param labels Assign a set of key-value pairs (= labels) to the
  /// dimensional data. The function does nothing, if the same set of labels
  /// already exists.
  /// \param args Arguments are passed to the constructor of metric type T. See
  /// Counter, Gauge, Histogram or Summary for required constructor arguments.
  /// \return Return the newly created dimensional data or - if a same set of
  /// labels already exists - the already existing dimensional data.
  /// \throw std::runtime_exception on invalid label names.
  template <typename... Args>
  T& Add(const std::map<std::string, std::string>& labels, Args&&... args) {
    return Add(labels, detail::make_unique<T>(args...));
  }

  /// \brief Remove the given dimensional data.
  ///
  /// \param metric Dimensional data to be removed. The function does nothing,
  /// if the given metric was not returned by Add().
  void Remove(T* metric);

  /// \brief Returns true if the dimensional data with the given labels exist
  ///
  /// \param labels A set of key-value pairs (= labels) of the dimensional data.
  bool Has(const std::map<std::string, std::string>& labels) const;

  /// \brief Returns the name for this family.
  ///
  /// \return The family name.
  const std::string& GetName() const;

  /// \brief Returns the constant labels for this family.
  ///
  /// \return All constant labels as key-value pairs.
  const std::map<std::string, std::string> GetConstantLabels() const;

  /// \brief Returns the current value of each dimensional data.
  ///
  /// Collect is called by the Registry when collecting metrics.
  ///
  /// \return Zero or more samples for each dimensional data.
  std::vector<MetricFamily> Collect() const override;

 private:
  std::unordered_map<std::size_t, std::unique_ptr<T>> metrics_;
  std::unordered_map<std::size_t, std::map<std::string, std::string>> labels_;
  std::unordered_map<T*, std::size_t> labels_reverse_lookup_;

  const std::string name_;
  const std::string help_;
  const std::map<std::string, std::string> constant_labels_;
  mutable std::mutex mutex_;

  ClientMetric CollectMetric(std::size_t hash, T* metric) const;
  T& Add(const std::map<std::string, std::string>& labels,
         std::unique_ptr<T> object);
};

}  // namespace prometheus
