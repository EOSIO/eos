#pragma once
#include <b1/rodeos/wasm_ql.hpp>
#include <boost/filesystem.hpp>

namespace b1::rodeos::wasm_ql {

struct http_config {
   uint32_t                               num_threads      = {};
   uint32_t                               max_request_size = {};
   uint64_t                               idle_timeout_ms  = {};
   std::string                            allow_origin     = {};
   std::string                            static_dir       = {};
   std::string                            address          = {};
   std::string                            port             = {};
   std::optional<boost::filesystem::path> checkpoint_dir   = {};
};

struct http_server {
   virtual ~http_server() {}

   static std::shared_ptr<http_server> create(const std::shared_ptr<const http_config>&  http_config,
                                              const std::shared_ptr<const shared_state>& shared_state);

   virtual void stop() = 0;
};

} // namespace b1::rodeos::wasm_ql
