#include <eosio/bnet_plugin/bnet_plugin.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>


#include <fc/io/json.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>

#include <eosio/chain/plugin_interface.hpp>

using tcp = boost::asio::ip::tcp;
namespace ws  = boost::beast::websocket;

namespace eosio { 
   using namespace chain;

   static appbase::abstract_plugin& _bnet_plugin = app().register_plugin<bnet_plugin>();

} /// namespace eosio

using eosio::public_key_type;
using eosio::chain_id_type;
using eosio::block_id_type;
using eosio::block_timestamp_type;
using std::string;
using eosio::sha256;
using eosio::signed_block_ptr;
using eosio::packed_transaction_ptr;
using std::vector;

struct hello {
   public_key_type               peer_id;
   string                        network_version;
   string                        agent;
   string                        user;
   string                        password;
   chain_id_type                 chain_id;
   bool                          request_transactions = false;
   uint32_t                      last_irr_block_num = 0;
   vector<block_id_type>         pending_block_ids;
};
FC_REFLECT( hello, (peer_id)(network_version)(user)(password)(agent)(chain_id)(request_transactions)(last_irr_block_num)(pending_block_ids) )


/**
 * This message is sent upon successful speculative application of a transaction
 * and informs a peer not to send this message.
 */
struct trx_notice { 
   sha256                signed_trx_id; ///< hash of trx + sigs 
   fc::time_point_sec    expiration; ///< expiration of trx
};

FC_REFLECT( trx_notice, (signed_trx_id)(expiration) )

/**
 *  This message is sent upon successfully adding a transaction to the fork database 
 *  and informs the remote peer that there is no need to send this block.
 */
struct block_notice { 
   block_id_type block_id;
};

FC_REFLECT( block_notice, (block_id) );

struct ping {
   fc::time_point sent;
   fc::sha256     code;
};
FC_REFLECT( ping, (sent)(code) )

struct pong {
   fc::time_point sent;
   fc::sha256     code;
};
FC_REFLECT( pong, (sent)(code) )

using bnet_message = fc::static_variant<hello,
                                        trx_notice,
                                        block_notice,
                                        signed_block_ptr,
                                        packed_transaction_ptr,
                                        ping, pong
                                        >;


struct by_id;
struct by_num;
struct by_received;
struct by_expired;

namespace eosio {
  using namespace chain::plugin_interface;

  class bnet_plugin_impl;

  /**
   *  Each session is presumed to operate in its own strand so that
   *  operations can execute in parallel. 
   */
  class session : public std::enable_shared_from_this<session>
  {
     public:
        enum session_state {
           hello_state,
           sending_state,
           idle_state
        };

        struct block_status {
           block_status( block_id_type i, bool kby_peer, bool rfrom_peer)
           {
              known_by_peer      = kby_peer;
              received_from_peer = rfrom_peer;
              id = i;
           }

           bool known_by_peer  = false;         ///< we sent block to peer or peer sent us notice
           bool received_from_peer = false;     ///< peer sent us this block and considers full block valid
           block_id_type id;                    ///< the block id;
           block_id_type prev;                  ///< the prev block id

           shared_ptr< vector<char> >  block_msg; ///< packed bnet_message for this block

           uint32_t block_num()const { return block_header::num_from_id(id); }
        };

        typedef std::shared_ptr<block_status> block_status_ptr;


        typedef boost::multi_index_container<block_status,
           indexed_by<
              ordered_unique< tag<by_id>,  member<block_status,block_id_type,&block_status::id> >,
              ordered_non_unique< tag<by_num>,  const_mem_fun<block_status,uint32_t,&block_status::block_num> >
           >
        > block_status_index;


        struct transaction_status {
           time_point                 received;
           time_point                 expired; /// 5 seconds from last accepted
           transaction_id_type        id;
           transaction_metadata_ptr   trx;

           void mark_known_by_peer() { received = fc::time_point::maximum();  }
           bool known_by_peer()const { return received == fc::time_point::maximum(); }
        };

        typedef boost::multi_index_container<transaction_status,
           indexed_by<
              ordered_unique< tag<by_id>,  member<transaction_status,transaction_id_type,&transaction_status::id> >,
              ordered_non_unique< tag<by_received>,  member<transaction_status,time_point,&transaction_status::received> >,
              ordered_non_unique< tag<by_expired>,  member<transaction_status,time_point,&transaction_status::expired> >
           >
        > transaction_status_index;

        auto get_status( block_id_type id ) {
           return _block_status.find( id );
        }

        block_status_index        _block_status; 
        transaction_status_index  _transaction_status; 

        public_key_type    _peer_id;
        public_key_type    _remote_peer_id;
        uint32_t           _local_lib             = 0;
        block_id_type      _local_lib_id;
        uint32_t           _local_head_block_num  = 0;
        block_id_type      _local_head_block_id; /// the last block id received on local channel

