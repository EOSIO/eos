#include <eosio/trace_api/trace_api_plugin.hpp>

#include <eosio/trace_api/abi_data_handler.hpp>
#include <eosio/trace_api/request_handler.hpp>
#include <eosio/trace_api/chain_extraction.hpp>
#include <eosio/trace_api/store_provider.hpp>

#include <eosio/trace_api/configuration_utils.hpp>

#include <boost/signals2/connection.hpp>

using namespace eosio::trace_api;
using namespace eosio::trace_api::configuration_utils;
using boost::signals2::scoped_connection;

namespace {
   appbase::abstract_plugin& plugin_reg = app().register_plugin<trace_api_plugin>();

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
         fc::unhandled_exception ue(
               FC_LOG_MESSAGE(warn, "unknown: ",),
               std::current_exception());
         return ue.to_detail_string();
      }
   }

   void log_exception( const exception_with_context& e, fc::log_level level ) {
      if( _log.is_enabled( level ) ) {
         auto detail_string = to_detail_string(std::get<0>(e));
         auto context = fc::log_context( level, std::get<1>(e), std::get<2>(e), std::get<3>(e) );
         _log.log(fc::log_message( context, detail_string ));
      }
   }

   /**
    * The exception_handler provided to the extraction sub-system throws `yield_exception` as a signal that
    * Something has gone wrong and the extraction process needs to terminate immediately
    *
    * This templated method is used to wrap signal handlers for `chain_controller` so that the plugin-internal
    * `yield_exception` can be translated to a `chain::controller_emit_signal_exception`.
    *
    * The goal is that the currently applied block will be rolled-back before the shutdown takes effect leaving
    * the system in a better state for restart.
    */
   template<typename F>
   void emit_killer(F&& f) {
      try {
         f();
      } catch (const yield_exception& ) {
         EOS_THROW(chain::controller_emit_signal_exception, "Trace API encountered an Error which it cannot recover from.  Please resolve the error and relaunch the process")
      }
   }

   template<typename Store>
   struct shared_store_provider {
      shared_store_provider(const std::shared_ptr<Store>& store)
      :store(store)
      {}

      void append( const block_trace_v1& trace ) {
         store->append(trace);
      }

      void append_lib( uint32_t new_lib ) {
         store->append_lib(new_lib);
      }

      get_block_t get_block(uint32_t height, const yield_function& yield) {
         return store->get_block(height, yield);
      }

      std::shared_ptr<Store> store;
   };
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
      cfg_options("trace-slice-stride", bpo::value<uint32_t>()->default_value(10'000),
                  "the number of blocks each \"slice\" of trace data will contain on the filesystem");
      cfg_options("trace-minimum-irreversible-history-blocks", boost::program_options::value<int32_t>()->default_value(-1),
                  "Number of blocks to ensure are kept past LIB for retrieval before \"slice\" files can be automatically removed.\n"
                  "A value of -1 indicates that automatic removal of \"slice\" files will be turned off.");
      cfg_options("trace-minimum-uncompressed-irreversible-history-blocks", boost::program_options::value<int32_t>()->default_value(-1),
                  "Number of blocks to ensure are uncompressed past LIB. Compressed \"slice\" files are still accessible but may carry a performance loss on retrieval\n"
                  "A value of -1 indicates that automatic compression of \"slice\" files will be turned off.");
   }

   void plugin_initialize(const appbase::variables_map& options) {
      auto dir_option = options.at("trace-dir").as<bfs::path>();
      if (dir_option.is_relative())
         trace_dir = app().data_dir() / dir_option;
      else
         trace_dir = dir_option;

      slice_stride = options.at("trace-slice-stride").as<uint32_t>();

      const int32_t blocks = options.at("trace-minimum-irreversible-history-blocks").as<int32_t>();
      EOS_ASSERT(blocks >= -1, chain::plugin_config_exception,
                 "\"trace-minimum-irreversible-history-blocks\" must be greater to or equal to -1.");
      if (blocks > manual_slice_file_value) {
         minimum_irreversible_history_blocks = blocks;
      }

      const int32_t uncompressed_blocks = options.at("trace-minimum-uncompressed-irreversible-history-blocks").as<int32_t>();
      EOS_ASSERT(uncompressed_blocks >= -1, chain::plugin_config_exception,
                 "\"trace-minimum-uncompressed-irreversible-history-blocks\" must be greater to or equal to -1.");

      if (uncompressed_blocks > manual_slice_file_value) {
         minimum_uncompressed_irreversible_history_blocks = uncompressed_blocks;
      }

      store = std::make_shared<store_provider>(
         trace_dir,
         slice_stride,
         minimum_irreversible_history_blocks,
         minimum_uncompressed_irreversible_history_blocks,
         compression_seek_point_stride
      );
   }

   void plugin_startup() {
      store->start_maintenance_thread([](const std::string& msg ){
         fc_dlog( _log, msg );
      });
   }

   void plugin_shutdown() {
      store->stop_maintenance_thread();
   }

   // common configuration paramters
   boost::filesystem::path trace_dir;
   uint32_t slice_stride = 0;

   std::optional<uint32_t> minimum_irreversible_history_blocks;
   std::optional<uint32_t> minimum_uncompressed_irreversible_history_blocks;

   static constexpr int32_t manual_slice_file_value = -1;
   static constexpr uint32_t compression_seek_point_stride = 6 * 1024 * 1024; // 6 MiB strides for clog seek points

   std::shared_ptr<store_provider> store;
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
                  "ABIs used when decoding trace RPC responses.\n"
                  "There must be at least one ABI specified OR the flag trace-no-abis must be used.\n"
                  "ABIs are specified as \"Key=Value\" pairs in the form <account-name>=<abi-def>\n"
                  "Where <abi-def> can be:\n"
                  "   an absolute path to a file containing a valid JSON-encoded ABI\n"
                  "   a relative path from `data-dir` to a file containing a valid JSON-encoded ABI\n"
                  );
      cfg_options("trace-no-abis",
            "Use to indicate that the RPC responses will not use ABIs.\n"
            "Failure to specify this option when there are no trace-rpc-abi configuations will result in an Error.\n"
            "This option is mutually exclusive with trace-rpc-api"
      );
   }

   void plugin_initialize(const appbase::variables_map& options) {
      std::shared_ptr<abi_data_handler> data_handler = std::make_shared<abi_data_handler>([](const exception_with_context& e){
         log_exception(e, fc::log_level::debug);
      });

      if( options.count("trace-rpc-abi") ) {
         EOS_ASSERT(options.count("trace-no-abis") == 0, chain::plugin_config_exception,
                    "Trace API is configured with ABIs however trace-no-abis is set");
         const std::vector<std::string> key_value_pairs = options["trace-rpc-abi"].as<std::vector<std::string>>();
         for (const auto& entry : key_value_pairs) {
            try {
               auto kv = parse_kv_pairs(entry);
               auto account = chain::name(kv.first);
               auto abi = abi_def_from_file(kv.second, app().data_dir());
               data_handler->add_abi(account, abi);
            } catch (...) {
               elog("Malformed trace-rpc-abi provider: \"${val}\"", ("val", entry));
               throw;
            }
         }
      } else {
         EOS_ASSERT(options.count("trace-no-abis") != 0, chain::plugin_config_exception,
                    "Trace API is not configured with ABIs and trace-no-abis is not set");
      }


      req_handler = std::make_shared<request_handler_t>(
         shared_store_provider<store_provider>(common->store),
         abi_data_handler::shared_provider(data_handler)
      );
   }

   fc::time_point calc_deadline( const fc::microseconds& max_serialization_time ) {
      fc::time_point deadline = fc::time_point::now();
      if( max_serialization_time > fc::microseconds::maximum() - deadline.time_since_epoch() ) {
         deadline = fc::time_point::maximum();
      } else {
         deadline += max_serialization_time;
      }
      return deadline;
   }

   void plugin_startup() {
      auto& http = app().get_plugin<http_plugin>();
      fc::microseconds max_response_time = http.get_max_response_time();

      http.add_async_handler("/v1/trace_api/get_block",
            [wthis=weak_from_this(), max_response_time](std::string, std::string body, url_response_callback cb)
      {
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

            const auto deadline = that->calc_deadline( max_response_time );
            auto resp = that->req_handler->get_block_trace(*block_number, [deadline]() { FC_CHECK_DEADLINE(deadline); });
            if (resp.is_null()) {
               error_results results{404, "Block trace missing"};
               cb( 404, fc::variant( results ));
            } else {
               cb( 200, std::move(resp) );
            }
         } catch (...) {
            http_plugin::handle_exception("trace_api", "get_block", body, cb);
         }
      });
   }

   void plugin_shutdown() {
   }

   std::shared_ptr<trace_api_common_impl> common;

   using request_handler_t = request_handler<shared_store_provider<store_provider>, abi_data_handler::shared_provider>;
   std::shared_ptr<request_handler_t> req_handler;
};

