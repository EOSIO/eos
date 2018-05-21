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
   public_key_type               remote_peer;
   string                        network_version;
   string                        agent;
   chain_id_type                 chain_id;
   uint32_t                      last_irr_block_num = 0;
   vector<block_id_type>         pending_block_ids;
};
FC_REFLECT( hello, (remote_peer)(network_version)(agent)(chain_id)(last_irr_block_num)(pending_block_ids) )

/**
 * This message tells the remote peer where our head block is and is sent
 * every time our head block changes to something other than the last block
 * a peer sent us.
struct status {
   block_id_type         head_block_id;
   block_timestamp_type  head_block_time;
   block_id_type         lib_block_id;
};
FC_REFLECT( status, (head_block_id)(head_block_time)(lib_block_id) )
 */

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


using bnet_message = fc::static_variant<hello,
                                        trx_notice,
                                        block_notice,
                                        signed_block_ptr,
                                        packed_transaction_ptr >;


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
           block_status( block_id_type i, 
                         bool kby_peer = false, 
                         bool nto_peer = false,
                         bool rfrom_peer = false,
                         bool nfrom_peer = false)
           {
              known_by_peer      = kby_peer;
              notice_to_peer     = nto_peer;
              received_from_peer = rfrom_peer;
              notice_from_peer   = nfrom_peer;
              id = i;
           }

           bool known_by_peer  = false;         ///< we sent block to peer or peer sent us notice
           bool notice_to_peer = false;         ///< we sent peer notice that we consider header valid
           bool received_from_peer = false;     ///< peer sent us this block and considers full block valid
           bool notice_from_peer = false;       ///< peer sent us a notice that they have this block and consider header valid
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
           bool                       known_by_peer = false; 
           bool                       notice_to_peer = false;
           time_point                 received;
           time_point                 expired;
           transaction_id_type        id;
           transaction_metadata_ptr   trx;
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

        uint32_t           _local_lib             = 0;
        uint32_t           _local_head_block_num  = 0;
        block_id_type      _local_head_block_id; /// the last block id received on local channel

        uint32_t           _remote_lib            = 0;
        uint32_t           _last_sent_block_num   = 0;
        block_id_type      _last_sent_block_id; /// the id of the last block sent
        bool               _recv_remote_hello     = false;
 

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
        boost::beast::multi_buffer                                     _in_buffer;

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


        ~session() {
           wlog( "close session ${n}",("n",_session_num) );
        }

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

          wlog( "resolve ${h}:${p}", ("h",_remote_host)("p",_remote_port) );
          _resolver.async_resolve( _remote_host, _remote_port, 
                                   std::bind( &session::on_resolve,
                                              shared_from_this(),
                                              std::placeholders::_1,
                                              std::placeholders::_2 ) );
        }

        void on_resolve( boost::system::error_code ec,
                         tcp::resolver::results_type results ) {
           if( ec ) return on_fail( ec, "resolve" );

           wlog( "resolve success" );
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

           wlog( "handshake success" );

           do_hello();
           do_read();
        }

        void on_accepted_transaction( transaction_metadata_ptr t ) {
           auto itr = _transaction_status.find( t->id );
           if( itr != _transaction_status.end() ) {
              return; /// this peer obviously knows this trx
           }
           
           //idump((t->id));
           transaction_status stat;
           stat.id  = t->id;
           stat.trx = t;
           stat.received = fc::time_point::now();
           stat.expired  = std::min<fc::time_point>( t->trx.expiration, fc::time_point::now()+fc::seconds(30) );

           _transaction_status.insert( stat );

           auto& idx = _transaction_status.get<by_expired>();
           auto start = idx.begin();
           if( start != idx.end() && start->expired < fc::time_point::now() ) {
              idx.erase(start);
              start = idx.begin();
           }
        }

        /**
         *  When our local LIB advances we can purge our known history up to
         *  the LIB or up to the last block known by the remote peer. 
         */
        void on_new_lib( block_state_ptr s ) {
           _local_lib = s->block_num;

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
           auto id = b->id();
           auto itr = _block_status.find( id );
           if( itr == _block_status.end() ) return;
           if( itr->received_from_peer ) {
              elog( "peer sent bad block #${b} ${i}, disconnect", ("b", b->block_num())("i",b->id())  );
              _ws->next_layer().close();
           }
        }

        void on_accepted_block( block_id_type id, block_id_type prev, shared_ptr<vector<char>> msgdata, block_state_ptr s ) {
           if( !_strand.running_in_this_thread() ) { elog( "wrong strand" ); }

           mark_block_known_by_peer( id );
           auto itr = _block_status.find( id );

           _block_status.modify( itr, [&]( auto& item ) {
              item.block_msg = msgdata;
              item.prev = prev;
           });

           _local_head_block_id = id;
           _local_head_block_num = block_header::num_from_id(id);

           for( const auto& receipt: s->block->transactions ) {
              if( receipt.trx.which() == 1 ) {
                 auto id = receipt.trx.get<packed_transaction>().id();
                 auto itr = _transaction_status.find( id );
                 _transaction_status.erase(itr);
              }
           }
           if( _transaction_status.size() > 1000 ) {
              idump((_transaction_status.size()));
           }

           do_send_next_message();
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

        void do_hello() {
           async_get_pending_block_ids( [self = shared_from_this() ]( const vector<block_id_type>& ids, uint32_t lib ){
               hello hello_msg;
               hello_msg.last_irr_block_num = lib;
               hello_msg.pending_block_ids  = ids;
              
               self->_local_lib = lib;
               self->send( hello_msg );
           });
        }

        void send( const shared_ptr<vector<char> >& buffer ) {
           try {
          //    wdump((_out_buffer.size())(buffer->size()));
              FC_ASSERT( !_out_buffer.size() );
              _out_buffer.resize(buffer->size());
              memcpy( _out_buffer.data(), buffer->data(), _out_buffer.size() );

              _state = sending_state;
           //    wlog( "state = sending" );
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

        void mark_block_known_by_peer( block_id_type id, bool noticed_by_peer = false, bool recv_from_peer = false ) {
            auto itr = _block_status.find(id);
            if( itr == _block_status.end() ) {
              itr = _block_status.insert( block_status(id) ).first;
            } 

            _block_status.modify( itr, [&]( auto& item ) {
              item.known_by_peer = true;
              item.notice_from_peer   |= noticed_by_peer;
              item.received_from_peer |= recv_from_peer;
            });
        }

        void do_send_next_message() {
           //wlog("");
           if( _state == sending_state ) return; /// in process of sending
           if( _out_buffer.size() ) return;
           if( !_recv_remote_hello ) return;
      //     wlog( "do send next message" );

           //wdump((_remote_head_block_num)(_local_lib) );

           /// until we get caught up with last irreversible block we 
           /// will simply fetch blocks from the block log and send
           if( _last_sent_block_num <= _local_lib ) {
              _state = sending_state;
            //  wlog( "state = sending" );
              async_get_block_num( _last_sent_block_num + 1, 
                   [self=shared_from_this()]( auto sblockptr ) {
                     self->_last_sent_block_num++;
                     self->_last_sent_block_id = sblockptr->id();
                     self->send( sblockptr );
                   }
              );
              return;
           } 
           if( _last_sent_block_num < _local_head_block_num ) 
              send_next_block();
           else
              send_next_trx();
        }

        bool is_known_by_peer( block_id_type id ) {
           auto itr = _block_status.find(id);
           if( itr == _block_status.end() ) return false;
           return itr->known_by_peer;
        }

        void send_next_trx() { try {
           //wlog( "send next trx" );
           auto& idx = _transaction_status.get<by_received>();
           auto start = idx.begin();
           while( start != idx.end() && start->expired < fc::time_point::now() ) {
              idx.erase( start );
              start = idx.begin();
           }
           if( idx.size() > 2000 ) {
              idump((idx.size()));
           }

           while( start != idx.end() && start->received < fc::time_point::maximum() ) {
              if( !start->known_by_peer ) {
                 idx.modify( start, [&]( auto& stat ) {
                     stat.received = fc::time_point::maximum();
                     stat.known_by_peer = true;
                 });
                 auto ptrx_ptr = std::make_shared<packed_transaction>( start->trx->packed_trx );
             //    wlog("sending trx ${id}", ("id",start->id) );
                 send(ptrx_ptr);
                 return;
              }
              ++start;
           }
        } FC_LOG_AND_RETHROW() }

        void send_next_block() {
           //wlog( "send next trx" );
           try {
              _state = sending_state;
            //  wlog( "state = sending " );
              async_get_block_num( _last_sent_block_num + 1, 
                   [self=shared_from_this()]( auto sblockptr ) {
                     auto prev = sblockptr->previous;

                     bool peer_knows_prev = self->_last_sent_block_id == prev; /// shortcut
                     if( !peer_knows_prev ) 
                        peer_knows_prev = self->is_known_by_peer( prev );

                     if( peer_knows_prev ) {
                        self->_last_sent_block_id  = sblockptr->id();
                        self->_last_sent_block_num = block_header::num_from_id(self->_last_sent_block_id);
                        self->mark_block_known_by_peer( self->_last_sent_block_id );
                        //ilog( "sending pending........... ${n}", ("n", sblockptr->block_num())  );
                        self->send( sblockptr );
                     } else { 
                        wlog( "looks like we had a fork... see if peer knows previous block num" );
                        self->_state = idle_state;
                        wlog( "state = idle" );
                        /// we must have forked... peer doesn't know about previous,
                        /// we need to find the most recent block the peer does know about
                        self->_last_sent_block_num--;
                        self->send_next_block();
                     }
                   }
              );
           } FC_LOG_AND_RETHROW() 
        }

        void on_fail( boost::system::error_code ec, const char* what ) {
           elog( "${w}: ${m}", ("w", what)("m", ec.message() ) );
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
           auto str = boost::beast::buffers_to_string(_in_buffer.data());

           /*
           auto bufs = _in_buffer.data();
           for( auto b : bufs ) {
              wdump((b.size()));
              auto tmp = b.data();
           }
           */
           fc::datastream<const char*> ds(str.data(), str.size());
           bnet_message msg;
           fc::raw::unpack( ds, msg );
           _in_buffer.consume( ds.tellp() );

           on_message( msg );
           do_read();
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
              }
              do_send_next_message();
           } catch( const fc::exception& e ) {
              elog( "${e}", ("e",e.to_detail_string()));
           }
        }

        void on( const hello& hi ) {
           idump((hi));
           _recv_remote_hello     = true;
           _last_sent_block_num   = hi.last_irr_block_num;
         //  _last_sent_block_id    = hi.lib_block_id;

           for( const auto& id : hi.pending_block_ids )
              mark_block_known_by_peer( id, true );
        }

        void on( const block_notice& stat ) {
           mark_block_known_by_peer( stat.block_id, true );
        }

        void on( const signed_block_ptr& b ) {
           auto id = b->id();
           mark_block_known_by_peer( id, true, true );

           _app_ios.post( [id,b]{
              auto& control = app().get_plugin<chain_plugin>().chain();
              auto existing = control.fetch_block_by_id( id );
              if( !existing ) {
                 app().get_channel<incoming::channels::block>().publish(b);
              }
           });
        }

        void on( const packed_transaction_ptr& p ) {
           auto id = p->id();
           //wlog( "received trx ${id}", ("id",id) );
           auto itr = _transaction_status.find( id );
           if( itr != _transaction_status.end() ) {
              _transaction_status.modify( itr, [&]( auto& stat ) {
                 stat.known_by_peer = true;
                 stat.received      = fc::time_point::maximum(); /// don't send it back
                 stat.expired       = std::min<fc::time_point>(stat.trx->trx.expiration, stat.trx->trx.expiration + fc::seconds(30) );
              });
              if( itr->trx ) return; /// we already know it
           }
           else 
           {
              transaction_status stat;
              stat.id       = id;
              stat.trx      = std::make_shared<transaction_metadata>( *p );

              if( stat.trx->trx.expiration < fc::time_point::now() ) 
                 return; /// nothing to do here
              
              stat.received = fc::time_point::maximum(); /// don't send it back
              stat.expired  = std::min<fc::time_point>(stat.trx->trx.expiration, stat.trx->trx.expiration + fc::seconds(30) );
              _transaction_status.insert(stat);
           }

           _app_ios.post( [id,p]{
              app().get_channel<incoming::channels::transaction>().publish(p);
           });
        }

        void on_write( boost::system::error_code ec, std::size_t bytes_transferred ) {
           boost::ignore_unused(bytes_transferred);
           if( ec ) {
              _ws->next_layer().close();
              return on_fail( ec, "write" );
           }
           _state = idle_state;
           //wlog( "state = idle" );
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
           if( ec ) {
              return;
           }

           _acceptor.set_option( boost::asio::socket_base::reuse_address(true) );

           _acceptor.bind( endpoint, ec );
           if( ec ) {
              return;
           }

           _acceptor.listen( boost::asio::socket_base::max_listen_connections, ec );
           if( ec ) {
           }
        }

        void run() {
           FC_ASSERT( _acceptor.is_open(), "unable top open listen socket" );
           do_accept();
        }

        void do_accept() {
           _acceptor.async_accept( _socket, [self=shared_from_this()]( auto ec ){ self->on_accept(ec); } );
        }

        void on_accept( boost::system::error_code ec );
  };


   class bnet_plugin_impl {
      public:
         string                                         _bnet_endpoint_address = "0.0.0.0";
         uint16_t                                       _bnet_endpoint_port = 4321;
                                                        
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

         void on_accepted_transaction( transaction_metadata_ptr trx ) {
            if( trx->trx.signatures.size() == 0 ) return;

            for( const auto& item : _sessions ) {
               auto ses = item.second.lock();
               if( ses ) {
                  //ses->_local_lib = s->block_num;
                  //wdump((ses->_local_lib));
                  ses->_ios.post( boost::asio::bind_executor(
                                        ses->_strand,
                                        [ses,trx](){
                                           ses->on_accepted_transaction(trx);
                                        }
                                    ));
               } 
            }
         }

         /**
          * Notify all active connection of the new irreversible block so they
          * can purge their block cache
          */
         void on_irreversible_block( block_state_ptr s ) {
           // wlog("on irb" );
            vector<const session*> removed;
            for( const auto& item : _sessions ) {
               auto ses = item.second.lock();
               if( ses ) {
                  //ses->_local_lib = s->block_num;
                  //wdump((ses->_local_lib));
                  ses->_ios.post( boost::asio::bind_executor(
                                        ses->_strand,
                                        [ses,s](){
                                           ses->on_new_lib(s);
                                        }
                                    ));
               } else {
                  removed.push_back(item.first);
               }
            }
            for( auto r : removed ) {
               _sessions.erase( _sessions.find(r) );
            }
         }

         /**
          * Notify all active connections of the new accepted block so
          * they can relay it. This method also pre-packages the block
          * as a packed bnet_message so the connections can simply relay
          * it on.
          */
         void on_accepted_block( block_state_ptr s ) {
            bnet_message msg(s->block);
            auto id = s->id;
            auto prev = s->header.previous;

            auto msgdata = std::make_shared<vector<char>>( fc::raw::pack(msg) );

            vector<const session*> removed;
            for( const auto& item : _sessions ) {
               shared_ptr<session> ses = item.second.lock();
               if( ses ) {
                  ses->_ios.post( boost::asio::bind_executor(
                                        ses->_strand,
                                        [ses,id,prev,msgdata,s](){
                                           ses->on_accepted_block(id,prev,msgdata,s);
                                        }
                                    ));
               } else {
                  removed.push_back(item.first);
               }
            }

            for( auto r : removed ) {
               _sessions.erase( _sessions.find(r) );
            }
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
            wdump(( s->id() ) );
            for( const auto& item : _sessions ) {
               auto ses = item.second.lock();
               if( ses ) {
                  wdump((ses->_local_lib));
                  ses->_ios.post( boost::asio::bind_executor(
                                        ses->_strand,
                                        [ses,s](){
                                           ses->on_bad_block(s);
                                        }
                                    ));
               }
            }
         };

         void on_reconnect_peers() {
             wlog( "attempt to connect to missing peers" );

             idump((_connect_to_peers)(_sessions.size()));
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
                   ilog( "reconnecting..." );
                   auto s = std::make_shared<session>( *_ioc, std::ref(*this) );
                   ilog( "reconnecting..." );
                   _sessions[s.get()] = s;
                   ilog( "reconnecting..." );
                   s->run( peer );
                   ilog( "reconnecting..." );
                }
             }

             start_reconnect_timer();
         }
         void start_reconnect_timer() {
            /// add some random delay so that all my peers don't attempt to reconnect to me 
            /// at the same time after shutting down.. TODO: verify srand is called
            _timer->expires_from_now( boost::posix_time::microseconds( 1000000*(10+rand()%5) ) );
            _timer->async_wait([=](const boost::system::error_code& ec) {
                if( ec ) {
                    elog( "reconnect timer error" );
                    return;
                }
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
         ;
   }

   void bnet_plugin::plugin_initialize(const variables_map& options) {
      ilog( "Initialize bnet plugin" );

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
                                       wdump(("on bad block"));
                                       my->on_bad_block(b);
                                });


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
   }

   void bnet_plugin::plugin_startup() {
      wlog( "bnet startup " );
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
      /// shut down all threads and close all connections
   }


} /// namespace eosio
