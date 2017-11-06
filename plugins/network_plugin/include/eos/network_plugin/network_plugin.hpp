#pragma once
#include "appbase/application.hpp"
#include "eos/network_plugin/connection_interface.hpp"

namespace eosio {
using namespace appbase;

class network_plugin : public appbase::plugin<network_plugin> {
   public:
      network_plugin();

      APPBASE_PLUGIN_REQUIRES()
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

      /*
       * Get work to send over a connection. After a connection being told to
       * begin_processing_network_send_queue(), it should repeatedly poll for work
       * until connection_send_nowork is returned
       */
      connection_send_work get_work_to_send(connection_send_context& context);

   private:
      std::unique_ptr<class network_plugin_impl> my;
};

struct connection_send_context {
   connection_send_context(std::unique_ptr<connection_interface>& c) : connection(c) {}
   size_t trx_tail;
   std::unique_ptr<connection_interface>& connection;
};

}
