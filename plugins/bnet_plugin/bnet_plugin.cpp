/**
 *  The purpose of this protocol is to synchronize (and keep synchronized) two
 *  blockchains using a very simple algorithm:
 *
 *  1. find the last block id on our local chain that the remote peer knows about
 *  2. if we have the next block send it to them
 *  3. if we don't have the next block send them a the oldest unexpired transaction
 *
 *  There are several input events:
 *
 *  1. new block accepted by local chain
 *  2. block deemed irreversible by local chain
 *  3. new block header accepted by local chain
 *  4. transaction accepted by local chain
 *  5. block received from remote peer
 *  6. transaction received from remote peer
 *  7. socket ready for next write
 *
 *  Each session is responsible for maintaining the following
 *
 *  1. the most recent block on our current best chain which we know
 *     with certainty that the remote peer has.
 *      - this could be the peers last irreversible block
 *      - a block ID after the LIB that the peer has notified us of
 *      - a block which we have sent to the remote peer
 *      - a block which the peer has sent us
 *  2. the block IDs we have received from the remote peer so that
 *     we can disconnect peer if one of those blocks is deemed invalid
 *      - we can clear these IDs once the block becomes reversible
 *  3. the transactions we have received from the remote peer so that
 *     we do not send them something that they already know.
 *       - this includes transactions sent as part of blocks
 *       - we clear this cache after we have applied a block that
 *         includes the transactions because we know the controller
 *         should not notify us again (they would be dupe)
 *
 *  Assumptions:
 *  1. all blocks we send the peer are valid and will be held in the
 *     peers fork database until they become irreversible or are replaced
 *     by an irreversible alternative.
 *  2. we don't care what fork the peer is on, so long as we know they have
 *     the block prior to the one we want to send. The peer will sort it out
 *     with its fork database and hopfully come to our conclusion.
 *  3. the peer will send us blocks on the same basis
 *
 */

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
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <eosio/chain/plugin_interface.hpp>

using tcp = boost::asio::ip::tcp;
namespace ws  = boost::beast::websocket;

namespace eosio {
   using namespace chain;

   static appbase::abstract_plugin& _bnet_plugin = app().register_plugin<bnet_plugin>();

} /// namespace eosio

namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
}

const fc::string logger_name("bnet_plugin");
fc::logger plugin_logger;
std::string peer_log_format;

