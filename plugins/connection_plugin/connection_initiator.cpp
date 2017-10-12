#include <eos/connection_plugin/connection_initiator.hpp>

#include <fc/log/logger.hpp>

namespace eos {

connection_initiator::connection_initiator(const boost::program_options::variables_map& options, boost::asio::io_service& i) :
  strand(i), ios(i) {

   //TODO: Create these lists based on command line arguments, for now, hardcode test values

   try {
      acceptors.emplace_front(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 5556));
   }
   catch(boost::system::system_error sr) {
      elog("Failed to listen on an incoming port-- ${sr}", ("sr", sr.what()));
   }

   outgoing_connections.emplace_front(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 6666));
}

void connection_initiator::go() {
   for(active_acceptor& aa : acceptors) {
      aa.acceptor.listen();
      accept_connection(aa);
   }

   for(outgoing_attempt& oa : outgoing_connections)
      retry_connection(oa);
}

void connection_initiator::accept_connection(active_acceptor& aa) {
   aa.acceptor.async_accept(aa.socket_to_accept, strand.wrap(boost::bind(&connection_initiator::handle_incoming_connection, this, _1, boost::ref(aa))));
}

 void connection_initiator::handle_incoming_connection(const boost::system::error_code& ec, active_acceptor& aa) {
   ilog("accepted connection");
  
   //XX verify on some sort of acceptable incoming IP range
   active_connections.emplace_front(std::make_shared<connection>((std::move(aa.socket_to_accept))));
   active_connections.front().connection_failure_connection = active_connections.front().connection->connect_on_disconnected(strand.wrap([this,it=active_connections.begin()]() {
      it->connection_failure_connection.disconnect();
      active_connections.erase(it);
    }));
  
   accept_connection(aa);
}
void connection_initiator::retry_connection(outgoing_attempt& outgoing) {
   outgoing.outgoing_socket = boost::asio::ip::tcp::socket(outgoing.reconnect_timer.get_io_service());
   outgoing.outgoing_socket.async_connect(outgoing.remote_endpoint, strand.wrap(boost::bind(&connection_initiator::outgoing_connection_complete, this, _1, boost::ref(outgoing))));
}

void connection_initiator::outgoing_connection_complete(const boost::system::error_code& ec, outgoing_attempt& outgoing) {
   if(ec) {
      ilog("connection failed");
      outgoing.reconnect_timer.expires_from_now(std::chrono::seconds(3));
      outgoing.reconnect_timer.async_wait(boost::bind(&connection_initiator::retry_connection, this, boost::ref(outgoing)));
   }
   else {
      ilog("Connection good!");
      outgoing.reconnect_timer.cancel();
      active_connections.emplace_front(std::make_shared<connection>(std::move(outgoing.outgoing_socket)));
      active_connections.front().connection_failure_connection = active_connections.front().connection->connect_on_disconnected(strand.wrap([this,it=active_connections.begin(),&outgoing]() {
         it->connection_failure_connection.disconnect();
         active_connections.erase(it);
         retry_connection(outgoing);
       }));
     }
 }

}