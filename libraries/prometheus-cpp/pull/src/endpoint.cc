#include "endpoint.h"

#include <utility>

#include "basic_auth.h"
#include "handler.h"
#include "prometheus/detail/future_std.h"

namespace prometheus {
namespace detail {

namespace {
class AlwaysAllowAccessHandler : public CivetAuthHandler {
  bool authorize(CivetServer*, struct mg_connection*) override { return true; }
};

AlwaysAllowAccessHandler alwaysAllowAccessHandler;
}  // namespace

Endpoint::Endpoint(CivetServer& server, std::string uri)
    : server_(server),
      uri_(std::move(uri)),
      endpoint_registry_(std::make_shared<Registry>()),
      metrics_handler_(
          detail::make_unique<MetricsHandler>(*endpoint_registry_)) {
  RegisterCollectable(endpoint_registry_);
  server_.addHandler(uri_, metrics_handler_.get());
}

Endpoint::~Endpoint() {
  server_.removeHandler(uri_);
  if (auth_handler_) {
    // work-around https://github.com/civetweb/civetweb/issues/941
    // server_.removeAuthHandler(uri_);
    server_.addAuthHandler(uri_, alwaysAllowAccessHandler);
  }
}

void Endpoint::RegisterCollectable(
    const std::weak_ptr<Collectable>& collectable) {
  metrics_handler_->RegisterCollectable(collectable);
}

void Endpoint::RegisterAuth(
    std::function<bool(const std::string&, const std::string&)> authCB,
    const std::string& realm) {
  // split creating, assigning, and storing to avoid a race-condition when
  // being called the second time and the handler is replaced
  auto new_handler =
      detail::make_unique<BasicAuthHandler>(std::move(authCB), realm);
  server_.addAuthHandler(uri_, new_handler.get());
  auth_handler_ = std::move(new_handler);
}

void Endpoint::RemoveCollectable(
    const std::weak_ptr<Collectable>& collectable) {
  metrics_handler_->RemoveCollectable(collectable);
}

const std::string& Endpoint::GetURI() const { return uri_; }

}  // namespace detail
}  // namespace prometheus
