#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

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

   private:
      std::shared_ptr<struct trace_api_rpc_plugin_impl> rpc;
   };
}