        bool               _remote_request_trx    = false;
        uint32_t           _remote_lib            = 0;
        uint32_t           _last_sent_block_num   = 0;
        block_id_type      _last_sent_block_id; /// the id of the last block sent
        bool               _recv_remote_hello     = false;
        bool               _sent_remote_hello     = false;


        fc::sha256         _current_code;
        ping               _last_recv_ping;
        ping               _last_sent_ping;
 

        int                                                            _session_num = 0;
        session_state                                                  _state = hello_state;
        tcp::resolver                                                  _resolver;
        bnet_plugin_impl&                                              _net_plugin;
        boost::asio::io_service&                                       _ios;
        unique_ptr<ws::stream<tcp::socket>>                            _ws;
        boost::asio::strand< boost::asio::io_context::executor_type>   _strand;
        boost::asio::io_service&                                       _app_ios;
       
        methods::get_block_by_number::method_type& _get_block_by_number;


        string                                                         _peer;
        string                                                         _remote_host;
        string                                                         _remote_port;

        vector<char>                                                   _out_buffer;
        //boost::beast::multi_buffer                                     _in_buffer;
        boost::beast::flat_buffer                                     _in_buffer;

        int next_session_id()const {
           static int session_count = 0;
           return ++session_count;
        }

        /**
         * Creating session from server socket acceptance
         */
        explicit session( tcp::socket socket, bnet_plugin_impl& net_plug )
        :_resolver(socket.get_io_service()),
         _net_plugin( net_plug ),
         _ios(socket.get_io_service()),
         _ws( new ws::stream<tcp::socket>(move(socket)) ), 
         _strand(_ws->get_executor() ),
         _app_ios( app().get_io_service() ),
         _get_block_by_number( app().get_method<methods::get_block_by_number>() )
        { 
            _session_num = next_session_id();
            _ws->binary(true);

           wlog( "open session ${n}",("n",_session_num) );
        }


        /**
         * Creating outgoing session
         */
        explicit session( boost::asio::io_context& ioc, bnet_plugin_impl& net_plug )
        :_resolver(ioc),
         _net_plugin( net_plug ),
         _ios(ioc),
         _ws( new ws::stream<tcp::socket>(ioc) ), 
         _strand( _ws->get_executor() ),
         _app_ios( app().get_io_service() ),
         _get_block_by_number( app().get_method<methods::get_block_by_number>() )
        { 
           _session_num = next_session_id();
           _ws->binary(true);

           wlog( "open session ${n}",("n",_session_num) );
        }

        ~session();

        void run() {
           _ws->async_accept( boost::asio::bind_executor( 
                             _strand,
                             std::bind( &session::on_accept,
                                        shared_from_this(),
                                        std::placeholders::_1) ) );
        }

        void run( const string& peer ) {
           auto c = peer.find(':');
           auto host = peer.substr( 0, c );
           auto port = peer.substr( c+1, peer.size() );

          _peer = peer;
          _remote_host = host;
          _remote_port = port;

          _resolver.async_resolve( _remote_host, _remote_port, 
                                   std::bind( &session::on_resolve,
                                              shared_from_this(),
                                              std::placeholders::_1,
                                              std::placeholders::_2 ) );
        }

        void on_resolve( boost::system::error_code ec,
                         tcp::resolver::results_type results ) {
           if( ec ) return on_fail( ec, "resolve" );

           boost::asio::async_connect( _ws->next_layer(),
                                       results.begin(), results.end(),
                                       boost::asio::bind_executor( _strand,
                                          std::bind( &session::on_connect,
                                                     shared_from_this(),
                                                     std::placeholders::_1 ) ) );
        }

        void on_connect( boost::system::error_code ec ) {
           if( ec ) return on_fail( ec, "connect" );

           _ws->async_handshake( _remote_host, "/",
                                boost::asio::bind_executor( _strand,
                                std::bind( &session::on_handshake,
                                           shared_from_this(),
                                           std::placeholders::_1 ) ) );
        }

        void on_handshake( boost::system::error_code ec ) {
           if( ec ) return on_fail( ec, "handshake" );
           do_hello();
           do_read();
        }

        /**
         *  This will be called "every time" a the transaction is accepted which happens
         *  on the speculative block (potentially several such blocks) and when a block 
         *  that contains the transaction is applied and/or when switching forks. 
         *
         *  We will add it to the transaction status table as "received now" to be the
         *  basis of sending it to the peer. When we send it to the peer "received now"
         *  will be set to the infinite future to mark it as sent so we don't resend it
         *  when it is accepted again. 
         *
         *  Each time the transaction is "accepted" we extend the time we cache it by
         *  5 seconds from now.  Every time a block is applied we purge all accepted 
         *  transactions that have reached 5 seconds without a new "acceptance". 
         */
        void on_accepted_transaction( transaction_metadata_ptr t ) {
           //ilog( "accepted ${t}", ("t",t->id) );
           auto itr = _transaction_status.find( t->id );
           if( itr != _transaction_status.end() ) {
              if( !itr->known_by_peer() ) { 
                 _transaction_status.modify( itr, [&]( auto& stat ) {
                    stat.expired = std::min<fc::time_point>( fc::time_point::now() + fc::seconds(5), t->trx.expiration );
                 });
              }
              return;
           } 

           transaction_status stat;
           stat.received = fc::time_point::now();
           stat.expired  = stat.received + fc::seconds(5);
           stat.trx      = t;
           stat.id       = t->id;
           _transaction_status.insert( stat );

           do_send_next_message(); 
        }

