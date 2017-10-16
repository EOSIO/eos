#pragma once

/*
 Simply responsible for handling all incoming connections and successful outgoing
 connections. Outgoing connections retried on failure.
*/

#include <eos/tcp_connection_plugin/tcp_connection.hpp>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/program_options/variables_map.hpp>

namespace eos {

class tcp_connection_initiator {
public:
  tcp_connection_initiator(const boost::program_options::variables_map& options, boost::asio::io_service& ios);

   //starts the incoming/outgoing connections
   void go();

private:
   ///////incoming connections
   struct active_acceptor : private boost::noncopyable {
      active_acceptor(boost::asio::io_service& ios, boost::asio::ip::tcp::endpoint ep) :
         acceptor(ios, ep), socket_to_accept(ios) {}

      boost::asio::ip::tcp::acceptor acceptor;
      boost::asio::ip::tcp::socket   socket_to_accept;
    };

    std::list<active_acceptor> acceptors;
    void accept_connection(active_acceptor& aa);
    void handle_incoming_connection(const boost::system::error_code& ec, active_acceptor& aa);

    //////outgoing connections
    struct outgoing_attempt : private boost::noncopyable {
      outgoing_attempt(boost::asio::io_service& ios, boost::asio::ip::tcp::resolver::query q) : 
      remote_resolver_query(q), reconnect_timer(ios), outgoing_socket(ios) {}

      boost::asio::ip::tcp::resolver::query remote_resolver_query;
      boost::asio::steady_timer       reconnect_timer;
      boost::asio::ip::tcp::socket    outgoing_socket;      //only used during conn. estab., then moved to connection object
    };
    std::list<outgoing_attempt> outgoing_connections;
    void retry_connection(outgoing_attempt& outgoing);
    void outgoing_resolve_complete(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator iterator, outgoing_attempt& outgoing);
    void outgoing_connection_complete(const boost::system::error_code& ec, outgoing_attempt& outgoing);
    void outgoing_failed_retry_inabit(outgoing_attempt& outgoing);

    //////keep a handle to all current connections and the signals2::connection that indicate their failure
    //Note: Actually, this class has no need to hang on to the connection. But until network_manager holds
    //      on to a reference _someone_ needs to hold on to a reference! Refactor once network_manager holds
    //      a list of the connections.
    struct active_connection : private boost::noncopyable {
      active_connection(tcp_connection_ptr s) : connection(s) {}

      tcp_connection_ptr  connection;
      boost::signals2::connection  connection_failure_connection;
    };
    std::list<active_connection> active_connections;
    boost::asio::strand strand;  //serializes accesses to active_connections

    boost::asio::ip::tcp::resolver resolver;

    boost::asio::io_service& ios;
};

}