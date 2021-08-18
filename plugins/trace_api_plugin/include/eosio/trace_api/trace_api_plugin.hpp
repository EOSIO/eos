#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/trace_api/trace.hpp>

namespace eosio {
   /**
    * Plugin that runs both a data extraction  and the HTTP RPC in the same application
    */
   class trace_api_plugin : public appbase::plugin<trace_api_plugin> {
   public:
      APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_plugin))

      trace_api_plugin();
      virtual ~trace_api_plugin();

      virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;

      void plugin_initialize(const appbase::variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

      void handle_sighup() override;

   private:
      std::shared_ptr<struct trace_api_plugin_impl>     my;
      std::shared_ptr<struct trace_api_rpc_plugin_impl> rpc;
   };

   typedef shared_ptr<struct trace_api_rpc_plugin_impl> trace_api_rpc_ptr;

   namespace trace_apis {

      class read_only {

          trace_api_rpc_ptr trace_api_rpc;

      public:
         read_only(trace_api_rpc_ptr&& trace_api_rpc): trace_api_rpc(trace_api_rpc) {}

         struct get_transaction_params {
            string                                id;
            std::optional<uint32_t>               block_num_hint;
         };

         trace_api::transaction_trace_v3 get_transaction( const get_transaction_params& ) const;

      };
   } // namespace trace_apis
   /**
    * Plugin that only runs the RPC
    */
   class trace_api_rpc_plugin : public appbase::plugin<trace_api_rpc_plugin> {
   public:
      APPBASE_PLUGIN_REQUIRES((http_plugin))

      trace_api_rpc_plugin();
      virtual ~trace_api_rpc_plugin();

      virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;

      void plugin_initialize(const appbase::variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

      void handle_sighup() override;

      trace_apis::read_only  get_read_only_api()const { return trace_apis::read_only(trace_api_rpc_ptr(rpc)); }

   private:
      std::shared_ptr<struct trace_api_rpc_plugin_impl> rpc;
   };

}

FC_REFLECT( eosio::trace_apis::read_only::get_transaction_params, (id)(block_num_hint) )
