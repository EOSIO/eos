#include <eos/tcp_connection_plugin/tcp_connection_plugin.hpp>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

namespace eos {

tcp_connection_plugin::tcp_connection_plugin() {}

void tcp_connection_plugin::set_program_options( options_description& cli, options_description& cfg ) {
   cfg.add_options()
      ( "listen-endpoint", bpo::value<vector<string>>()->composing(), "Local IP address and port tuples to bind to for incoming connections.")
      ( "remote-endpoint", bpo::value<vector<string>>()->composing(), "IP address and port tuples to connect to remote peers.")
      ;
   }

void tcp_connection_plugin::plugin_initialize(const variables_map& options) {
   ilog("Initialize connection plugin");
   initiator = std::make_unique<tcp_connection_initiator>(options, appbase::app().get_io_service());
}
  
void tcp_connection_plugin::plugin_startup() {
   initiator->go();
}

void tcp_connection_plugin::plugin_shutdown() {
   initiator.reset();
}

}
