#include <eos/tcp_connection_plugin/tcp_connection_initiator.hpp>
#include <eos/network_plugin/network_plugin.hpp>

#include <fc/log/logger.hpp>

#include <boost/algorithm/string/erase.hpp>

namespace eosio {

static void parse_ip_port(const std::string& in, std::string& ip, std::string& port) {
   size_t last_colon = in.find_last_of(':');

   //(naively) handle the case user didn't specify port; we'll not touch the passed port parameter
   if(last_colon == std::string::npos)
      ip = in;
   else if(last_colon && in.find('[') != std::string::npos && in.find_last_of(']') != last_colon-1)
      ip = in;
   else {
      ip = in.substr(0, last_colon);
      port = in.substr(last_colon+1);
   }
   boost::erase_all(ip, "[");
   boost::erase_all(ip, "]");
}

tcp_connection_initiator::tcp_connection_initiator(const boost::program_options::variables_map& options, boost::asio::io_service& i) :
   _strand(i), _resolver(i), _ios(i) {

   if(options.count("listen-endpoint")) {
      for(const std::string& bindaddr : options["listen-endpoint"].as<std::vector<std::string>>()) {
         std::string ip;
         std::string port{"9876"};
         parse_ip_port(bindaddr, ip, port);

         boost::asio::ip::tcp::resolver::query query(ip, port);

         try {
            boost::asio::ip::tcp::resolver::iterator addresses = _resolver.resolve(query);
            _acceptors.emplace_front(_ios, addresses->endpoint());
         }
         catch(boost::system::system_error sr) {
            elog("Failed to listen on an incoming port-- ${sr}", ("sr", sr.what()));
         }
      }
   }

   if(options.count("remote-endpoint")) {
      for(const std::string& bindaddr : options["remote-endpoint"].as<std::vector<std::string>>()) {
         std::string ip;
         std::string port{"9876"};
         parse_ip_port(bindaddr, ip, port);

         _outgoing_connections.emplace_front(_ios, boost::asio::ip::tcp::resolver::query(ip, port));
      }
   }

   if(_acceptors.empty())
      wlog("Not listening on any TCP address/ports for incoming connections");
   if(_outgoing_connections.empty())
      wlog("Not attempting any outgoing TCP connections");
}

void tcp_connection_initiator::go() {
   for(active_acceptor& aa : _acceptors) {
      aa.acceptor.listen();
      accept_connection(aa);
   }

   for(outgoing_attempt& oa : _outgoing_connections)
      retry_connection(oa);
}

void tcp_connection_initiator::accept_connection(active_acceptor& aa) {
   aa.acceptor.async_accept(aa.socket_to_accept, _strand.wrap([this,&aa](auto ec) {
      handle_incoming_connection(ec, aa);
   }));
}

 void tcp_connection_initiator::handle_incoming_connection(const boost::system::error_code& ec, active_acceptor& aa) {
   std::unique_ptr<tcp_connection> new_conn = std::make_unique<tcp_connection>(std::move(aa.socket_to_accept));
   appbase::app().get_plugin<network_plugin>().new_connection(std::move(new_conn));
  
   accept_connection(aa);
}
void tcp_connection_initiator::retry_connection(outgoing_attempt& outgoing) {
   outgoing.outgoing_socket = boost::asio::ip::tcp::socket(_ios);

   _resolver.async_resolve(outgoing.remote_resolver_query, _strand.wrap([this, &outgoing](auto ec, auto i) {
      outgoing_resolve_complete(ec, i, outgoing);
   }));
}

void tcp_connection_initiator::outgoing_resolve_complete(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator iterator, outgoing_attempt& outgoing) {
   if(ec)
      outgoing_failed_retry_inabit(outgoing);
   else
      boost::asio::async_connect(outgoing.outgoing_socket, iterator, _strand.wrap([this,&outgoing](auto ec, auto i) {
         outgoing_connection_complete(ec, outgoing);
      }));
}

void tcp_connection_initiator::outgoing_connection_complete(const boost::system::error_code& ec, outgoing_attempt& outgoing) {
   if(ec)
      outgoing_failed_retry_inabit(outgoing);
   else {
      std::unique_ptr<tcp_connection> new_conn = std::make_unique<tcp_connection>(std::move(outgoing.outgoing_socket));

      new_conn->on_disconnected(_strand.wrap([this, &outgoing]() {
        retry_connection(outgoing);
      }));

      appbase::app().get_plugin<network_plugin>().new_connection(std::move(new_conn));
    }
}

void tcp_connection_initiator::outgoing_failed_retry_inabit(outgoing_attempt& outgoing) {
  outgoing.reconnect_timer.expires_from_now(std::chrono::seconds(3));
  outgoing.reconnect_timer.async_wait(_strand.wrap([this,&outgoing](auto ec){
     if(ec)
        throw std::runtime_error("Unexpected timer wait failure");
     retry_connection(outgoing);
  }));
}

}