#define peer_dlog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( plugin_logger.is_enabled( fc::log_level::debug ) ) \
      plugin_logger.log( FC_LOG_MESSAGE( debug, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_ilog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( plugin_logger.is_enabled( fc::log_level::info ) ) \
      plugin_logger.log( FC_LOG_MESSAGE( info, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_wlog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( plugin_logger.is_enabled( fc::log_level::warn ) ) \
      plugin_logger.log( FC_LOG_MESSAGE( warn, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_elog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( plugin_logger.is_enabled( fc::log_level::error ) ) \
      plugin_logger.log( FC_LOG_MESSAGE( error, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant())) ); \
  FC_MULTILINE_MACRO_END


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
   string                        protocol_version = "1.0.1";
   string                        user;
   string                        password;
   chain_id_type                 chain_id;
   bool                          request_transactions = false;
   uint32_t                      last_irr_block_num = 0;
   vector<block_id_type>         pending_block_ids;
};
FC_REFLECT( hello, (peer_id)(network_version)(user)(password)(agent)(protocol_version)(chain_id)(request_transactions)(last_irr_block_num)(pending_block_ids) )

struct hello_extension_irreversible_only {};

FC_REFLECT( hello_extension_irreversible_only, BOOST_PP_SEQ_NIL )

using hello_extension = fc::static_variant<hello_extension_irreversible_only>;

/**
 * This message is sent upon successful speculative application of a transaction
 * and informs a peer not to send this message.
 */
struct trx_notice {
   vector<sha256>  signed_trx_id; ///< hash of trx + sigs
};

FC_REFLECT( trx_notice, (signed_trx_id) )

/**
 *  This message is sent upon successfully adding a transaction to the fork database
 *  and informs the remote peer that there is no need to send this block.
 */
struct block_notice {
   vector<block_id_type> block_ids;
};

FC_REFLECT( block_notice, (block_ids) );

struct ping {
   fc::time_point sent;
   fc::sha256     code;
   uint32_t       lib; ///< the last irreversible block
};
FC_REFLECT( ping, (sent)(code)(lib) )

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

  template <typename Strand>
  void verify_strand_in_this_thread(const Strand& strand, const char* func, int line) {
     if( !strand.running_in_this_thread() ) {
        elog( "wrong strand: ${f} : line ${n}, exiting", ("f", func)("n", line) );
        app().quit();
     }
  }

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
       //    block_id_type prev;                  ///< the prev block id

       //    shared_ptr< vector<char> >  block_msg; ///< packed bnet_message for this block

           uint32_t block_num()const { return block_header::num_from_id(id); }
        };

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

           void mark_known_by_peer() { received = fc::time_point::maximum(); trx.reset();  }
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

        public_key_type    _local_peer_id;
        uint32_t           _local_lib             = 0;
        block_id_type      _local_lib_id;
        uint32_t           _local_head_block_num  = 0;
        block_id_type      _local_head_block_id; /// the last block id received on local channel


        public_key_type    _remote_peer_id;
        uint32_t           _remote_lib            = 0;
        block_id_type      _remote_lib_id;
        bool               _remote_request_trx    = false;
        bool               _remote_request_irreversible_only = false;

        uint32_t           _last_sent_block_num   = 0;
        block_id_type      _last_sent_block_id; /// the id of the last block sent
        bool               _recv_remote_hello     = false;
        bool               _sent_remote_hello     = false;


        fc::sha256         _current_code;
        fc::time_point     _last_recv_ping_time = fc::time_point::now();
        ping               _last_recv_ping;
        ping               _last_sent_ping;


        int                                                            _session_num = 0;
        session_state                                                  _state = hello_state;
        tcp::resolver                                                  _resolver;
        bnet_ptr                                                       _net_plugin;
        boost::asio::io_service&                                       _ios;
        unique_ptr<ws::stream<tcp::socket>>                            _ws;
        boost::asio::strand< boost::asio::io_context::executor_type>   _strand;
        boost::asio::io_service&                                       _app_ios;

        methods::get_block_by_number::method_type& _get_block_by_number;


        string                                                         _peer;
        string                                                         _remote_host;
        string                                                         _remote_port;

        vector<char>                                                  _out_buffer;
        //boost::beast::multi_buffer                                  _in_buffer;
        boost::beast::flat_buffer                                     _in_buffer;
        flat_set<block_id_type>                                       _block_header_notices;
        fc::optional<fc::variant_object>                              _logger_variant;


        int next_session_id()const {
           static int session_count = 0;
           return ++session_count;
        }

        /**
         * Creating session from server socket acceptance
         */
        explicit session( tcp::socket socket, bnet_ptr net_plug )
        :_resolver(socket.get_io_service()),
         _net_plugin( std::move(net_plug) ),
         _ios(socket.get_io_service()),
         _ws( new ws::stream<tcp::socket>(move(socket)) ),
         _strand(_ws->get_executor() ),
         _app_ios( app().get_io_service() ),
         _get_block_by_number( app().get_method<methods::get_block_by_number>() )
        {
            _session_num = next_session_id();
            set_socket_options();
            _ws->binary(true);
            wlog( "open session ${n}",("n",_session_num) );
        }


        /**
         * Creating outgoing session
         */
        explicit session( boost::asio::io_context& ioc, bnet_ptr net_plug )
        :_resolver(ioc),
         _net_plugin( std::move(net_plug) ),
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


        void set_socket_options() {
           try {
            /** to minimize latency when sending short messages */
            _ws->next_layer().set_option( boost::asio::ip::tcp::no_delay(true) );

            /** to minimize latency when sending large 1MB blocks, the send buffer should not have to
             * wait for an "ack", making this larger could result in higher latency for smaller urgent
             * messages.
             */
            _ws->next_layer().set_option( boost::asio::socket_base::send_buffer_size( 1024*1024 ) );
            _ws->next_layer().set_option( boost::asio::socket_base::receive_buffer_size( 1024*1024 ) );
           } catch ( ... ) {
              elog( "uncaught exception on set socket options" );
           }
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

          _resolver.async_resolve( _remote_host, _remote_port,
                                   boost::asio::bind_executor( _strand,
                                   std::bind( &session::on_resolve,
                                              shared_from_this(),
                                              std::placeholders::_1,
                                              std::placeholders::_2 ) ) );
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

           set_socket_options();

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
           stat.id       = t->id;
           stat.trx      = t;
           _transaction_status.insert( stat );

           maybe_send_next_message();
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
           verify_strand_in_this_thread(_strand, __func__, __LINE__);
           _local_lib = s->block_num;
           _local_lib_id = s->id;

           auto purge_to = std::min( _local_lib, _last_sent_block_num );

           auto& idx = _block_status.get<by_num>();
           auto itr = idx.begin();
           while( itr != idx.end() && itr->block_num() < purge_to ) {
              idx.erase(itr);
              itr = idx.begin();
           }

           if( _remote_request_irreversible_only ) {
              auto bitr = _block_status.find(s->id);
              if ( bitr == _block_status.end() || !bitr->received_from_peer ) {
                 _block_header_notices.insert(s->id);
              }
           }

           maybe_send_next_message();
        }


        void on_bad_block( signed_block_ptr b ) {
           verify_strand_in_this_thread(_strand, __func__, __LINE__);
           try {
              auto id = b->id();
              auto itr = _block_status.find( id );
              if( itr == _block_status.end() ) return;
              if( itr->received_from_peer ) {
                 peer_elog(this, "bad signed_block_ptr : unknown" );
                 elog( "peer sent bad block #${b} ${i}, disconnect", ("b", b->block_num())("i",b->id())  );
                 _ws->next_layer().close();
              }
           } catch ( ... ) {
              elog( "uncaught exception" );
           }
        }

        void on_accepted_block_header( const block_state_ptr& s ) {
           verify_strand_in_this_thread(_strand, __func__, __LINE__);
          // ilog( "accepted block header ${n}", ("n",s->block_num) );
           if( fc::time_point::now() - s->block->timestamp  < fc::seconds(6) ) {
           //   ilog( "queue notice to peer that we have this block so hopefully they don't send it to us" );
              auto itr = _block_status.find(s->id);
              if( !_remote_request_irreversible_only && ( itr == _block_status.end() || !itr->received_from_peer ) ) {
                 _block_header_notices.insert(s->id);
              }
           }

           //idump((_block_status.size())(_transaction_status.size()));
           auto id = s->id;
           //ilog( "accepted block ${n}", ("n",s->block_num) );

           auto itr = _block_status.find( id );
           if( itr == _block_status.end() ) {
              itr = _block_status.insert( block_status(id, false, false) ).first;
           }

           _local_head_block_id = id;
           _local_head_block_num = block_header::num_from_id(id);

           if( _local_head_block_num < _last_sent_block_num ) {
              _last_sent_block_num = _local_lib;
              _last_sent_block_id  = _local_lib_id;
           }

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

           maybe_send_next_message(); /// attempt to send if we are idle
        }


        template<typename L>
        void async_get_pending_block_ids( L&& callback ) {
           /// send peer my head block status which is read from chain plugin
           _app_ios.post( [self = shared_from_this(),callback]{
              auto& control = app().get_plugin<chain_plugin>().chain();
              auto lib = control.last_irreversible_block_num();
              auto head = control.fork_db_head_block_id();
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
              signed_block_ptr sblockptr;
              try {
                 //ilog( "fetch block ${n}", ("n",blocknum) );
                 sblockptr = control.fetch_block_by_number( blocknum );
              } catch ( const fc::exception& e ) {
                 edump((e.to_detail_string()));
              }

              self->_ios.post( boost::asio::bind_executor(
                    self->_strand,
                    [callback,sblockptr](){
                       callback(sblockptr);
                    }
              ));
           });
        }

        void do_hello();


        void send( const bnet_message& msg ) { try {
           auto ps = fc::raw::pack_size(msg);
           _out_buffer.resize(ps);
           fc::datastream<char*> ds(_out_buffer.data(), ps);
           fc::raw::pack(ds, msg);
           send();
        } FC_LOG_AND_RETHROW() }

        template<class T>
        void send( const bnet_message& msg, const T& ex ) { try {
           auto ex_size = fc::raw::pack_size(ex);
           auto ps = fc::raw::pack_size(msg) + fc::raw::pack_size(unsigned_int(ex_size)) + ex_size;
           _out_buffer.resize(ps);
           fc::datastream<char*> ds(_out_buffer.data(), ps);
           fc::raw::pack( ds, msg );
           fc::raw::pack( ds, unsigned_int(ex_size) );
           fc::raw::pack( ds, ex );
           send();
        } FC_LOG_AND_RETHROW() }

        void send() { try {
           verify_strand_in_this_thread(_strand, __func__, __LINE__);

           _state = sending_state;
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


        /**
         *  This method will determine whether there is a message in the
         *  out queue, if so it returns. Otherwise it determines the best
         *  message to send.
         */
        void maybe_send_next_message() {
           verify_strand_in_this_thread(_strand, __func__, __LINE__);
           if( _state == sending_state ) return; /// in process of sending
           if( _out_buffer.size() ) return; /// in process of sending
           if( !_recv_remote_hello || !_sent_remote_hello ) return;

           clear_expired_trx();

           if( send_block_notice() ) return;
           if( send_pong() ) return;
           if( send_ping() ) return;

           /// we don't know where we are (waiting on accept block localhost)
           if( _local_head_block_id == block_id_type() ) return ;
           if( send_next_block() ) return;
           if( send_next_trx() ) return;
        }

        bool send_block_notice() {
           if( _block_header_notices.size() == 0 )
              return false;

           block_notice notice;
           notice.block_ids.reserve( _block_header_notices.size() );
           for( auto& id : _block_header_notices )
              notice.block_ids.emplace_back(id);
           send(notice);
           _block_header_notices.clear();
           return true;
        }

        bool send_pong() {
           if( _last_recv_ping.code == fc::sha256() )
              return false;

           send( pong{ fc::time_point::now(), _last_recv_ping.code } );
           _last_recv_ping.code = fc::sha256();
           return true;
        }

        bool send_ping() {
           auto delta_t = fc::time_point::now() - _last_sent_ping.sent;
           if( delta_t < fc::seconds(3) ) return false;

           if( _last_sent_ping.code == fc::sha256() ) {
              _last_sent_ping.sent = fc::time_point::now();
              _last_sent_ping.code = fc::sha256::hash(_last_sent_ping.sent); /// TODO: make this more random
              _last_sent_ping.lib  = _local_lib;
              send( _last_sent_ping );
           }

           /// we expect the peer to send us a ping every 3 seconds, so if we haven't gotten one
           /// in the past 6 seconds then the connection is likely hung. Unfortunately, we cannot
           /// use the round-trip time of ping/pong to measure latency because during syncing the
           /// remote peer can be stuck doing CPU intensive tasks that block its reading of the
           /// buffer.  This buffer gets filled with perhaps 100 blocks taking .1 seconds each for
           /// a total processing time of 10+ seconds. That said, the peer should come up for air
           /// every .1 seconds so should still be able to send out a ping every 3 seconds.
           //
           // We don't want to wait a RTT for each block because that could also slow syncing for
           // empty blocks...
           //
           //if( fc::time_point::now() - _last_recv_ping_time > fc::seconds(6) ) {
           //   do_goodbye( "no ping from peer in last 6 seconds...." );
           //}
           return true;
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


           auto ptrx_ptr = std::make_shared<packed_transaction>( start->trx->packed_trx );

           idx.modify( start, [&]( auto& stat ) {
              stat.mark_known_by_peer();
           });

           // wlog("sending trx ${id}", ("id",start->id) );
           send(ptrx_ptr);

           return true;

        } FC_LOG_AND_RETHROW() }

        void on_async_get_block( const signed_block_ptr& nextblock ) {
           verify_strand_in_this_thread(_strand, __func__, __LINE__);
            if( !nextblock)  {
               _state = idle_state;
               maybe_send_next_message();
               return;
            }

            /// if something changed, the next block doesn't link to the last
            /// block we sent, local chain must have switched forks
            if( nextblock->previous != _last_sent_block_id ) {
                if( !is_known_by_peer( nextblock->previous ) ) {
                  _last_sent_block_id  = _local_lib_id;
                  _last_sent_block_num = _local_lib;
                  _state = idle_state;
                  maybe_send_next_message();
                  return;
                }
            }

            /// at this point we know the peer can link this block

            auto next_id = nextblock->id();

            /// if the peer already knows about this block, great no need to
            /// send it, mark it as 'sent' and move on.
            if( is_known_by_peer( next_id ) ) {
               _last_sent_block_id  = next_id;
               _last_sent_block_num = nextblock->block_num();

               _state = idle_state;
               maybe_send_next_message();
               return;
            }

            mark_block_known_by_peer( next_id );

            _last_sent_block_id  = next_id;
            _last_sent_block_num = nextblock->block_num();

            send( nextblock );
            status( "sending block " + std::to_string( block_header::num_from_id(next_id) ) );

            if( nextblock->timestamp > (fc::time_point::now() - fc::seconds(5)) ) {
               mark_block_transactions_known_by_peer( nextblock );
            }
        }

        /**
         *  Send the next block after the last block in our current fork that
         *  we know the remote peer knows.
         */
        bool send_next_block() {

           if ( _remote_request_irreversible_only && _last_sent_block_id == _local_lib_id ) {
              return false;
           }

           if( _last_sent_block_id == _local_head_block_id ) /// we are caught up
              return false;

           ///< set sending state because this callback may result in sending a message
           _state = sending_state;
           async_get_block_num( _last_sent_block_num + 1,
                [self=shared_from_this()]( auto sblockptr ) {
                      self->on_async_get_block( sblockptr );
           });

           return true;
        }

        void on_fail( boost::system::error_code ec, const char* what ) {
           try {
              verify_strand_in_this_thread(_strand, __func__, __LINE__);
              elog( "${w}: ${m}", ("w", what)("m", ec.message() ) );
              _ws->next_layer().close();
           } catch ( ... ) {
              elog( "uncaught exception on close" );
           }
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

           try {
              auto d = boost::asio::buffer_cast<char const*>(boost::beast::buffers_front(_in_buffer.data()));
              auto s = boost::asio::buffer_size(_in_buffer.data());
              fc::datastream<const char*> ds(d,s);

              bnet_message msg;
              fc::raw::unpack( ds, msg );
              on_message( msg, ds );
              _in_buffer.consume( ds.tellp() );

              wait_on_app();
              return;

           } catch ( ... ) {
              wlog( "close bad payload" );
           }
           try {
              _ws->close( boost::beast::websocket::close_code::bad_payload );
           } catch ( ... ) {
              elog( "uncaught exception on close" );
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
            app().get_io_service().post( 
                boost::asio::bind_executor( _strand, [self=shared_from_this()]{ self->do_read(); } )
            );
        }

     void on_message( const bnet_message& msg, fc::datastream<const char*>& ds ) {
           try {
              switch( msg.which() ) {
                 case bnet_message::tag<hello>::value:
                    on( msg.get<hello>(), ds );
                    break;
                 case bnet_message::tag<block_notice>::value:
                    on( msg.get<block_notice>() );
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
              maybe_send_next_message();
           } catch( const fc::exception& e ) {
              elog( "${e}", ("e",e.to_detail_string()));
              _ws->close( boost::beast::websocket::close_code::bad_payload );
           }
        }

        void on( const block_notice& notice ) {
           peer_ilog(this, "received block_notice");
           for( const auto& id : notice.block_ids ) {
              status( "received notice " + std::to_string( block_header::num_from_id(id) ) );
              mark_block_known_by_peer( id );
           }
        }

     void on( const hello& hi, fc::datastream<const char*>& ds );

        void on( const ping& p ) {
           peer_ilog(this, "received ping");
           _last_recv_ping = p;
           _remote_lib     = p.lib;
           _last_recv_ping_time = fc::time_point::now();
        }

        void on( const pong& p ) {
           peer_ilog(this, "received pong");
           if( p.code != _last_sent_ping.code ) {
              peer_elog(this, "bad ping : invalid pong code");
              return do_goodbye( "invalid pong code" );
           }
           _last_sent_ping.code = fc::sha256();
        }

        void do_goodbye( const string& reason ) {
           try {
              status( "goodbye - " + reason );
              _ws->next_layer().close();
           } catch ( ... ) {
              elog( "uncaught exception on close" );
           }
        }

        void check_for_redundant_connection();

        void on( const signed_block_ptr& b ) {
           peer_ilog(this, "received signed_block_ptr");
           if (!b) {
              peer_elog(this, "bad signed_block_ptr : null pointer");
              EOS_THROW(block_validate_exception, "bad block" );
           }
           status( "received block " + std::to_string(b->block_num()) );
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
           peer_ilog(this, "received packed_transaction_ptr");
           if (!p) {
              peer_elog(this, "bad packed_transaction_ptr : null pointer");
              EOS_THROW(transaction_exception, "bad transaction");
           }

           auto id = p->id();
          // ilog( "recv trx ${n}", ("n", id) );
           if( p->expiration() < fc::time_point::now() ) return;

           if( mark_transaction_known_by_peer( id ) )
              return;

           app().get_channel<incoming::channels::transaction>().publish(p);
        }

        void on_write( boost::system::error_code ec, std::size_t bytes_transferred ) {
           boost::ignore_unused(bytes_transferred);
           verify_strand_in_this_thread(_strand, __func__, __LINE__);
           if( ec ) {
              _ws->next_layer().close();
              return on_fail( ec, "write" );
           }
           _state = idle_state;
           _out_buffer.resize(0);
           maybe_send_next_message();
        }

        void status( const string& msg ) {
        //   ilog( "${remote_peer}: ${msg}", ("remote_peer",fc::variant(_remote_peer_id).as_string().substr(3,5) )("msg",msg) );
        }

        const fc::variant_object& get_logger_variant()  {
           if (!_logger_variant) {
              boost::system::error_code ec;
              auto rep = _ws->lowest_layer().remote_endpoint(ec);
              string ip = ec ? "<unknown>" : rep.address().to_string();
              string port = ec ? "<unknown>" : std::to_string(rep.port());

              auto lep = _ws->lowest_layer().local_endpoint(ec);
              string lip = ec ? "<unknown>" : lep.address().to_string();
              string lport = ec ? "<unknown>" : std::to_string(lep.port());

              _logger_variant.emplace(fc::mutable_variant_object()
                 ("_name", _peer)
                 ("_id", _remote_peer_id)
                 ("_ip", ip)
                 ("_port", port)
                 ("_lip", lip)
                 ("_lport", lport)
              );
           }
           return *_logger_variant;
        }
  };


  /**
   *  Accepts incoming connections and launches the sessions
   */
  class listener : public std::enable_shared_from_this<listener> {
     private:
        tcp::acceptor         _acceptor;
        tcp::socket           _socket;
        bnet_ptr              _net_plugin;

     public:
        listener( boost::asio::io_context& ioc, tcp::endpoint endpoint, bnet_ptr np  )
        :_acceptor(ioc), _socket(ioc), _net_plugin(std::move(np))
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
           EOS_ASSERT( _acceptor.is_open(), plugin_exception, "unable top open listen socket" );
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


   class bnet_plugin_impl : public std::enable_shared_from_this<bnet_plugin_impl> {
      public:
         bnet_plugin_impl() = default;

         const private_key_type  _peer_pk = fc::crypto::private_key::generate(); /// one time random key to identify this process
         public_key_type         _peer_id = _peer_pk.get_public_key();
         string                                                 _bnet_endpoint_address = "0.0.0.0";
         uint16_t                                               _bnet_endpoint_port = 4321;
         bool                                                   _request_trx = true;
         bool                                                   _follow_irreversible = false;

         std::vector<std::string>                               _connect_to_peers; /// list of peers to connect to
         std::vector<std::thread>                               _socket_threads;
         int32_t                                                _num_threads = 1;

         std::unique_ptr<boost::asio::io_context>               _ioc; // lifetime guarded by shared_ptr of bnet_plugin_impl
         std::shared_ptr<listener>                              _listener;
         std::shared_ptr<boost::asio::deadline_timer>           _timer;    // only access on app io_service
         std::map<const session*, std::weak_ptr<session> >      _sessions; // only access on app io_service

         channels::irreversible_block::channel_type::handle     _on_irb_handle;
         channels::accepted_block::channel_type::handle         _on_accepted_block_handle;
         channels::accepted_block_header::channel_type::handle  _on_accepted_block_header_handle;
         channels::rejected_block::channel_type::handle         _on_bad_block_handle;
         channels::accepted_transaction::channel_type::handle   _on_appled_trx_handle;

         void async_add_session( std::weak_ptr<session> wp ) {
            app().get_io_service().post( [wp,this]{
               if( auto l = wp.lock() ) {
                  _sessions[l.get()] = wp;
               }
            });
         }

         void on_session_close( const session* s ) {
            verify_strand_in_this_thread(app().get_io_service().get_executor(), __func__, __LINE__);
            auto itr = _sessions.find(s);
            if( _sessions.end() != itr )
               _sessions.erase(itr);
         }

         template<typename Call>
         void for_each_session( Call callback ) {
            app().get_io_service().post([this, callback = callback] {
               for (const auto& item : _sessions) {
                  if (auto ses = item.second.lock()) {
                     ses->_ios.post(boost::asio::bind_executor(
                           ses->_strand,
                           [ses, cb = callback]() { cb(ses); }
                     ));
                  }
               }
            });
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

         void on_accepted_block_header( block_state_ptr s ) {
            _ioc->post( [s,this] { /// post this to the thread pool because packing can be intensive
               for_each_session( [s]( auto ses ){ ses->on_accepted_block_header( s ); } );
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
             verify_strand_in_this_thread(app().get_io_service().get_executor(), __func__, __LINE__);
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
                   auto s = std::make_shared<session>( *_ioc, shared_from_this() );
                   s->_local_peer_id = _peer_id;
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
        if( ec == boost::system::errc::too_many_files_open )
           do_accept();
        return;
     }
     std::shared_ptr<session> newsession;
     try {
        newsession = std::make_shared<session>( move( _socket ), _net_plugin );
     }
     catch( std::exception& e ) {
        //making a session creates an instance of std::random_device which may open /dev/urandom
        // for example. Unfortuately the only defined error is a std::exception derivative
        _socket.close();
     }
     if( newsession ) {
        _net_plugin->async_add_session( newsession );
        newsession->_local_peer_id = _net_plugin->_peer_id;
        newsession->run();
     }
     do_accept();
   }


   bnet_plugin::bnet_plugin()
   :my(std::make_shared<bnet_plugin_impl>()) {
   }

   bnet_plugin::~bnet_plugin() {
   }

   void bnet_plugin::set_program_options(options_description& cli, options_description& cfg) {
      cfg.add_options()
         ("bnet-endpoint", bpo::value<string>()->default_value("0.0.0.0:4321"), "the endpoint upon which to listen for incoming connections" )
         ("bnet-follow-irreversible", bpo::value<bool>()->default_value(false), "this peer will request only irreversible blocks from other nodes" )
         ("bnet-threads", bpo::value<uint32_t>(), "the number of threads to use to process network messages" )
         ("bnet-connect", bpo::value<vector<string>>()->composing(), "remote endpoint of other node to connect to; Use multiple bnet-connect options as needed to compose a network" )
         ("bnet-no-trx", bpo::bool_switch()->default_value(false), "this peer will request no pending transactions from other nodes" )
         ("bnet-peer-log-format", bpo::value<string>()->default_value( "[\"${_name}\" ${_ip}:${_port}]" ),
           "The string used to format peers when logging messages about them.  Variables are escaped with ${<variable name>}.\n"
           "Available Variables:\n"
           "   _name  \tself-reported name\n\n"
           "   _id    \tself-reported ID (Public Key)\n\n"
           "   _ip    \tremote IP address of peer\n\n"
           "   _port  \tremote port number of peer\n\n"
           "   _lip   \tlocal IP address connected to peer\n\n"
           "   _lport \tlocal port number connected to peer\n\n")
         ;
   }

   void bnet_plugin::plugin_initialize(const variables_map& options) {
      ilog( "Initialize bnet plugin" );

      try {
         peer_log_format = options.at( "bnet-peer-log-format" ).as<string>();

         if( options.count( "bnet-endpoint" )) {
            auto ip_port = options.at( "bnet-endpoint" ).as<string>();

            //auto host = boost::asio::ip::host_name(ip_port);
            auto port = ip_port.substr( ip_port.find( ':' ) + 1, ip_port.size());
            auto host = ip_port.substr( 0, ip_port.find( ':' ));
            my->_bnet_endpoint_address = host;
            my->_bnet_endpoint_port = std::stoi( port );
            idump((ip_port)( host )( port )( my->_follow_irreversible ));
         }
         if( options.count( "bnet-follow-irreversible" )) {
            my->_follow_irreversible = options.at( "bnet-follow-irreversible" ).as<bool>();
         }


         if( options.count( "bnet-connect" )) {
            my->_connect_to_peers = options.at( "bnet-connect" ).as<vector<string>>();
         }
         if( options.count( "bnet-threads" )) {
            my->_num_threads = options.at( "bnet-threads" ).as<uint32_t>();
            if( my->_num_threads > 8 )
               my->_num_threads = 8;
         }
         my->_request_trx = !options.at( "bnet-no-trx" ).as<bool>();

      } FC_LOG_AND_RETHROW()
   }

   void bnet_plugin::plugin_startup() {
      if(fc::get_logger_map().find(logger_name) != fc::get_logger_map().end())
         plugin_logger = fc::get_logger_map()[logger_name];

      wlog( "bnet startup " );

      my->_on_appled_trx_handle = app().get_channel<channels::accepted_transaction>()
                                .subscribe( [this]( transaction_metadata_ptr t ){
                                       my->on_accepted_transaction(t);
                                });

      my->_on_irb_handle = app().get_channel<channels::irreversible_block>()
                                .subscribe( [this]( block_state_ptr s ){
                                       my->on_irreversible_block(s);
                                });

      my->_on_accepted_block_header_handle = app().get_channel<channels::accepted_block_header>()
                                         .subscribe( [this]( block_state_ptr s ){
                                                my->on_accepted_block_header(s);
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
                                                  my );
      my->_listener->run();

      my->_socket_threads.reserve( my->_num_threads );
      for( auto i = 0; i < my->_num_threads; ++i ) {
         my->_socket_threads.emplace_back( [&ioc]{ wlog( "start thread" ); ioc.run(); wlog( "end thread" ); } );
      }

      for( const auto& peer : my->_connect_to_peers ) {
         auto s = std::make_shared<session>( ioc, my );
         s->_local_peer_id = my->_peer_id;
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

      /// shut down all threads and close all connections

      my->for_each_session([](auto ses){
         ses->do_goodbye( "shutting down" );
      });

      my->_listener.reset();
      my->_ioc->stop();

      wlog( "joining bnet threads" );
      for( auto& t : my->_socket_threads ) {
         t.join();
      }
      wlog( "done joining threads" );

      my->for_each_session([](auto ses){
         EOS_ASSERT( false, plugin_exception, "session ${ses} still active", ("ses", ses->_session_num) );
      });

      // lifetime of _ioc is guarded by shared_ptr of bnet_plugin_impl
   }


   session::~session() {
     wlog( "close session ${n}",("n",_session_num) );
     std::weak_ptr<bnet_plugin_impl> netp = _net_plugin;
     _app_ios.post( [netp,ses=this]{
        if( auto net = netp.lock() )
           net->on_session_close(ses);
     });
   }

   void session::do_hello() {
      /// TODO: find more effecient way to move large array of ids in event of fork
      async_get_pending_block_ids( [self = shared_from_this() ]( const vector<block_id_type>& ids, uint32_t lib ){
          hello hello_msg;
          hello_msg.peer_id = self->_local_peer_id;
          hello_msg.last_irr_block_num = lib;
          hello_msg.pending_block_ids  = ids;
          hello_msg.request_transactions = self->_net_plugin->_request_trx;
          hello_msg.chain_id = app().get_plugin<chain_plugin>().get_chain_id(); // TODO: Quick fix in a rush. Maybe a better solution is needed.

          self->_local_lib = lib;
          if ( self->_net_plugin->_follow_irreversible ) {
             self->send( hello_msg, hello_extension(hello_extension_irreversible_only()) );
          } else {
             self->send( hello_msg );
          }
          self->_sent_remote_hello = true;
      });
   }

   void session::check_for_redundant_connection() {
     app().get_io_service().post( [self=shared_from_this()]{
       self->_net_plugin->for_each_session( [self]( auto ses ){
         if( ses != self && ses->_remote_peer_id == self->_remote_peer_id ) {
           self->do_goodbye( "redundant connection" );
         }
       });
     });
   }

   void session::on( const hello& hi, fc::datastream<const char*>& ds ) {
      peer_ilog(this, "received hello");
      _recv_remote_hello     = true;

      if( hi.chain_id != app().get_plugin<chain_plugin>().get_chain_id() ) { // TODO: Quick fix in a rush. Maybe a better solution is needed.
         peer_elog(this, "bad hello : wrong chain id");
         return do_goodbye( "disconnecting due to wrong chain id" );
      }

      if( hi.peer_id == _local_peer_id ) {
         return do_goodbye( "connected to self" );
      }

      if ( _net_plugin->_follow_irreversible && hi.protocol_version <= "1.0.0") {
         return do_goodbye( "need newer protocol version that supports sending only irreversible blocks" );
      }

      if ( hi.protocol_version >= "1.0.1" ) {
         //optional extensions
         while ( 0 < ds.remaining() ) {
            unsigned_int size;
            fc::raw::unpack( ds, size ); // next extension size
            auto ex_start = ds.pos();
            fc::datastream<const char*> dsw( ex_start, size );
            unsigned_int wich;
            fc::raw::unpack( dsw, wich );
            hello_extension ex;
            if ( wich < ex.count() ) { //know extension
               fc::datastream<const char*> dsx( ex_start, size ); //unpack needs to read static_variant _tag again
               fc::raw::unpack( dsx, ex );
               if ( ex.which() == hello_extension::tag<hello_extension_irreversible_only>::value ) {
                  _remote_request_irreversible_only = true;
               }
            } else {
               //unsupported extension, we just ignore it
               //another side does know our protocol version, i.e. it know which extensions we support
               //so, it some extensions were crucial, another side will close the connection
            }
            ds.skip(size); //move to next extension
         }
      }

      _last_sent_block_num   = hi.last_irr_block_num;
      _remote_request_trx    = hi.request_transactions;
      _remote_peer_id        = hi.peer_id;
      _remote_lib            = hi.last_irr_block_num;

      for( const auto& id : hi.pending_block_ids )
         mark_block_known_by_peer( id );

      check_for_redundant_connection();

   }

} /// namespace eosio