        /**
         * Remove all transactions that expired from cache prior to now
         */
        void purge_transaction_cache() {
           auto& idx = _transaction_status.get<by_expired>();
           auto itr = idx.begin();
           auto now = fc::time_point::now();
           while( itr != idx.end() && itr->expired < now ) {
              idx.erase(itr);
              itr = idx.begin();
           }
        }

        /**
         *  When our local LIB advances we can purge our known history up to
         *  the LIB or up to the last block known by the remote peer. 
         */
        void on_new_lib( block_state_ptr s ) {
           if( !_strand.running_in_this_thread() ) { elog( "wrong strand" ); }
           _local_lib = s->block_num;
           _local_lib_id = s->id;

           auto purge_to = std::min( _local_lib, _last_sent_block_num );

           auto& idx = _block_status.get<by_num>();
           auto itr = idx.begin();
           while( itr != idx.end() && itr->block_num() < purge_to ) {
              idx.erase(itr);
              itr = idx.begin();
           }
           do_send_next_message();
        }


        void on_bad_block( signed_block_ptr b ) {
           if( !_strand.running_in_this_thread() ) { elog( "wrong strand" ); }
           try {
              auto id = b->id();
              auto itr = _block_status.find( id );
              if( itr == _block_status.end() ) return;
              if( itr->received_from_peer ) {
                 elog( "peer sent bad block #${b} ${i}, disconnect", ("b", b->block_num())("i",b->id())  );
                 _ws->next_layer().close();
              }
           } catch ( ... ) {
              elog( "uncaught exception" );
           }
        }

        void on_accepted_block( block_id_type id, block_id_type prev, shared_ptr<vector<char>> msgdata, block_state_ptr s ) {
           //ilog( "accepted block ${n}", ("n",s->block_num) );
           if( !_strand.running_in_this_thread() ) { elog( "wrong strand" ); }

           auto itr = _block_status.find( id );
           if( itr == _block_status.end() ) {
              itr = _block_status.insert( block_status(id, false, false) ).first;
           }
           
           _block_status.modify( itr, [&]( auto& item ) {
              item.block_msg = msgdata;
              item.prev = prev;
           });

           _local_head_block_id = id;
           _local_head_block_num = block_header::num_from_id(id);

           purge_transaction_cache();

           /** purge all transactions from cache, I will send them as part of a block
            * in the future unless peer tells me they already have block.
            */
           for( const auto& receipt : s->block->transactions ) {
              if( receipt.trx.which() == 1 ) {
                 auto id = receipt.trx.get<packed_transaction>().id();
                 auto itr = _transaction_status.find( id );
                 if( itr != _transaction_status.end() )
                    _transaction_status.erase(itr);
              }
           }

           do_send_next_message(); /// attempt to send if we are idle
        }


        template<typename L>
        void async_get_pending_block_ids( L&& callback ) {
           /// send peer my head block status which is read from chain plugin
           _app_ios.post( [self = shared_from_this(),callback]{
              auto& control = app().get_plugin<chain_plugin>().chain();
              auto lib = control.last_irreversible_block_num();
              auto head = control.head_block_id();
              auto head_num = block_header::num_from_id(head);


              std::vector<block_id_type> ids;
              if( lib > 0 ) {
                 ids.reserve((head_num-lib)+1);
                 for( auto i = lib; i <= head_num; ++i ) {
                   ids.emplace_back(control.get_block_id_for_num(i));
                 }
              }
              self->_ios.post( boost::asio::bind_executor(
                                    self->_strand,
                                    [callback,ids,lib](){
                                       callback(ids,lib);
                                    }
                                ));
           });
        }

        template<typename L>
        void async_get_block_num( uint32_t blocknum, L&& callback ) {
           _app_ios.post( [self = shared_from_this(), blocknum, callback]{
              auto& control = app().get_plugin<chain_plugin>().chain();
              try {
                 //ilog( "fetch block ${n}", ("n",blocknum) );
                 auto sblockptr = control.fetch_block_by_number( blocknum );

                 self->_ios.post( boost::asio::bind_executor(
                                       self->_strand,
                                       [callback,sblockptr](){
                                          callback(sblockptr);
                                       }
                                   ));

              } catch ( const fc::exception& e ) {
                 edump((e.to_detail_string()));
              }
           });
        }

        void do_hello();

