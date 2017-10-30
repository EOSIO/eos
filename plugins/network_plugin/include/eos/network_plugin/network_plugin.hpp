#pragma once
#include "appbase/application.hpp"
#include "eos/chain_plugin/chain_plugin.hpp"
#include "eos/network_plugin/connection_interface.hpp"

namespace eosio {
using namespace appbase;

class network_plugin : public appbase::plugin<network_plugin> {
   public:
      network_plugin();

      APPBASE_PLUGIN_REQUIRES((eos::chain_plugin))
      void set_program_options(options_description& cli, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

      void indicate_about_to_shutdown();
      boost::asio::io_service& network_io_service();

      /*
       * Use a new connection. Only connections that are connected and otherwise valid
       * should be passed to network_plugin. Connections will continue to be used until
       * they indicate on_disconnected()
       */
      void new_connection(std::unique_ptr<connection_interface> new_connection);

   private:
      std::unique_ptr<class network_plugin_impl> my;
   };

}
