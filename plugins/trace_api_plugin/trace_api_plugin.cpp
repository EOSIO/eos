#include <eosio/trace_api_plugin/trace_api_plugin.hpp>

#include <eosio/trace_api_plugin/abi_data_handler.hpp>
#include <eosio/trace_api_plugin/request_handler.hpp>

#include <eosio/trace_api_plugin/configuration_utils.hpp>

using namespace eosio::trace_api_plugin;
using namespace eosio::trace_api_plugin::configuration_utils;

namespace {
   const std::string logger_name("trace_api");
   fc::logger _log;

   std::string to_detail_string(const std::exception_ptr& e) {
      try {
         std::rethrow_exception(e);
      } catch (fc::exception& er) {
         return er.to_detail_string();
      } catch (const std::exception& e) {
         fc::exception fce(
               FC_LOG_MESSAGE(warn, "std::exception: ${what}: ", ("what", e.what())),
               fc::std_exception_code,
               BOOST_CORE_TYPEID(e).name(),
               e.what());
         return fce.to_detail_string();
      } catch (...) {
         fc::unhandled_exception e(
               FC_LOG_MESSAGE(warn, "unknown: ",),
               std::current_exception());
         return e.to_detail_string();
      }
   }

   void log_exception( const exception_with_context& e, fc::log_level level ) {
      if( _log.is_enabled( level ) ) {
         auto detail_string = to_detail_string(std::get<0>(e));
         auto context = fc::log_context( level, std::get<1>(e), std::get<2>(e), std::get<3>(e) );
         _log.log(fc::log_message( context, detail_string ));
      }
   }
}

namespace eosio {

/**
 * A common source for information shared between the extraction process and the RPC process
 */
struct trace_api_common_impl {
   static void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
      auto cfg_options = cfg.add_options();
      cfg_options("trace-dir", bpo::value<bfs::path>()->default_value("traces"),
                  "the location of the trace directory (absolute path or relative to application data dir)");
      cfg_options("slice-stride", bpo::value<uint32_t>()->default_value(10'000),
                  "the number of blocks each \"slice\" of trace data will contain on the filesystem");
   }

   void plugin_initialize(const appbase::variables_map& options) {
      auto dir_option = options.at("trace-dir").as<bfs::path>();
      if (dir_option.is_relative())
         trace_dir = app().data_dir() / dir_option;
      else
         trace_dir = dir_option;

      slice_stride = options.at("slice-stride").as<uint32_t>();
   }

   // common configuration paramters
   boost::filesystem::path trace_dir;
   uint32_t slice_stride = 0;
};

/**
 * Interface with the RPC process
 */
struct trace_api_rpc_plugin_impl : public std::enable_shared_from_this<trace_api_rpc_plugin_impl>
{
   trace_api_rpc_plugin_impl( const std::shared_ptr<trace_api_common_impl>& common )
   :common(common) {}

   static void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
      auto cfg_options = cfg.add_options();
      cfg_options("trace-rpc-abi", bpo::value<vector<string>>()->composing(),
                  "ABIs used when decoding trace RPC responses, specified as \"Key=Value\" pairs in the form <account-name>=<abi-def>\n"
                  "Where <abi-def> can be:\n"
                  "   a valid JSON-encoded ABI as a string\n"
                  "   an absolute path to a file containing a valid JSON-encoded ABI\n"
                  "   a relative path from `data-dir` to a file containing a valid JSON-encoded ABI\n"
                  );
   }

   void plugin_initialize(const appbase::variables_map& options) {
      data_handler = std::make_shared<abi_data_handler>([](const exception_with_context& e){
         log_exception(e, fc::log_level::debug);
      });

      if( options.count("trace-rpc-abi") ) {
         const std::vector<std::string> key_value_pairs = options["trace-rpc-abi"].as<std::vector<std::string>>();
         for (const auto& entry : key_value_pairs) {
            try {
               auto kv = parse_kv_pairs(entry);
               auto account = chain::name(kv.first);
               auto abi = abi_def_from_file_or_str(kv.second);
               data_handler->add_abi(account, abi);
            } catch (...) {
               elog("Malformed trace-rpc-abi provider: \"${val}\", ignoring!", ("val", entry));
            }
         }
      }
   }