        void send( const shared_ptr<vector<char> >& buffer ) {
           try {
           if( !_strand.running_in_this_thread() ) { elog( "wrong strand" ); }
          //    wdump((_out_buffer.size())(buffer->size()));
              FC_ASSERT( !_out_buffer.size() );
              _out_buffer.resize(buffer->size());
              memcpy( _out_buffer.data(), buffer->data(), _out_buffer.size() );

              _state = sending_state;
              // wlog( "state = sending" );
              _ws->async_write( boost::asio::buffer(_out_buffer), 
                                boost::asio::bind_executor( 
                                   _strand,
                                   std::bind( &session::on_write,
                                             shared_from_this(),
                                             std::placeholders::_1,
                                             std::placeholders::_2 ) ) );
           } FC_LOG_AND_RETHROW() 
        }


        void send( const bnet_message& msg ) { try {

           if( !_strand.running_in_this_thread() ) { elog( "wrong strand" ); }
           FC_ASSERT( !_out_buffer.size() );

           auto ps = fc::raw::pack_size(msg);
           _out_buffer.resize(ps);
       //    wdump((_out_buffer.size()));
           fc::datastream<char*> ds(_out_buffer.data(), ps);
           fc::raw::pack(ds, msg);

           _state = sending_state;
     //      wlog( "state = sending" );
           _ws->async_write( boost::asio::buffer(_out_buffer), 
                             boost::asio::bind_executor( 
                                _strand,
                               std::bind( &session::on_write,
                                          shared_from_this(),
                                          std::placeholders::_1,
                                          std::placeholders::_2 ) ) );
        } FC_LOG_AND_RETHROW() }

        void mark_block_known_by_peer( block_id_type id) {
            auto itr = _block_status.find(id);
            if( itr == _block_status.end() ) {
               _block_status.insert( block_status(id, true, false) );
            } else {
               _block_status.modify( itr, [&]( auto& item ) {
                 item.known_by_peer = true;
               });
            }
        }
        void mark_block_recv_from_peer( block_id_type id ) {
            auto itr = _block_status.find(id);
            if( itr == _block_status.end() ) {
               _block_status.insert( block_status(id, true, true) );
            } else {
               _block_status.modify( itr, [&]( auto& item ) {
                 item.known_by_peer = true;
                 item.received_from_peer = true;
               });
            }
        }

        void do_send_next_message() {
           if( !_strand.running_in_this_thread() ) { elog( "wrong strand" ); }
           if( _state == sending_state ) return; /// in process of sending
           if( _out_buffer.size() ) return;
           if( !_recv_remote_hello || !_sent_remote_hello ) return;

           clear_expired_trx();

           if( send_pong() ) return;
           if( send_ping() ) return;
           if( send_next_block() ) return;
           if( send_next_trx() ) return;
        }

        bool send_pong() {
           if( _last_recv_ping.code == fc::sha256() ) 
              return false;

           send( pong{ fc::time_point::now(), _last_recv_ping.code } );
           _last_recv_ping.code = fc::sha256();
           return true;
        }

        bool send_ping() {

           if( fc::time_point::now() - _last_sent_ping.sent > fc::seconds(6) ) {

              if( _last_sent_ping.code != fc::sha256() ) {
                 do_goodbye( "attempt to send another ping before receiving pong" );
                 return true;
              }

              _last_sent_ping.sent = fc::time_point::now();
              _last_sent_ping.code = fc::sha256::hash(_last_sent_ping.sent); /// TODO: make this more random
              send( _last_sent_ping );
              return true;
           } 


           return false;
        }

        bool is_known_by_peer( block_id_type id ) {
           auto itr = _block_status.find(id);
           if( itr == _block_status.end() ) return false;
           return itr->known_by_peer;
        }

        void clear_expired_trx() {
           auto& idx = _transaction_status.get<by_expired>();
           auto itr = idx.begin();
           while( itr != idx.end() && itr->expired < fc::time_point::now() ) {
              idx.erase(itr);
              itr = idx.begin();
           }
        }

        bool send_next_trx() { try {
           if( !_remote_request_trx  ) return false;

           auto& idx = _transaction_status.get<by_received>();
           auto start = idx.begin();
           if( start == idx.end() || start->known_by_peer() )
              return false;

           idx.modify( start, [&]( auto& stat ) {
              stat.mark_known_by_peer();
           });

           auto ptrx_ptr = std::make_shared<packed_transaction>( start->trx->packed_trx );
           // wlog("sending trx ${id}", ("id",start->id) );
           send(ptrx_ptr);

           return true;

        } FC_LOG_AND_RETHROW() }

