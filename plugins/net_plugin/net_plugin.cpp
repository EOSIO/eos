#include <eos/chain/protocol/types.hpp>

#include <eos/net_plugin/net_plugin.hpp>
#include <eos/net_plugin/protocol.hpp>

#include <fc/network/ip.hpp>
#include <fc/io/raw.hpp>
#include <fc/container/flat.hpp>
#include <fc/reflect/variant.hpp>

#include <boost/asio/ip/tcp.hpp>

namespace eos {
   using std::vector;
   using boost::asio::ip::tcp;
   using fc::time_point;
   using fc::time_point_sec;
   using eos::chain::transaction_id_type;
   namespace bip = boost::interprocess;

   using socket_ptr = std::shared_ptr<tcp::socket>;

struct node_transaction_state {
   transaction_id_type id;
   fc::time_point      received;
   fc::time_point_sec  expires;
   vector<char>        packed_transaction;
   uint64_t            block_num = -1; /// block transaction was included in
   bool                validated = false; /// whether or not our node has validated it
};


/**
 *  Index by id
 *  Index by is_known, block_num, validated_time, this is the order we will broadcast
 *  to peer.
 *  Index by is_noticed, validated_time
 *  
 */
struct transaction_state {
   transaction_id_type id;
   bool                is_known_by_peer = false; ///< true if we sent or received this trx to this peer or received notice from peer
   bool                is_noticed_to_peer = false; ///< have we sent peer noitce we know it (true if we reeive from this peer)
   uint64_t            block_num = -1; ///< the block number the transaction was included in
   time_point          validated_time; ///< infinity for unvalidated
   time_point          requested_time; /// incase we fetch large trx
};

typedef multi_index_container<
   transaction_state,
   indexed_by<
      ordered_unique< tag<by_id>, member<transaction_state, transaction_id_type, &transaction_state::id > >
   >
> transaction_state_index;

/**
 * 
 */
struct block_state {
   block_id_type id;
   bool          is_known;
   bool          is_noticed_to_peer;
   time_point    requested_time;
};

typedef multi_index_container<
   block_state,
   indexed_by<
      ordered_unique< tag<by_id>, member<block_state, block_id_type, &block_state::id > >
   >
> block_state_index;

/**
 * Index by start_block
 */
struct sync_state {
   uint64_t     start_block = 0;
   uint64_t     end_block = 0;
   uint64_t     last = 0; ///< last sent or received
   time_point   start_time;; ///< time request made or received
};

struct by_start_block;
typedef multi_index_container<
   sync_state,
   indexed_by<
      ordered_unique< tag<by_start_block>, member<sync_state, uint64_t, &sync_state::start_block > >
   >
> sync_request_index;

class connection {
   public:
      connection( socket_ptr s ):socket(s){
         wlog( "created connection" );
         pending_message_buffer.resize( 1024*1024*4 );
      }
      ~connection() {
         wlog( "released connection" );
      }

      block_state_index              block_state;
      transaction_state_index        trx_state;
      sync_request_index             in_sync_state;
      sync_request_index             out_sync_state;
      socket_ptr                     socket;

      uint32_t                       pending_message_size;
      vector<char>                   pending_message_buffer;

      handshake_message              last_handshake;

      std::deque<net_message>            out_queue;

      void send( const net_message& m ) {
         out_queue.push_back( m );
         if( out_queue.size() == 1 )
            send_next_message();
      }

      void send_next_message() {
         if( !out_queue.size() ) 
            return;

         auto& m = out_queue.front();

         vector<char> buffer;
         uint32_t size = fc::raw::pack_size( m );
         buffer.resize(  size + sizeof(uint32_t) );
         fc::datastream<char*> ds( buffer.data(), buffer.size() );
         ds.write( (char*)&size, sizeof(size) );
         fc::raw::pack( ds, m );

         boost::asio::async_write( *socket, boost::asio::buffer( buffer.data(), buffer.size() ),
            [this,buf=std::move(buffer)]( boost::system::error_code ec, std::size_t bytes_transferred ) {
               ilog( "write message handler..." );
               if( ec ) {
                  elog( "Error sending message: ${msg}", ("msg",ec.message() ) );
               } else  {
                  out_queue.pop_front();
                  send_next_message();
               }
         });
      }
};



class net_plugin_impl {
   public:
   unique_ptr<tcp::acceptor> acceptor;

   tcp::endpoint listen_endpoint;
   tcp::endpoint public_endpoint;

   vector<string> seed_nodes;

   std::set<socket_ptr>          pending_connections;
   std::set< connection* >       connections;
   bool                          done = false;

   void connect( const string& ep ) {
     auto host = ep.substr( 0, ep.find(':') );
     auto port = ep.substr( host.size()+1, host.size() );
     idump((host)(port));
     auto resolver = std::make_shared<tcp::resolver>( std::ref( app().get_io_service() ) ); 
     tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );

     resolver->async_resolve( query, 
       [resolver,ep,this]( const boost::system::error_code& err, tcp::resolver::iterator endpoint_itr ){
         if( !err ) {
           connect( resolver, endpoint_itr );
         } else {
           elog( "Unable to resolve ${ep}: ${error}", ( "ep", ep )("error", err.message() ) );
         }
     });
   }

   void connect( std::shared_ptr<tcp::resolver> resolver, tcp::resolver::iterator endpoint_itr ) {
     auto sock = std::make_shared<tcp::socket>( std::ref( app().get_io_service() ) );
     pending_connections.insert( sock );

     auto current_endpoint = *endpoint_itr;
     ++endpoint_itr;
     sock->async_connect( current_endpoint,
        [sock,resolver,endpoint_itr,this]( const boost::system::error_code& err ) {
           if( !err ) {
              pending_connections.erase( sock );
              start_session( new connection( sock ) );
           } else {
              if( endpoint_itr != tcp::resolver::iterator() ) {
                connect( resolver, endpoint_itr );
              }
           }
        }
     );
   }

