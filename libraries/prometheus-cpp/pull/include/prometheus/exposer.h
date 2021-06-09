#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "prometheus/collectable.h"
#include "prometheus/detail/pull_export.h"

class CivetServer;
struct CivetCallbacks;

namespace prometheus {

namespace detail {
class Endpoint;
}  // namespace detail

class PROMETHEUS_CPP_PULL_EXPORT Exposer {
 public:
  /// @note This ctor will be merged with the "callback" variant once
  /// prometheus-cpp 0.13 performs an ABI-break
  explicit Exposer(const std::string& bind_address,
                   const std::size_t num_threads = 2);
  /// @note This ctor will be merged with the "callback" variant once
  /// prometheus-cpp 0.13 performs an ABI-break
  explicit Exposer(std::vector<std::string> options);

  explicit Exposer(const std::string& bind_address,
                   const std::size_t num_threads,
                   const CivetCallbacks* callbacks);
  explicit Exposer(std::vector<std::string> options,
                   const CivetCallbacks* callbacks);
  ~Exposer();
  void RegisterCollectable(const std::weak_ptr<Collectable>& collectable,
                           const std::string& uri = std::string("/metrics"));

  void RegisterAuth(
      std::function<bool(const std::string&, const std::string&)> authCB,
      const std::string& realm = "Prometheus-cpp Exporter",
      const std::string& uri = std::string("/metrics"));

  void RemoveCollectable(const std::weak_ptr<Collectable>& collectable,
                         const std::string& uri = std::string("/metrics"));

  std::vector<int> GetListeningPorts() const;

 private:
  detail::Endpoint& GetEndpointForUri(const std::string& uri);

  std::unique_ptr<CivetServer> server_;
  std::vector<std::unique_ptr<detail::Endpoint>> endpoints_;
};

}  // namespace prometheus