        bool send_next_block() {
           if( _last_sent_block_num > _local_head_block_num ) {
              _last_sent_block_num  = _local_lib;
              _last_sent_block_id  = _local_lib_id;
           }

           if( _last_sent_block_num == _local_head_block_num )
              return false;

           _state = sending_state;
           async_get_block_num( _last_sent_block_num + 1, 
                [self=shared_from_this()]( auto sblockptr ) {
                  if( !sblockptr )  {
                     self->_state = idle_state;
                     self->do_send_next_message();
                     return;
                  }

                  auto prev = sblockptr->previous;
                  auto next = sblockptr->id();

                  bool peer_knows_prev = self->_last_sent_block_id == prev; /// shortcut
                  peer_knows_prev |= self->_last_sent_block_num <= self->_local_lib;
                  if( !peer_knows_prev ) 
                     peer_knows_prev = self->is_known_by_peer( prev );

                  if( peer_knows_prev ) {
                     self->_last_sent_block_id  = next;
                     self->_last_sent_block_num = block_header::num_from_id(next);

                     if( !self->is_known_by_peer( next ) ) {
                        self->mark_block_known_by_peer( next );
                        self->mark_block_transactions_known_by_peer( sblockptr );

                        //ilog( "send block ${n}", ("n", sblockptr->block_num()) );
                        self->send( sblockptr );
                        return;
                     } else {
                        self->_state = idle_state;
                        self->do_send_next_message();
                     }
                  } else { 
                     wlog( "looks like we had a fork... " );
                     self->_state = idle_state;
                     /// we must have forked... peer doesn't know about previous,
                     /// we need to find the most recent block the peer does know about
                     self->_last_sent_block_num = self->_local_lib;
                     self->_last_sent_block_id  = self->_local_lib_id;
                     if( self->is_known_by_peer( self->_local_lib_id ) )
                        self->send_next_block();
                     else
                        self->do_goodbye( "I don't know that you know my lib, please reconnect" );
                  }
                }
           );
           return true;
        }

        void on_fail( boost::system::error_code ec, const char* what ) {
           elog( "${w}: ${m}", ("w", what)("m", ec.message() ) );
           _ws->next_layer().close();
        }

        void on_accept( boost::system::error_code ec ) {
           if( ec ) {
              return on_fail( ec, "accept" );
           }

           do_hello();
           do_read();
        }

        void do_read() {
           _ws->async_read( _in_buffer, 
                           boost::asio::bind_executor( 
                              _strand,
                              std::bind( &session::on_read,
                                         shared_from_this(),
                                         std::placeholders::_1,
                                         std::placeholders::_2)));
        }

        void on_read( boost::system::error_code ec, std::size_t bytes_transferred ) {
           boost::ignore_unused(bytes_transferred);

           if( ec == ws::error::closed ) 
              return on_fail( ec, "close on read" );

           if( ec ) {
              return on_fail( ec, "read" );;
           }
           //wdump((_in_buffer.size()));
           //auto str = boost::beast::buffers_to_string(_in_buffer.data());
           //fc::datastream<const char*> ds(str.data(), str.size());

           //idump((_in_buffer.size()));
           try {
              auto d = boost::asio::buffer_cast<char const*>(boost::beast::buffers_front(_in_buffer.data()));
              auto s = boost::asio::buffer_size(_in_buffer.data());
              fc::datastream<const char*> ds(d,s);

              bnet_message msg;
              fc::raw::unpack( ds, msg );
              _in_buffer.consume( ds.tellp() );
              on_message( msg ); 

              wait_on_app();

           } catch ( ... ) {
              wlog( "close bad payload" );
              _ws->close( boost::beast::websocket::close_code::bad_payload );
           }
        }

        /** if we just call do_read here then this thread might run ahead of
         * the main thread, instead we post an event to main which will then 
         * post a new read event when ready.
         *
         * This also keeps the "shared pointer" alive in the callback preventing
         * the connection from being closed.
         */
        void wait_on_app() {
            app().get_io_service().post( [self=shared_from_this()]{ self->do_read(); });
        }

        void on_message( const bnet_message& msg ) {
           try {
              switch( msg.which() ) {
                 case bnet_message::tag<hello>::value:
                    on( msg.get<hello>() );
                    break;
                 case bnet_message::tag<signed_block_ptr>::value:
                    on( msg.get<signed_block_ptr>() );
                    break;
                 case bnet_message::tag<packed_transaction_ptr>::value:
                    on( msg.get<packed_transaction_ptr>() );
                    break;
                 case bnet_message::tag<ping>::value:
                    on( msg.get<ping>() );
                    break;
                 case bnet_message::tag<pong>::value:
                    on( msg.get<pong>() );
                    break;
                 default:
                    wlog( "bad message received" );
                    _ws->close( boost::beast::websocket::close_code::bad_payload );
                    return;
              }
           } catch( const fc::exception& e ) {
              elog( "${e}", ("e",e.to_detail_string()));
              _ws->close( boost::beast::websocket::close_code::bad_payload );
           }
        }


        void on( const ping& p ) {
           _last_recv_ping = p;
        }