   /**
    * This thread performs high level coordination among multiple connections and
    * ensures connections are cleaned up, reconnected, etc.
    */
   void network_loop() {
   try {
      ilog( "starting network loop" );
      while( !done ) {
         for( auto itr = connections.begin(); itr != connections.end(); ) {
            auto con = *itr;
            if( !con->socket->is_open() )  {
               close(con);
               itr = connections.begin();
               continue;
            }
            ++itr;
         }
      }
      ilog("network loop done");
   } FC_CAPTURE_AND_RETHROW() }

   void start_session( connection* con ) {
     connections.insert( con );
     start_read_message( *con );
     con->send( handshake_message{} );
     // con->readloop_complete  = bf::async( [=](){ read_loop( con ); } );
     // con->writeloop_complete = bf::async( [=](){ write_loop( con ); } );
   }

   void start_listen_loop() {
      auto socket = std::make_shared<tcp::socket>( std::ref( app().get_io_service() ) );
      acceptor->async_accept( *socket, [socket,this]( boost::system::error_code ec ) {
         if( !ec ) {
            start_session( new connection( socket ) );
            start_listen_loop();
         } else {
            elog( "Error accepting connection: ${m}", ("m", ec.message() ) );
         }
      });
   }

   void start_read_message( connection& c ) {
      c.pending_message_size = 0;
      boost::asio::async_read( *c.socket, boost::asio::buffer((char*)&c.pending_message_size,sizeof(c.pending_message_size)), 
        [&]( boost::system::error_code ec, std::size_t bytes_transferred ) {
           ilog( "read size handler..." );
           if( !ec ) {
              if( c.pending_message_size <= c.pending_message_buffer.size() ) {
                start_reading_pending_buffer( c );
                return;
              } else {
                elog( "Received a message that was too big" );
              }
           } else {
            elog( "Error reading message from connection: ${m}", ("m", ec.message() ) );
           }
           close( &c );
        }
      );
   }
   void start_reading_pending_buffer( connection& c ) {
      boost::asio::async_read( *c.socket, boost::asio::buffer(c.pending_message_buffer.data(), c.pending_message_size ), 
        [&]( boost::system::error_code ec, std::size_t bytes_transferred ) {
           ilog( "read buffer handler..." );
           if( !ec ) {
            try {
               auto msg = fc::raw::unpack<net_message>( c.pending_message_buffer );
               ilog( "received message of size: ${s}", ("s",bytes_transferred) );
               start_read_message( c );
               return;
            } catch ( const fc::exception& e ) {
              edump((e.to_detail_string() ));
            }
           } else {
            elog( "Error reading message from connection: ${m}", ("m", ec.message() ) );
           }
           close( &c );
        }
      );
   }


   void write_loop( connection* c ) {
      try {
         c->send( handshake_message{} );
      } catch ( ... ) {
         wlog( "write loop exception" );
      }
   }

   void close( connection* c ) {
      ilog( "close ${c}", ("c",int64_t(c)));
      if( c->socket )
         c->socket->close();
      connections.erase( c );
      delete c;
   }

};

net_plugin::net_plugin()
:my( new net_plugin_impl ) {
}

net_plugin::~net_plugin() {
}

void net_plugin::set_program_options( options_description& cli, options_description& cfg ) 
{
   cfg.add_options()
         ("listen-endpoint", bpo::value<string>()->default_value( "127.0.0.1:9876" ), "The local IP address and port to listen for incoming connections.")
         ("remote-endpoint", bpo::value< vector<string> >()->composing(), "The IP address and port of a remote peer to sync with.")
         ("public-endpoint", bpo::value<string>()->default_value( "0.0.0.0:9876" ), "The public IP address and port that should be advertized to peers.")
         ;
}

void net_plugin::plugin_initialize( const variables_map& options ) {
   ilog("Initialize net plugin");
   if( options.count( "listen-endpoint" ) ) {
      auto lipstr = options.at("listen-endpoint").as< string >();
      auto fcep   = fc::ip::endpoint::from_string( lipstr );
      my->listen_endpoint = tcp::endpoint( boost::asio::ip::address_v4::from_string( (string)fcep.get_address() ), fcep.port() );
      
      ilog( "configured net to listen on ${ep}", ("ep", fcep) );

      my->acceptor.reset( new tcp::acceptor( app().get_io_service() ) );
   }
   if( options.count( "remote-endpoint" ) ) {
      my->seed_nodes = options.at( "remote-endpoint" ).as< vector<string> >();
   }
}

void net_plugin::plugin_startup() {
  // boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
  if( my->acceptor ) {
      my->acceptor->open(my->listen_endpoint.protocol());
      my->acceptor->set_option(tcp::acceptor::reuse_address(true));
      my->acceptor->bind(my->listen_endpoint);
      my->acceptor->listen();

      my->start_listen_loop();
  }

  for( auto seed_node : my->seed_nodes ) {
     my->connect( seed_node );
  }
}

void net_plugin::plugin_shutdown() {
try {
   ilog( "shutdown.." );
   my->done = true;
   if( my->acceptor ) {
      ilog( "close acceptor" );
      my->acceptor->close();

      ilog( "close connections ${s}", ("s",my->connections.size()) );
      auto cons = my->connections;
      for( auto con : cons ) 
         con->socket->close();

      while( my->connections.size() ) {
         auto c = *my->connections.begin();
         my->close( c );
      }

      idump((my->connections.size()));
      my->acceptor.reset(nullptr);
   }
   ilog( "exit shutdown" );
} FC_CAPTURE_AND_RETHROW() }

}
