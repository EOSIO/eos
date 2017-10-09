
#include <eos/chain/types.hpp>

#include <eos/network_plugin/network_plugin.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/block.hpp>


#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>


namespace eos {
  using std::vector;

  using boost::asio::ip::tcp;
  using boost::asio::ip::address_v4;
  using boost::asio::ip::host_name;

  using fc::time_point;
  using fc::time_point_sec;
  using eos::chain::transaction_id_type;
  namespace bip = boost::interprocess;

  class connection;
  class sync_manager;

  class network_plugin_impl {

  };

  network_plugin::network_plugin()
    :pimpl( new network_plugin_impl ) {
  }

  network_plugin::~network_plugin() {
  }

  void network_plugin::set_program_options( options_description& cli, options_description& cfg )
  {
     /*
    cfg.add_options()
     ( "listen-endpoint", bpo::value<string>()->default_value( "0.0.0.0:9876" ), "The local IP address and port to listen for incoming connections.")
     ( "remote-endpoint", bpo::value< vector<string> >()->composing(), "The IP address and port of a remote peer to sync with.")
     ( "public-endpoint", bpo::value<string>(), "Overrides the advertised listen endpointlisten ip address.")
     ( "agent-name", bpo::value<string>()->default_value("EOS Test Agent"), "The name supplied to identify this node amongst the peers.")
      ( "send-whole-blocks", bpo::value<bool>()->default_value(def_send_whole_blocks), "True to always send full blocks, false to send block summaries" )
      ;
      */
  }

  void network_plugin::plugin_initialize( const variables_map& options ) {
    ilog("Initialize network plugin");

  }

  void network_plugin::plugin_startup() {

  }

  void network_plugin::plugin_shutdown() {
 }

    void network_plugin::broadcast_block( const chain::signed_block &sb) {
      //      dlog( "broadcasting block #${num}",("num",sb.block_num()) );
     
    }
}