        void on( const pong& p ) {
           if( p.code != _last_sent_ping.code ) {
              return do_goodbye( "invalid pong code" );
           }
           if( fc::time_point::now() - _last_sent_ping.sent > fc::seconds(3) ) {
              return do_goodbye( "pong response too slow" );
           }
           _last_sent_ping.code = fc::sha256();
        }

        void do_goodbye( const string& reason ) {
           wlog( "${reason}", ("reason",reason) );
           _ws->next_layer().close();
        }

        void on( const hello& hi ) {
           _recv_remote_hello     = true;

           if( hi.chain_id != chain_id_type() ) {
              return do_goodbye( "disconnecting due to wrong chain id" );
           }

           if( hi.peer_id == _peer_id ) {
              return do_goodbye( "connected to self" );
           }

           _last_sent_block_num   = hi.last_irr_block_num;
           _remote_request_trx    = hi.request_transactions;
           _remote_peer_id        = hi.peer_id;
           
           for( const auto& id : hi.pending_block_ids )
              mark_block_known_by_peer( id );

           check_for_redundant_connection();
        }
        void check_for_redundant_connection();

        void on( const block_notice& stat ) {
           mark_block_known_by_peer( stat.block_id );
        }

        void on( const signed_block_ptr& b ) {
           //ilog( "recv block ${n}", ("n", b->block_num()) );
           auto id = b->id();
           mark_block_recv_from_peer( id );

           app().get_channel<incoming::channels::block>().publish(b);

           mark_block_transactions_known_by_peer( b );
        }

        void mark_block_transactions_known_by_peer( const signed_block_ptr& b ) {
           for( const auto& receipt : b->transactions ) {
              if( receipt.trx.which() == 1 ) {
                 auto id = receipt.trx.get<packed_transaction>().id();
                 mark_transaction_known_by_peer(id);
              }
           }
        }

        /**
         * @return true if trx is known by local host, false if new to this host
         */
        bool mark_transaction_known_by_peer( const transaction_id_type& id ) {
           auto itr = _transaction_status.find( id );
           if( itr != _transaction_status.end() ) {
              _transaction_status.modify( itr, [&]( auto& stat ) {
                 stat.mark_known_by_peer();
              });
              return true;
           } else {
              transaction_status stat;
              stat.id = id;
              stat.mark_known_by_peer();
              stat.expired = fc::time_point::now()+fc::seconds(5);
              _transaction_status.insert(stat);
           }
           return false;
        }

        void on( const packed_transaction_ptr& p ) {
           auto id = p->id();
          // ilog( "recv trx ${n}", ("n", id) );
           if( p->expiration() < fc::time_point::now() ) return;

           if( mark_transaction_known_by_peer( id ) )
              return;

           app().get_channel<incoming::channels::transaction>().publish(p);
        }

        void on_write( boost::system::error_code ec, std::size_t bytes_transferred ) {
           boost::ignore_unused(bytes_transferred);
           if( !_strand.running_in_this_thread() ) { elog( "wrong strand" ); }
           if( ec ) {
              _ws->next_layer().close();
              return on_fail( ec, "write" );
           }
           _state = idle_state;
           _out_buffer.resize(0);
           do_send_next_message();
        }
  };


  /**
   *  Accepts incoming connections and launches the sessions
   */
  class listener : public std::enable_shared_from_this<listener> {
     private:
        tcp::acceptor         _acceptor;
        tcp::socket           _socket;
        bnet_plugin_impl&     _net_plugin;

     public:
        listener( boost::asio::io_context& ioc, tcp::endpoint endpoint, bnet_plugin_impl& np  )
        :_acceptor(ioc), _socket(ioc),_net_plugin(np)
        {
           boost::system::error_code ec;

           _acceptor.open( endpoint.protocol(), ec );
           if( ec ) { on_fail( ec, "open" ); return; }

           _acceptor.set_option( boost::asio::socket_base::reuse_address(true) );

           _acceptor.bind( endpoint, ec );
           if( ec ) { on_fail( ec, "bind" ); return; }

           _acceptor.listen( boost::asio::socket_base::max_listen_connections, ec );
           if( ec ) on_fail( ec, "listen" );
        }

        void run() {
           FC_ASSERT( _acceptor.is_open(), "unable top open listen socket" );
           do_accept();
        }

        void do_accept() {
           _acceptor.async_accept( _socket, [self=shared_from_this()]( auto ec ){ self->on_accept(ec); } );
        }

        void on_fail( boost::system::error_code ec, const char* what ) {
           elog( "${w}: ${m}", ("w", what)("m", ec.message() ) );
        }

        void on_accept( boost::system::error_code ec );
  };


   class bnet_plugin_impl {
      public:
         bnet_plugin_impl() {
            _peer_pk = fc::crypto::private_key::generate();
            _peer_id = _peer_pk.get_public_key();
         }

         string                                         _bnet_endpoint_address = "0.0.0.0";
         uint16_t                                       _bnet_endpoint_port = 4321;
         bool                                           _request_trx = true;
         public_key_type                                _peer_id;
         private_key_type                               _peer_pk; /// one time random key to identify this process
                                                        