struct trace_api_plugin_impl {
   trace_api_plugin_impl( const std::shared_ptr<trace_api_common_impl>& common )
   :common(common) {}

   static void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
      auto cfg_options = cfg.add_options();
   }

   void plugin_initialize(const appbase::variables_map& options) {
      auto log_exceptions_and_shutdown = [](const exception_with_context& e) {
         log_exception(e, fc::log_level::error);
         app().quit();
         throw yield_exception("shutting down");
      };
      extraction = std::make_shared<chain_extraction_t>(shared_store_provider<store_provider>(common->store), log_exceptions_and_shutdown);

      auto& chain = app().find_plugin<chain_plugin>()->chain();

      applied_transaction_connection.emplace(
         chain.applied_transaction.connect([this](std::tuple<const chain::transaction_trace_ptr&, const chain::packed_transaction_ptr&> t) {
            emit_killer([&](){
               extraction->signal_applied_transaction(std::get<0>(t), std::get<1>(t));
            });
         }));

      accepted_block_connection.emplace(
         chain.accepted_block.connect([this](const chain::block_state_ptr& p) {
            emit_killer([&](){
               extraction->signal_accepted_block(p);
            });
         }));

      irreversible_block_connection.emplace(
         chain.irreversible_block.connect([this](const chain::block_state_ptr& p) {
            emit_killer([&](){
               extraction->signal_irreversible_block(p);
            });
         }));

   }

   void plugin_startup() {
      common->plugin_startup();
   }

   void plugin_shutdown() {
      common->plugin_shutdown();
   }

   std::shared_ptr<trace_api_common_impl> common;

   using chain_extraction_t = chain_extraction_impl_type<shared_store_provider<store_provider>>;
   std::shared_ptr<chain_extraction_t> extraction;

   fc::optional<scoped_connection>                            applied_transaction_connection;
   fc::optional<scoped_connection>                            accepted_block_connection;
   fc::optional<scoped_connection>                            irreversible_block_connection;
};

trace_api_plugin::trace_api_plugin()
{}

trace_api_plugin::~trace_api_plugin()
{}

void trace_api_plugin::set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
   trace_api_common_impl::set_program_options(cli, cfg);
   trace_api_plugin_impl::set_program_options(cli, cfg);
   trace_api_rpc_plugin_impl::set_program_options(cli, cfg);
}

void trace_api_plugin::plugin_initialize(const appbase::variables_map& options) {
   auto common = std::make_shared<trace_api_common_impl>();
   common->plugin_initialize(options);

   my = std::make_shared<trace_api_plugin_impl>(common);
   my->plugin_initialize(options);

   rpc = std::make_shared<trace_api_rpc_plugin_impl>(common);
   rpc->plugin_initialize(options);
}

void trace_api_plugin::plugin_startup() {
   handle_sighup(); // setup logging

   my->plugin_startup();
   rpc->plugin_startup();
}

void trace_api_plugin::plugin_shutdown() {
   my->plugin_shutdown();
   rpc->plugin_shutdown();
}

void trace_api_plugin::handle_sighup() {
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