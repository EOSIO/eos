#include "wasm_ql_plugin.hpp"
#include "wasm_ql_http.hpp"

#include <b1/rodeos/wasm_ql.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

using namespace appbase;
using namespace b1::rodeos;
using namespace std::literals;

using wasm_ql_plugin = b1::wasm_ql_plugin;

static abstract_plugin& _wasm_ql_plugin = app().register_plugin<wasm_ql_plugin>();

namespace b1 {

struct wasm_ql_plugin_impl : std::enable_shared_from_this<wasm_ql_plugin_impl> {
   bool                                         stopping     = false;
   uint32_t                                     max_retries  = 0xffff'ffff;
   uint32_t                                     retried      = 0;
   std::shared_ptr<const wasm_ql::http_config>  http_config  = {};
   std::shared_ptr<const wasm_ql::shared_state> shared_state = {};
   std::shared_ptr<wasm_ql::http_server>        http_server  = {};
   boost::asio::deadline_timer                  timer;

   wasm_ql_plugin_impl() : timer(app().get_io_service()) {}

   void start_http() {
      http_server = wasm_ql::http_server::create(http_config, shared_state);
      if (!http_server)
         schedule_retry();
   }

   void schedule_retry() {
      if (retried++ < max_retries) {
         timer.expires_from_now(boost::posix_time::seconds(1));
         timer.async_wait([this](auto&) {
            ilog("retry...");
            try {
               try {
                  start_http();
               }
               FC_LOG_AND_RETHROW()
            } catch (...) {
               elog("shutting down");
               app().quit();
            }
         });
      } else {
         elog("hit --wql-retries limit; shutting down");
         app().quit();
      }
   }

   void shutdown() {
      stopping = true;
      timer.cancel();
      if (http_server)
         http_server->stop();
   }
}; // wasm_ql_plugin_impl

} // namespace b1

wasm_ql_plugin::wasm_ql_plugin() : my(std::make_shared<wasm_ql_plugin_impl>()) {}

wasm_ql_plugin::~wasm_ql_plugin() {
   if (my->stopping)
      ilog("wasm_ql_plugin stopped");
}

void wasm_ql_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();
   op("wql-threads", bpo::value<int>()->default_value(8), "Number of threads to process requests");
   op("wql-listen", bpo::value<std::string>()->default_value("127.0.0.1:8880"), "Endpoint to listen on");
   op("wql-unix-listen", bpo::value<std::string>(), "Unix socket path to listen on");
   op("wql-retries", bpo::value<uint32_t>()->default_value(0xffff'ffff),
      "Number of times to retry binding to --wql-listen. Each retry is approx 1 second apart. Set to 0 to prevent "
      "retries.");
   op("wql-allow-origin", bpo::value<std::string>(), "Access-Control-Allow-Origin header. Use \"*\" to allow any.");
   op("wql-contract-dir", bpo::value<std::string>(),
      "Directory to fetch contracts from. These override contracts on the chain. (default: disabled)");
   op("wql-static-dir", bpo::value<std::string>(), "Directory to serve static files from (default: disabled)");
   op("wql-query-mem", bpo::value<uint32_t>()->default_value(33), "Maximum size of wasm memory (MiB)");
   op("wql-console-size", bpo::value<uint32_t>()->default_value(0), "Maximum size of console data");
   op("wql-wasm-cache-size", bpo::value<uint32_t>()->default_value(100), "Maximum number of compiled wasms to cache");
   op("wql-max-request-size", bpo::value<uint32_t>()->default_value(10000), "HTTP maximum request body size (bytes)");
   op("wql-idle-timeout", bpo::value<uint64_t>()->default_value(std::numeric_limits<uint32_t>::max()), "HTTP idle connection timeout");
   op("wql-exec-time", bpo::value<uint64_t>()->default_value(200), "Max query execution time (ms)");
   op("wql-checkpoint-dir", bpo::value<boost::filesystem::path>(),
      "Directory to place checkpoints. Caution: this allows anyone to create a checkpoint using RPC (default: "
      "disabled)");
   op("wql-max-action-return-value", bpo::value<uint32_t>()->default_value(MAX_SIZE_OF_BYTE_ARRAYS), "Max action return value size (bytes)");
}

void wasm_ql_plugin::plugin_initialize(const variables_map& options) {
   try {
      auto http_config  = std::make_shared<wasm_ql::http_config>();
      auto shared_state = std::make_shared<wasm_ql::shared_state>(app().find_plugin<rocksdb_plugin>()->get_db());
      my->http_config   = http_config;
      my->shared_state  = shared_state;

      my->max_retries                = options.at("wql-retries").as<uint32_t>();
      http_config->num_threads       = options.at("wql-threads").as<int>();

      auto ip_port = options.at("wql-listen").as<std::string>();
      if(ip_port.size()) {
         if (ip_port.find(':') == std::string::npos)
            throw std::runtime_error("invalid --wql-listen value: " + ip_port);
         http_config->port              = ip_port.substr(ip_port.find(':') + 1, ip_port.size());
         http_config->address           = ip_port.substr(0, ip_port.find(':'));
      }
      if(options.count("wql-unix-listen"))
         http_config->unix_path = options.at("wql-unix-listen").as<std::string>();

      shared_state->max_pages        = options.at("wql-query-mem").as<uint32_t>() * 16;
      shared_state->max_console_size = options.at("wql-console-size").as<uint32_t>();
      shared_state->wasm_cache_size  = options.at("wql-wasm-cache-size").as<uint32_t>();
      http_config->max_request_size  = options.at("wql-max-request-size").as<uint32_t>();
      http_config->idle_timeout_ms   = std::chrono::milliseconds( options.at("wql-idle-timeout").as<uint64_t>() );
      shared_state->max_exec_time_ms = options.at("wql-exec-time").as<uint64_t>();
      shared_state->max_action_return_value_size = options.at("wql-max-action-return-value").as<uint32_t>();
      if (options.count("wql-contract-dir"))
         shared_state->contract_dir = options.at("wql-contract-dir").as<std::string>();
      if (options.count("wql-allow-origin"))
         http_config->allow_origin = options.at("wql-allow-origin").as<std::string>();
      if (options.count("wql-static-dir"))
         http_config->static_dir = options.at("wql-static-dir").as<std::string>();
      if (options.count("wql-checkpoint-dir")) {
         auto path = options.at("wql-checkpoint-dir").as<boost::filesystem::path>();
         if (path.is_relative())
            http_config->checkpoint_dir = app().data_dir() / path;
         else
            http_config->checkpoint_dir = path;
         boost::filesystem::create_directories(*http_config->checkpoint_dir);
      }
   }
   FC_LOG_AND_RETHROW()
}

void wasm_ql_plugin::plugin_startup() { my->start_http(); }
void wasm_ql_plugin::plugin_shutdown() { my->shutdown(); }