         std::vector<std::string>                       _connect_to_peers; /// list of peers to connect to
         std::vector<std::thread>                       _socket_threads;
         int32_t                                        _num_threads = 1;
         std::unique_ptr<boost::asio::io_context>       _ioc;
         std::shared_ptr<listener>                      _listener;
         std::shared_ptr<boost::asio::deadline_timer>   _timer;

         std::map<const session*, std::weak_ptr<session> > _sessions;

         channels::irreversible_block::channel_type::handle  _on_irb_handle;
         channels::accepted_block::channel_type::handle      _on_accepted_block_handle;
         channels::rejected_block::channel_type::handle      _on_bad_block_handle;
         channels::accepted_transaction::channel_type::handle _on_appled_trx_handle;

         void async_add_session( std::weak_ptr<session> wp ) {
            app().get_io_service().post( [wp,this]{
               ilog( "add session" );
               if( auto l = wp.lock() ) {
                  _sessions[l.get()] = wp;
               }
            });
         }

         void on_session_close( const session* s ) {
            auto itr = _sessions.find(s);
            if( _sessions.end() != itr ) 
               _sessions.erase(itr);
         }

         template<typename Call>
         void for_each_session( Call callback ) {
            for( const auto& item : _sessions ) {
               if( auto ses = item.second.lock() ) {
                  ses->_ios.post( boost::asio::bind_executor(
                                        ses->_strand,
                                        [ses,cb=callback](){ cb(ses); }
                                    ));
               } 
            }
         }

         void on_accepted_transaction( transaction_metadata_ptr trx ) {
            if( trx->trx.signatures.size() == 0 ) return;
            for_each_session( [trx]( auto ses ){ ses->on_accepted_transaction( trx ); } );
         }

         /**
          * Notify all active connection of the new irreversible block so they
          * can purge their block cache
          */
         void on_irreversible_block( block_state_ptr s ) {
            for_each_session( [s]( auto ses ){ ses->on_new_lib( s ); } );
         }

         /**
          * Notify all active connections of the new accepted block so
          * they can relay it. This method also pre-packages the block
          * as a packed bnet_message so the connections can simply relay
          * it on.
          */
         void on_accepted_block( block_state_ptr s ) {
            _ioc->post( [s,this] { /// post this to the thread pool because packing can be intensive
               bnet_message msg(s->block);
               auto msgdata = std::make_shared<vector<char>>( fc::raw::pack(msg) );
               for_each_session( [msgdata,s]( auto ses ){ ses->on_accepted_block( s->id, s->header.previous, msgdata, s ); } );
            });
         }

         /**
          * We received a bad block which either
          * 1. didn't link to known chain
          * 2. violated the consensus rules
          *
          * Any peer which sent us this block (not noticed) 
          * should be disconnected as they are objectively bad
          */
         void on_bad_block( signed_block_ptr s ) {
            for_each_session( [s]( auto ses ) { ses->on_bad_block(s); } );
         };

         void on_reconnect_peers() {
             for( const auto& peer : _connect_to_peers ) {
                bool found = false;
                for( const auto& con : _sessions ) {
                   auto ses = con.second.lock();
                   if( ses && (ses->_peer == peer) ) {
                      found = true;
                      break;
                   }
                }

                if( !found ) {
                   wlog( "attempt to connect to ${p}", ("p",peer) );
                   auto s = std::make_shared<session>( *_ioc, std::ref(*this) );
                   s->_peer_id = _peer_id;
                   _sessions[s.get()] = s;
                   s->run( peer );
                }
             }

             start_reconnect_timer();
         }


         void start_reconnect_timer() {
            /// add some random delay so that all my peers don't attempt to reconnect to me 
            /// at the same time after shutting down.. 
            _timer->expires_from_now( boost::posix_time::microseconds( 1000000*(10+rand()%5) ) );
            _timer->async_wait([=](const boost::system::error_code& ec) {
                if( ec ) { return; }
                on_reconnect_peers();
            });
         }
   };


   void listener::on_accept( boost::system::error_code ec ) {
     if( ec ) {
        return;
     }
     auto newsession = std::make_shared<session>( move( _socket ), std::ref(_net_plugin) );
     _net_plugin.async_add_session( newsession );
     newsession->_peer_id = _net_plugin._peer_id;
     newsession->run();
     do_accept();
   }


   bnet_plugin::bnet_plugin()
   :my(std::make_shared<bnet_plugin_impl>()) {
   }

   bnet_plugin::~bnet_plugin() {
   }

