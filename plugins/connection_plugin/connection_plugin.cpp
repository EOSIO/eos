#include <eos/connection_plugin/connection_plugin.hpp>
#include <eos/network_plugin/connection_interface.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>

namespace eos {
   class connection {
   public:
      connection(boost::asio::ip::tcp::socket s) : socket(std::move(s)) {
         ilog("connection created");
      }
  private:
      boost::asio::ip::tcp::socket socket;
   };

   class connection_plugin_impl {
      struct active_acceptor {
        boost::asio::ip::tcp::acceptor acceptor;
        boost::asio::ip::tcp::socket   socket_to_accept;
      };
   public:
      connection_plugin_impl() :
         ios(appbase::app().get_io_service())
      {
         acceptors.emplace_front(active_acceptor{
                                {ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 5555)},
                                boost::asio::ip::tcp::socket(ios)});
         acceptors.front().acceptor.listen();
         acceptors.front().acceptor.async_accept(acceptors.front().socket_to_accept,
                                                 boost::bind(&connection_plugin_impl::accept_connection, this, _1, acceptors.begin()));
      }
   private:
      void accept_connection(boost::system::error_code ec, std::list<active_acceptor>::iterator it) {
         ilog("accept");

         ///XXX create connection object here; move tcp socket in to it
         new connection(std::move(it->socket_to_accept));
        
         //prepare for another connection from same acceptor again
         it->acceptor.async_accept(it->socket_to_accept,
                                   boost::bind(&connection_plugin_impl::accept_connection, this, _1, it));
      }

      std::list<active_acceptor> acceptors;

      boost::asio::io_service& ios;
   };

   connection_plugin::connection_plugin() : pimpl(new connection_plugin_impl) {}

   connection_plugin::~connection_plugin() {
   }

   void connection_plugin::set_program_options( options_description& cli, options_description& cfg )
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

   void connection_plugin::plugin_initialize( const variables_map& options ) {
      ilog("Initialize network plugin");
   }
  
   void connection_plugin::plugin_startup() {

   }

  void connection_plugin::plugin_shutdown() {
  }
}
