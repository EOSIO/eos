#pragma once

#include <functional>
#include <memory>
#include <string>

#include "CivetServer.h"
#include "basic_auth.h"
#include "prometheus/collectable.h"
#include "prometheus/registry.h"

namespace prometheus {
namespace detail {
class MetricsHandler;

class Endpoint {
 public:
  explicit Endpoint(CivetServer& server, std::string uri);
  ~Endpoint();

  void RegisterCollectable(const std::weak_ptr<Collectable>& collectable);
  void RegisterAuth(
      std::function<bool(const std::string&, const std::string&)> authCB,
      const std::string& realm);
  void RemoveCollectable(const std::weak_ptr<Collectable>& collectable);

  const std::string& GetURI() const;

 private:
  CivetServer& server_;
  const std::string uri_;
  // registry for "meta" metrics about the endpoint itself
  std::shared_ptr<Registry> endpoint_registry_;
  std::unique_ptr<MetricsHandler> metrics_handler_;
  std::unique_ptr<BasicAuthHandler> auth_handler_;
};

}  // namespace detail
}  // namespace prometheus