   void bnet_plugin::set_program_options(options_description& cli, options_description& cfg) {
      cfg.add_options()
         ("bnet_endpoint", bpo::value<string>()->default_value("0.0.0.0:4321"), "the endpoint upon which to listen for incoming connections" )
         ("bnet_threads", bpo::value<uint32_t>(), "the number of threads to use to process network messages" )
         ("bnet_connect", bpo::value<vector<string>>()->composing(), "the endpoint upon which to listen for incoming connections" )
         ("bnet_no_trx", bpo::bool_switch()->default_value(false), "this peer will request no pending transactions from other nodes" )
         ;
   }

   void bnet_plugin::plugin_initialize(const variables_map& options) {
      ilog( "Initialize bnet plugin" );

      if( options.count( "bnet_endpoint" ) ) {
         auto ip_port = options.at("bnet_endpoint").as< string >();

        //auto host = boost::asio::ip::host_name(ip_port);
        auto port = ip_port.substr( ip_port.find(':')+1, ip_port.size() );
        auto host = ip_port.substr( 0, ip_port.find(':') );
        my->_bnet_endpoint_address = host;
        my->_bnet_endpoint_port = std::stoi( port );
        idump((ip_port)(host)(port));
      }

      if( options.count( "bnet_connect" ) ) {
         my->_connect_to_peers = options.at( "bnet_connect" ).as<vector<string>>();
      }
      if( options.count( "bnet_threads" ) ) {
         my->_num_threads = options.at("bnet_threads").as<uint32_t>();
         if( my->_num_threads > 8 ) 
            my->_num_threads = 8;
      }
      my->_request_trx = !options.at( "bnet_no_trx" ).as<bool>();
   }

   void bnet_plugin::plugin_startup() {
      wlog( "bnet startup " );

      my->_on_appled_trx_handle = app().get_channel<channels::accepted_transaction>()
                                .subscribe( [this]( transaction_metadata_ptr t ){
                                       my->on_accepted_transaction(t);
                                });

      my->_on_irb_handle = app().get_channel<channels::irreversible_block>()
                                .subscribe( [this]( block_state_ptr s ){
                                       my->on_irreversible_block(s);
                                });

      my->_on_accepted_block_handle = app().get_channel<channels::accepted_block>()
                                         .subscribe( [this]( block_state_ptr s ){
                                                my->on_accepted_block(s);
                                         });

      my->_on_bad_block_handle = app().get_channel<channels::rejected_block>()
                                .subscribe( [this]( signed_block_ptr b ){
                                       my->on_bad_block(b);
                                });


      const auto address = boost::asio::ip::make_address( my->_bnet_endpoint_address );
      my->_ioc.reset( new boost::asio::io_context{my->_num_threads} );


      auto& ioc = *my->_ioc;
      my->_timer = std::make_shared<boost::asio::deadline_timer>( app().get_io_service() );

      my->start_reconnect_timer();

      my->_listener = std::make_shared<listener>( ioc, 
                                                  tcp::endpoint{ address, my->_bnet_endpoint_port }, 
                                                  std::ref(*my) );
      my->_listener->run();

      my->_socket_threads.reserve( my->_num_threads );
      for( auto i = 0; i < my->_num_threads; ++i ) {
         my->_socket_threads.emplace_back( [&ioc]{ wlog( "start thread" ); ioc.run(); wlog( "end thread" ); } );
      }

      for( const auto& peer : my->_connect_to_peers ) {
         auto s = std::make_shared<session>( ioc, std::ref(*my) );
         s->_peer_id = my->_peer_id;
         my->_sessions[s.get()] = s;
         s->run( peer );
      }
   }

   void bnet_plugin::plugin_shutdown() {
      try {
         my->_timer->cancel();
         my->_timer.reset();
      } catch ( ... ) {
         elog( "exception thrown on timer shutdown" );
      }

      my->_listener.reset();
      my->_ioc->stop();

      wlog( "joining bnet threads" );
      for( auto& t : my->_socket_threads ) {
         t.join();
      }
      wlog( "done joining threads" );
      my->_ioc.reset();
      /// shut down all threads and close all connections
   }


   session::~session() {
     wlog( "close session ${n}",("n",_session_num) );
     _app_ios.post( [net=&_net_plugin,ses=this]{
        net->on_session_close(ses);
     });
   }

   void session::do_hello() {
      async_get_pending_block_ids( [self = shared_from_this() ]( const vector<block_id_type>& ids, uint32_t lib ){
          hello hello_msg;
          hello_msg.peer_id = self->_peer_id;
          hello_msg.last_irr_block_num = lib;
          hello_msg.pending_block_ids  = ids;
          hello_msg.request_transactions = self->_net_plugin._request_trx;
         
          self->_local_lib = lib;
          self->send( hello_msg );
          self->_sent_remote_hello = true;
      });
   }

   void session::check_for_redundant_connection() {
     app().get_io_service().post( [self=shared_from_this()]{
       self->_net_plugin.for_each_session( [self]( auto ses ){
         if( ses != self && ses->_remote_peer_id == self->_remote_peer_id ) {
           self->do_goodbye( "redundant connection" ); 
         }
       });
     });
   }

} /// namespace eosio
