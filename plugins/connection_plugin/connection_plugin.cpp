#include <eos/connection_plugin/connection_plugin.hpp>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

namespace eos {

connection_plugin::connection_plugin() {}

void connection_plugin::set_program_options( options_description& cli, options_description& cfg ) {
  cfg.add_options()
     ( "listen-endpoint", bpo::value<vector<string>>()->composing(), "Local IP address and port tuples to bind to for incoming connections.")
     ( "remote-endpoint", bpo::value<vector<string>>()->composing(), "IP address and port tuples to connect to remote peers.")
     ;
  }

void connection_plugin::plugin_initialize(const variables_map& options) {
  ilog("Initialize connection plugin");
  initiator = std::make_unique<connection_initiator>(options, appbase::app().get_io_service());
}
  
void connection_plugin::plugin_startup() {
  initiator->go();
}

void connection_plugin::plugin_shutdown() {
  initiator.reset();
}

}
