#pragma once
#include <appbase/application.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/network_plugin/network_plugin.hpp>
#include <eos/tcp_connection_plugin/tcp_connection_initiator.hpp>

namespace eos {
using namespace appbase;

class tcp_connection_plugin : public appbase::plugin<tcp_connection_plugin> {
public:
   tcp_connection_plugin();

   APPBASE_PLUGIN_REQUIRES((network_plugin))
   void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<tcp_connection_initiator> initiator;
};

}