   void plugin_startup() {
      auto& http = app().get_plugin<http_plugin>();
      http.add_handler("/v1/trace_api/get_block", [wthis=weak_from_this()](std::string, std::string body, url_response_callback cb){
         auto that = wthis.lock();
         if (!that) {
            return;
         }

         auto block_number = ([&body]() -> std::optional<uint32_t> {
            if (body.empty()) {
               return {};
            }

            try {
               auto input = fc::json::from_string(body);
               auto block_num = input.get_object()["block_num"].as_uint64();
               if (block_num > std::numeric_limits<uint32_t>::max()) {
                  return {};
               }
               return block_num;
            } catch (...) {
               return {};
            }
         })();

         if (!block_number) {
            error_results results{400, "Bad or missing block_num"};
            cb( 400, fc::variant( results ));
            return;
         }

         try {

            auto resp = fc::variant(); // connect to response formatter
            if (resp.is_null()) {
               error_results results{404, "Block trace missing"};
               cb( 404, fc::variant( results ));
            } else {
               cb( 200, resp );
            }
         } catch (...) {
            http_plugin::handle_exception("trace_api", "get_block", body, cb);
         }
      });
   }

   void plugin_shutdown() {
   }

   std::shared_ptr<trace_api_common_impl> common;
   std::shared_ptr<abi_data_handler> data_handler;
};

struct trace_api_plugin_impl {
   trace_api_plugin_impl( const std::shared_ptr<trace_api_common_impl>& common )
   :common(common) {}

   static void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
      auto cfg_options = cfg.add_options();
      cfg_options("minimum-irreversible-trace-history", bpo::value<std::string>()->default_value("-1"),
                  "the minimum amount of history, as defined by time, this node will keep after it becomes irreversible\n"
                  "this value can be specified as a number of seconds or using the following suffixes:\n"
                  "  \"s\" : the value is in seconds\n"
                  "  \"m\" : the value is in minutes\n"
                  "  \"h\" : the value is in hours\n"
                  "  \"d\" : the value is in days\n"
                  "  \"w\" : the value is in weeks\n"
                  "a value of \"-1\" will disable automatic maintenance of the trace slice files\n"
                  );
   }

   void plugin_initialize(const appbase::variables_map& options) {
      if( options.count("minimum-irreversible-trace-history") ) {
         auto value = options.at("minimum-irreversible-trace-history").as<std::string>();
         try {
            minimum_irreversible_trace_history = parse_microseconds(value);
         } catch (...) {
            elog("Malformed minimum-irreversible-trace-history: \"${val}\"!", ("val", value));
            throw;
         }
      }
   }

   void plugin_startup() {
   }

   void plugin_shutdown() {
   }

   std::shared_ptr<trace_api_common_impl> common;
   fc::microseconds minimum_irreversible_trace_history = fc::microseconds::maximum();
};

_trace_api_plugin::_trace_api_plugin()
{}

_trace_api_plugin::~_trace_api_plugin()
{}

void _trace_api_plugin::set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
   trace_api_common_impl::set_program_options(cli, cfg);
   trace_api_plugin_impl::set_program_options(cli, cfg);
   trace_api_rpc_plugin_impl::set_program_options(cli, cfg);
}

void _trace_api_plugin::plugin_initialize(const appbase::variables_map& options) {
   auto common = std::make_shared<trace_api_common_impl>();
   common->plugin_initialize(options);

   my = std::make_shared<trace_api_plugin_impl>(common);
   my->plugin_initialize(options);

   rpc = std::make_shared<trace_api_rpc_plugin_impl>(common);
   rpc->plugin_initialize(options);
}

void _trace_api_plugin::plugin_startup() {
   my->plugin_startup();
   rpc->plugin_startup();
}

void _trace_api_plugin::plugin_shutdown() {
   my->plugin_shutdown();
   rpc->plugin_shutdown();
}

void _trace_api_plugin::handle_sighup() {
   fc::logger::update( logger_name, _log );
}

trace_api_rpc_plugin::trace_api_rpc_plugin()
{}

trace_api_rpc_plugin::~trace_api_rpc_plugin()
{}

void trace_api_rpc_plugin::set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
   trace_api_common_impl::set_program_options(cli, cfg);
   trace_api_rpc_plugin_impl::set_program_options(cli, cfg);
}

void trace_api_rpc_plugin::plugin_initialize(const appbase::variables_map& options) {
   auto common = std::make_shared<trace_api_common_impl>();
   common->plugin_initialize(options);

   rpc = std::make_shared<trace_api_rpc_plugin_impl>(common);
   rpc->plugin_initialize(options);
}

void trace_api_rpc_plugin::plugin_startup() {
   rpc->plugin_startup();
}

void trace_api_rpc_plugin::plugin_shutdown() {
   rpc->plugin_shutdown();
}

void trace_api_rpc_plugin::handle_sighup() {
   fc::logger::update( logger_name, _log );
}

}