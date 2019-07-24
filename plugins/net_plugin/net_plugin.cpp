/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/chain/types.hpp>

#include <eosio/net_plugin/net_plugin.hpp>
#include <eosio/net_plugin/protocol.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain/contract_types.hpp>
#include <eosio/topology_plugin/topology_plugin.hpp>

#include <fc/network/message_buffer.hpp>
#include <fc/network/ip.hpp>
#include <fc/io/json.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/appender.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/exception/exception.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/steady_timer.hpp>

#include <atomic>
#include <shared_mutex>

using namespace eosio::chain::plugin_interface;

namespace eosio {
   static appbase::abstract_plugin& _net_plugin = app().register_plugin<net_plugin>();

   using std::vector;

   using boost::asio::ip::tcp;
   using boost::asio::ip::address_v4;
   using boost::asio::ip::host_name;
   using boost::multi_index_container;

   using fc::time_point;
   using fc::time_point_sec;
   using eosio::chain::transaction_id_type;
   using eosio::chain::sha256_less;

   class connection;

   using connection_ptr = std::shared_ptr<connection>;
   using connection_wptr = std::weak_ptr<connection>;

   using io_work_t = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

   template<typename T>
   T dejsonify(const string& s) {
      return fc::json::from_string(s).as<T>();
   }

   template <typename Strand>
   void verify_strand_in_this_thread(const Strand& strand, const char* func, int line) {
      if( !strand.running_in_this_thread() ) {
         elog( "wrong strand: ${f} : line ${n}, exiting", ("f", func)("n", line) );
         app().quit();
      }
   }

   struct node_transaction_state {
      transaction_id_type id;
      time_point_sec  expires;        /// time after which this may be purged.
      uint32_t        block_num = 0;  /// block transaction was included in
      uint32_t        connection_id = 0;
   };

   struct by_expiry;
   struct by_block_num;

   typedef multi_index_container<
      node_transaction_state,
      indexed_by<
         ordered_unique<
            tag<by_id>,
            composite_key< node_transaction_state,
               member<node_transaction_state, transaction_id_type, &node_transaction_state::id>,
               member<node_transaction_state, uint32_t, &node_transaction_state::connection_id>
            >,
            composite_key_compare< sha256_less, std::less<uint32_t> >
         >,
         ordered_non_unique<
            tag< by_expiry >,
            member< node_transaction_state, fc::time_point_sec, &node_transaction_state::expires > >,
         ordered_non_unique<
            tag<by_block_num>,
            member< node_transaction_state, uint32_t, &node_transaction_state::block_num > >
         >
      >
   node_transaction_index;

   struct peer_block_state {
      block_id_type id;
      uint32_t      block_num = 0;
      uint32_t      connection_id = 0;
      bool          have_block = false; // true if we have received the block, false if only received id notification
   };

   struct by_block_id;

   typedef multi_index_container<
      eosio::peer_block_state,
      indexed_by<
         ordered_unique< tag<by_id>,
               composite_key< peer_block_state,
                     member<peer_block_state, uint32_t, &eosio::peer_block_state::connection_id>,
                     member<peer_block_state, block_id_type, &eosio::peer_block_state::id>
               >,
               composite_key_compare< std::less<uint32_t>, sha256_less >
         >,
         ordered_non_unique< tag<by_block_id>,
               composite_key< peer_block_state,
                     member<peer_block_state, block_id_type, &eosio::peer_block_state::id>,
                     member<peer_block_state, bool, &eosio::peer_block_state::have_block>
               >,
               composite_key_compare< sha256_less, std::greater<bool> >
         >,
         ordered_non_unique< tag<by_block_num>, member<eosio::peer_block_state, uint32_t, &eosio::peer_block_state::block_num > >
      >
      > peer_block_state_index;


   struct update_block_num {
      uint32_t new_bnum;
      update_block_num(uint32_t bnum) : new_bnum(bnum) {}
      void operator() (node_transaction_state& nts) {
         nts.block_num = new_bnum;
      }
   };

   class sync_manager {
   private:
      enum stages {
         lib_catchup,
         head_catchup,
         in_sync
      };

      mutable std::mutex sync_mtx;
      uint32_t       sync_known_lib_num;
      uint32_t       sync_last_requested_num;
      uint32_t       sync_next_expected_num;
      uint32_t       sync_req_span;
      connection_ptr sync_source;
      std::atomic<stages> sync_state;

      chain_plugin* chain_plug = nullptr;

   private:
      constexpr static auto stage_str( stages s );
      void set_state( stages s );
      bool is_sync_required( uint32_t fork_head_block_num );
      void request_next_chunk( std::unique_lock<std::mutex> g_sync, const connection_ptr& conn = connection_ptr() );
      void start_sync( const connection_ptr& c, uint32_t target );
      bool verify_catchup( const connection_ptr& c, uint32_t num, const block_id_type& id );

   public:
      explicit sync_manager( uint32_t span );
      static void send_handshakes();
      bool syncing_with_peer() const { return sync_state == lib_catchup; }
      bool block_while_syncing_with_other_peer( const connection_ptr& c ) const;
      void sync_reset_lib_num( const connection_ptr& conn );
      void sync_reassign_fetch( const connection_ptr& c, go_away_reason reason );
      void rejected_block( const connection_ptr& c, uint32_t blk_num );
      void sync_recv_block( const connection_ptr& c, const block_id_type& blk_id, uint32_t blk_num );
      void recv_handshake( const connection_ptr& c, const handshake_message& msg );
      void sync_recv_notice( const connection_ptr& c, const notice_message& msg );
   };

   class dispatch_manager {
      mutable std::mutex      blk_state_mtx;
      peer_block_state_index  blk_state;
      mutable std::mutex      local_txns_mtx;
      node_transaction_index  local_txns;

   public:
      boost::asio::io_context::strand  strand;

      explicit dispatch_manager(boost::asio::io_context& io_context)
      : strand( io_context ) {}

      void bcast_topology_message (const topology_message& tm);

      void bcast_transaction(const transaction_metadata_ptr& trx);
      void rejected_transaction(const transaction_id_type& msg, uint32_t head_blk_num);
      void bcast_block(const block_state_ptr& bs);
      void bcast_notice( const block_id_type& id );
      void rejected_block(const block_id_type& id);

      void recv_block(const connection_ptr& conn, const block_id_type& msg, uint32_t bnum);
      void expire_blocks( uint32_t bnum );
      void recv_transaction(const connection_ptr& conn, const transaction_metadata_ptr& txn);
      void recv_notice(const connection_ptr& conn, const notice_message& msg, bool generated);

      void retry_fetch(const connection_ptr& conn);

      bool add_peer_block( const block_id_type& blkid, uint32_t connection_id );
      bool add_peer_block_id( const block_id_type& blkid, uint32_t connection_id );
      bool peer_has_block(const block_id_type& blkid, uint32_t connection_id) const;
      bool have_block(const block_id_type& blkid) const;
      size_t num_entries( uint32_t connection_id ) const;

      bool add_peer_txn( const node_transaction_state& nts );
      void update_txns_block_num( const signed_block_ptr& sb );
      void update_txns_block_num( const transaction_id_type& id, uint32_t blk_num );
      bool peer_has_txn( const transaction_id_type& tid, uint32_t connection_id ) const;
      bool have_txn( const transaction_id_type& tid ) const;
      void expire_txns( uint32_t lib_num );
   };

   class net_plugin_impl : public std::enable_shared_from_this<net_plugin_impl> {
   public:
      unique_ptr<tcp::acceptor>        acceptor;
      std::atomic<uint32_t>            current_connection_id{0};

      unique_ptr< sync_manager >       sync_master;
      unique_ptr< dispatch_manager >   dispatcher;

      /**
       * Thread safe, only updated in plugin initialize
       *  @{
       */
      string                                p2p_address;
      string                                p2p_server_address;

      vector<string>                        supplied_peers;
      vector<chain::public_key_type>        allowed_peers; ///< peer keys allowed to connect
      std::map<chain::public_key_type,
               chain::private_key_type>     private_keys; ///< overlapping with producer keys, also authenticating non-producing nodes
      enum possible_connections : char {
         None = 0,
            Producers = 1 << 0,
            Specified = 1 << 1,
            Any = 1 << 2
            };
      possible_connections                  allowed_connections{None};

      boost::asio::steady_timer::duration   connector_period;
      boost::asio::steady_timer::duration   txn_exp_period;
      boost::asio::steady_timer::duration   resp_expected_period;
      boost::asio::steady_timer::duration   keepalive_interval{std::chrono::seconds{32}};
      boost::asio::steady_timer::duration   sample_interval;

      int                                   max_cleanup_time_ms = 0;
      uint32_t                              max_client_count = 0;
      uint32_t                              max_nodes_per_host = 1;

      /// Peer clock may be no more than 1 second skewed from our clock, including network latency.
      const std::chrono::system_clock::duration peer_authentication_interval{std::chrono::seconds{1}};

      bool                                  network_version_match = false;
      chain_id_type                         chain_id;
      fc::sha256                            node_id;
      string                                user_agent_name;

      eosio::db_read_mode                   db_read_mode = eosio::db_read_mode::SPECULATIVE;
      chain_plugin*                         chain_plug = nullptr;
      producer_plugin*                      producer_plug = nullptr;
      topology_plugin*                      topology_plug = nullptr;
      bool                                  use_socket_read_watermark = false;
      /** @} */

      mutable std::shared_mutex             connections_mtx;
      std::set< connection_ptr >            connections;     // todo: switch to a thread safe container to avoid big mutex over complete collection

      std::mutex                            connector_check_timer_mtx;
      unique_ptr<boost::asio::steady_timer> connector_check_timer;
      int                                   connector_checks_in_flight{0};

      std::mutex                            expire_timer_mtx;
      unique_ptr<boost::asio::steady_timer> expire_timer;

      std::mutex                            keepalive_timer_mtx;
      unique_ptr<boost::asio::steady_timer> keepalive_timer;

      std::mutex                            sample_timer_mtx;
      unique_ptr<boost::asio::steady_timer> sample_timer;

      std::atomic<bool>                     in_shutdown{false};

      compat::channels::transaction_ack::channel_type::handle  incoming_transaction_ack_subscription;
      channels::irreversible_block::channel_type::handle       incoming_irreversible_block_subscription;

      uint16_t                                  thread_pool_size = 2;
      optional<eosio::chain::named_thread_pool> thread_pool;

   private:
      mutable std::mutex            chain_info_mtx; // protects chain_*
      uint32_t                      chain_lib_num{0};
      uint32_t                      chain_head_blk_num{0};
      uint32_t                      chain_fork_head_blk_num{0};
      block_id_type                 chain_lib_id;
      block_id_type                 chain_head_blk_id;
      block_id_type                 chain_fork_head_blk_id;

   public:
      void update_chain_info();
      //         lib_num, head_block_num, fork_head_blk_num, lib_id, head_blk_id, fork_head_blk_id
      std::tuple<uint32_t, uint32_t, uint32_t, block_id_type, block_id_type, block_id_type> get_chain_info() const;

      void start_listen_loop();

      void on_topology_update( const topology_message& tm );
      void on_accepted_block( const block_state_ptr& bs );
      void transaction_ack(const std::pair<fc::exception_ptr, transaction_metadata_ptr>&);
      void on_irreversible_block( const block_state_ptr& blk );

      void start_conn_timer(boost::asio::steady_timer::duration du, std::weak_ptr<connection> from_connection);
      void start_expire_timer();
      void start_monitors();

      void expire();
      void connection_monitor(std::weak_ptr<connection> from_connection, bool reschedule);
      /** \name Peer Timestamps
       *  Time message handling
       *  @{
       */
      /** \brief Peer heartbeat ticker.
       */
      void ticker();
      /** \brief Topology monitor sample metrics collection strobe
       */
      void strobe();

      /** @} */
      /** \brief Determine if a peer is allowed to connect.
       *
       * Checks current connection mode and key authentication.
       *
       * \return False if the peer should not connect, true otherwise.
       */
      bool authenticate_peer(const handshake_message& msg) const;
      /** \brief Retrieve public key used to authenticate with peers.
       *
       * Finds a key to use for authentication.  If this node is a producer, use
       * the front of the producer key map.  If the node is not a producer but has
       * a configured private key, use it.  If the node is neither a producer nor has
       * a private key, returns an empty key.
       *
       * \note On a node with multiple private keys configured, the key with the first
       *       numerically smaller byte will always be used.
       */
      chain::public_key_type get_authentication_key() const;
      /** \brief Returns a signature of the digest using the corresponding private key of the signer.
       *
       * If there are no configured private keys, returns an empty signature.
       */
      chain::signature_type sign_compact(const chain::public_key_type& signer, const fc::sha256& digest) const;

      constexpr uint16_t to_protocol_version(uint16_t v);

      connection_ptr find_connection(const string& host)const; // must call with held mutex
   };

   const fc::string logger_name("net_plugin_impl");
   fc::logger logger;
   std::string peer_log_format;

#define peer_dlog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( logger.is_enabled( fc::log_level::debug ) ) \
      logger.log( FC_LOG_MESSAGE( debug, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_ilog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( logger.is_enabled( fc::log_level::info ) ) \
      logger.log( FC_LOG_MESSAGE( info, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_wlog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( logger.is_enabled( fc::log_level::warn ) ) \
      logger.log( FC_LOG_MESSAGE( warn, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant()) ) ); \
  FC_MULTILINE_MACRO_END

#define peer_elog( PEER, FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( logger.is_enabled( fc::log_level::error ) ) \
      logger.log( FC_LOG_MESSAGE( error, peer_log_format + FORMAT, __VA_ARGS__ (PEER->get_logger_variant())) ); \
  FC_MULTILINE_MACRO_END


   template<class enum_type, class=typename std::enable_if<std::is_enum<enum_type>::value>::type>
   inline enum_type& operator|=(enum_type& lhs, const enum_type& rhs)
   {
      using T = std::underlying_type_t <enum_type>;
      return lhs = static_cast<enum_type>(static_cast<T>(lhs) | static_cast<T>(rhs));
   }

   static net_plugin_impl *my_impl;

   /**
    * default value initializers
    */
   constexpr auto     def_send_buffer_size_mb = 4;
   constexpr auto     def_send_buffer_size = 1024*1024*def_send_buffer_size_mb;
   constexpr auto     def_max_write_queue_size = def_send_buffer_size*10;
   constexpr boost::asio::chrono::milliseconds def_read_delay_for_full_write_queue{100};
   constexpr auto     def_max_trx_in_progress_size = 100*1024*1024; // 100 MB
   constexpr auto     def_max_read_delays = 100; // number of times read_delay_timer started without any reads
   constexpr auto     def_max_consecutive_rejected_blocks = 3; // num of rejected blocks before disconnect
   constexpr auto     def_max_consecutive_immediate_connection_close = 9; // back off if client keeps closing
   constexpr auto     def_max_peer_block_ids_per_connection = 100*1024; // if we reach this many then the connection is spaming us, disconnect
   constexpr auto     def_max_clients = 25; // 0 for unlimited clients
   constexpr auto     def_max_nodes_per_host = 1;
   constexpr auto     def_conn_retry_wait = 30;
   constexpr auto     def_txn_expire_wait = std::chrono::seconds(3);
   constexpr auto     def_resp_expected_wait = std::chrono::seconds(5);
   constexpr auto     def_sync_fetch_span = 100;

   constexpr auto     message_header_size = 4;
   constexpr uint32_t signed_block_which = 7;        // see protocol net_message
   constexpr uint32_t packed_transaction_which = 8;  // see protocol net_message
   constexpr uint32_t topology_message_which = 9;    // see protocol net_message

   /**
    *  For a while, network version was a 16 bit value equal to the second set of 16 bits
    *  of the current build's git commit id. We are now replacing that with an integer protocol
    *  identifier. Based on historical analysis of all git commit identifiers, the larges gap
    *  between ajacent commit id values is shown below.
    *  these numbers were found with the following commands on the master branch:
    *
    *  git log | grep "^commit" | awk '{print substr($2,5,4)}' | sort -u > sorted.txt
    *  rm -f gap.txt; prev=0; for a in $(cat sorted.txt); do echo $prev $((0x$a - 0x$prev)) $a >> gap.txt; prev=$a; done; sort -k2 -n gap.txt | tail
    *
    *  DO NOT EDIT net_version_base OR net_version_range!
    */
   constexpr uint16_t net_version_base = 0x04b5;
   constexpr uint16_t net_version_range = 106;
   /**
    *  If there is a change to network protocol or behavior, increment net version to identify
    *  the need for compatibility hooks
    */
   constexpr uint16_t proto_base = 0;
   constexpr uint16_t proto_explicit_sync = 1;
   constexpr uint16_t block_id_notify = 2;
   constexpr uint16_t proto_topology = 3;

   constexpr uint16_t net_version = proto_topology;

   constexpr auto  time_placeholder = "-*-TIME-*-";
   constexpr auto  topology_enabler = "topo:";
   constexpr size_t   time_placeholder_len = 10; //strlen(time_placeholder);

   /** \brief Read system time and convert to a 64 bit integer.
    *
    * There are only two calls on this routine in the program.  One
    * when a packet arrives from the network and the other when a
    * packet is placed on the send queue.  Calls the kernel time of
    * day routine and converts to a (at least) 64 bit integer.
    */
   static tstamp get_tstamp() {
      return std::chrono::system_clock::now().time_since_epoch().count();
   }
   /** @} */

   /**
    * Index by start_block_num
    */
   struct sync_state {
      sync_state(uint32_t start = 0, uint32_t end = 0, uint32_t last_acted = 0)
         :start_block( start ), end_block( end ), last( last_acted ),
          start_time(time_point::now())
      {}
      uint32_t     start_block;
      uint32_t     end_block;
      uint32_t     last; ///< last sent or received
      time_point   start_time; ///< time request made or received
   };

   // thread safe
   class queued_buffer : boost::noncopyable {
   public:
      void clear_write_queue() {
         std::lock_guard<std::mutex> g( _mtx );
         _write_queue.clear();
         _sync_write_queue.clear();
         _write_queue_size = 0;
      }

      void clear_out_queue() {
         std::lock_guard<std::mutex> g( _mtx );
         while ( _out_queue.size() > 0 ) {
            _out_queue.pop_front();
         }
      }

      uint32_t write_queue_size() const {
         std::lock_guard<std::mutex> g( _mtx );
         return _write_queue_size;
      }

      bool is_out_queue_empty() const {
         std::lock_guard<std::mutex> g( _mtx );
         return _out_queue.empty();
      }

      bool ready_to_send() const {
         std::lock_guard<std::mutex> g( _mtx );
         // if out_queue is not empty then async_write is in progress
         return ((!_sync_write_queue.empty() || !_write_queue.empty()) && _out_queue.empty());
      }

      // @param callback must not callback into queued_buffer
      bool add_write_queue( const std::shared_ptr<vector<char>>& buff,
                            std::function<void( boost::system::error_code, std::size_t )> callback,
                            bool to_sync_queue ) {
         std::lock_guard<std::mutex> g( _mtx );
         if( to_sync_queue ) {
            _sync_write_queue.push_back( {buff, callback} );
         } else {
            _write_queue.push_back( {buff, callback} );
         }
         _write_queue_size += buff->size();
         if( _write_queue_size > 2 * def_max_write_queue_size ) {
            return false;
         }
         return true;
      }

      void fill_out_buffer( std::vector<boost::asio::const_buffer>& bufs,
                            uint32_t& bytes,
                            uint32_t&msgs,
                            int do_xmt_time ) {
         std::lock_guard<std::mutex> g( _mtx );
         bytes = _write_queue_size;
         if( _sync_write_queue.size() > 0 ) { // always send msgs from sync_write_queue first
            msgs = _sync_write_queue.size();
            fill_out_buffer( bufs, _sync_write_queue, 0 );
         } else { // postpone real_time write_queue if sync queue is not empty
            msgs = _write_queue.size();
            fill_out_buffer( bufs, _write_queue, do_xmt_time );
            EOS_ASSERT( _write_queue_size == 0, plugin_exception, "write queue size expected to be zero" );
         }

      }

     void out_callback( boost::system::error_code ec, std::size_t w ) {
         std::lock_guard<std::mutex> g( _mtx );
         for( auto& m : _out_queue ) {
            m.callback( ec, w );
         }
      }

   private:
      struct queued_write;
      void fill_out_buffer( std::vector<boost::asio::const_buffer>& bufs,
                            deque<queued_write>& w_queue, int do_xmt_time ) {

         constexpr size_t header_size = sizeof(uint32_t);
         constexpr size_t time_data_offset = time_placeholder_len + header_size;
         constexpr size_t time_msg_size = time_data_offset + sizeof(time_message);
         while ( w_queue.size() > 0 ) {
            auto& m = w_queue.front();
            size_t msize = m.buff->size();
            if (do_xmt_time &&
                m.buff->size() == time_msg_size &&
                strncmp(time_placeholder, m.buff->data() + header_size, time_placeholder_len) == 0) {
               time_message* tm = reinterpret_cast<time_message *>(m.buff->data() + time_data_offset);
               tm->xmt = get_tstamp();
               const uint32_t payload_size = fc::raw::pack_size( *tm );
               const char* const header = reinterpret_cast<const char* const>(&payload_size);
               static_assert( header_size == message_header_size, "invalid message_header_size" );
               const size_t buffer_size = header_size + payload_size;

               auto send_buffer = std::make_shared<vector<char>>(buffer_size);
               fc::datastream<char*> ds( send_buffer->data(), buffer_size);
               ds.write( header, header_size );
               fc::raw::pack( ds, tm );
               m.buff = send_buffer;
               --do_xmt_time;
            }

            bufs.push_back( boost::asio::buffer( *m.buff ));
            _out_queue.emplace_back( m );

            _write_queue_size -= msize;
            w_queue.pop_front();
         }
      }

   private:
      struct queued_write {
         std::shared_ptr<vector<char>> buff;
         std::function<void( boost::system::error_code, std::size_t )> callback;
      };

      mutable std::mutex  _mtx;
      uint32_t            _write_queue_size{0};
      deque<queued_write> _write_queue;
      deque<queued_write> _sync_write_queue; // sync_write_queue will be sent first
      deque<queued_write> _out_queue;

   }; // queued_buffer


   class connection : public std::enable_shared_from_this<connection> {
   public:
      explicit connection( string endpoint );
      connection();

      ~connection() {}

      bool start_session();

      bool socket_is_open() const { return socket_open.load(); } // thread safe, atomic
      const string& peer_address() const { return peer_addr; } // thread safe, const
      // thread safe, not updated after start_session()
      const string& remote_address() const { return socket_open.load() ? remote_endpoint_ip : unknown; }

      void set_connection_type( const string& peer_addr );
      bool is_transactions_only_connection()const { return connection_type == transactions_only; }
      bool is_blocks_only_connection()const { return connection_type == blocks_only; }

   private:
      static const string unknown;

      void update_endpoints();

      optional<sync_state>    peer_requested;  // this peer is requesting info from us

      std::atomic<bool>       socket_open{false};

      const string            peer_addr;
      string                  remote_endpoint_ip;     // not updated after start
      string                  remote_endpoint_port;   // not updated after start
      string                  local_endpoint_ip;      // not updated after start
      string                  local_endpoint_port;    // not updated after start

      enum connection_types : char {
         both,
         transactions_only,
         blocks_only
      };

      std::atomic<connection_types>             connection_type{both};

   public:
      boost::asio::io_context::strand           strand;
      std::shared_ptr<tcp::socket>              socket; // only accessed through strand after construction

      fc::message_buffer<1024*1024>    pending_message_buffer;
      std::atomic<std::size_t>         outstanding_read_bytes{0}; // accessed only from strand threads

      queued_buffer           buffer_queue;

      std::atomic<uint32_t>   trx_in_progress_size{0};
      fc::sha256              node_id;
      const uint32_t          connection_id;
      int16_t                 sent_handshake_count = 0;
      std::atomic<bool>       connecting{true};
      std::atomic<bool>       syncing{false};
      uint16_t                protocol_version = 0;
      uint16_t                consecutive_rejected_blocks = 0;
      std::atomic<uint16_t>   consecutive_immediate_connection_close = 0;

      std::mutex                            response_expected_timer_mtx;
      boost::asio::steady_timer             response_expected_timer;

      uint16_t                              read_delay_count = 0; // only accessed from strand
      std::mutex                            read_delay_timer_mtx;
      boost::asio::steady_timer             read_delay_timer;

      std::atomic<go_away_reason>           no_retry{no_reason};

      mutable std::mutex          conn_mtx; //< mtx for last_req, last_handshake_recv, last_handshake_sent, fork_head, fork_head_num, last_close
      optional<request_message>   last_req;
      handshake_message           last_handshake_recv;
      handshake_message           last_handshake_sent;
      block_id_type               fork_head;
      uint32_t                    fork_head_num{0};
      fc::time_point              last_close;
      link_id                     topo_id;
      bool                        peer_lacks_topology = true;

      connection_status get_status()const;

      /** \name Peer Timestamps
       *  Time message handling
       *  @{
       */
      // Members set from network data
      time_message                   last_time_recv; //!< a clone of the last time message received
      time_message                   last_time_sent; //!< a clone of the last time message sent
      tstamp                         rec{0}; //!< receive timestamp
      /** @} */

      // data samples for the topology plugin
      mutable std::mutex       sample_mtx;
      uint32_t                 queue_hwm{0};               //!< high water mark for the outbound queue
      uint32_t                 net_latency_sample{0};      //!< measured propagation delay peer to peer
      uint32_t                 bytes_sent_sample{0};       //!< count of octets sent to peer during this sample
      uint32_t                 messages_sent_sample{0};    //!< count of messages sent to peer during this sample
      uint32_t                 fork_instances_sample{0};   //!< count of instances when a fork was detected
      uint32_t                 fork_depth_sample{0};       //!< the depth of the current fork
      uint32_t                 fork_hwm{0};                //!< the depth of the longest fork detected
      int                      time_message_queued{0}; //!< the buffer stuffer needs to fix up xmt in a time_message

      bool connected();
      bool current();
      void close( bool reconnect = true );
   private:
      static void _close( connection* self, bool reconnect ); // for easy capture
   public:

      void populate_handshake( handshake_message& hello );

      bool resolve_and_connect();
      void connect( const std::shared_ptr<tcp::resolver>& resolver, tcp::resolver::results_type endpoints );
      void start_read_message();

      /** \brief Process the next message from the pending message buffer
       *
       * Process the next message from the pending_message_buffer.
       * message_length is the already determined length of the data
       * part of the message that will handle the message.
       * Returns true is successful. Returns false if an error was
       * encountered unpacking or processing the message.
       */
      bool process_next_message(uint32_t message_length);

      void send_handshake();

      void collect_samples ();

      /** \name Peer Timestamps
       *  Time message handling
       */
      /**  \brief Populate and queue time_message
       */
      void send_time();
      /** \brief Populate and queue time_message immediately using incoming time_message
       */
      void send_time(const time_message& msg);

      const string peer_name();

      void blk_send_branch();
      void blk_send(const block_id_type& blkid);
      void stop_send();

      void enqueue( const net_message &msg, bool trigger_send = true );
      void enqueue_time( const time_message &msg, bool trigger_send = true );
      void enqueue_block( const signed_block_ptr& sb, bool trigger_send = true, bool to_sync_queue = false);
      void enqueue_buffer( const std::shared_ptr<std::vector<char>>& send_buffer,
                           bool trigger_send, int priority, go_away_reason close_after_send,
                           bool to_sync_queue = false);
      void cancel_sync(go_away_reason);
      void flush_queues();
      bool enqueue_sync_block();
      void request_sync_blocks(uint32_t start, uint32_t end);

      void cancel_wait();
      void sync_wait();
      void fetch_wait();
      void sync_timeout(boost::system::error_code ec);
      void fetch_timeout(boost::system::error_code ec);
      /**
       * periodically forward metrics samples to the topology manager if present.
       */
      void strobe_metrics(boost::system::error_code ec);

      void queue_write(const std::shared_ptr<vector<char>>& buff,
                       bool trigger_send,
                       int priority,
                       std::function<void(boost::system::error_code, std::size_t)> callback,
                       bool to_sync_queue = false);
      void do_queue_write(int priority);

      static bool is_valid( const handshake_message& msg );

      void handle_message( const handshake_message& msg );
      void handle_message( const chain_size_message& msg );
      void handle_message( const go_away_message& msg );
      /** \name Peer Timestamps
       *  Time message handling
       *  @{
       */
      /** \brief Process time_message
       *
       * Calculate offset, delay and dispersion.  Note carefully the
       * implied processing.  The first-order difference is done
       * directly in 64-bit arithmetic, then the result is converted
       * to floating double.  All further processing is in
       * floating-double arithmetic with rounding done by the hardware.
       * This is necessary in order to avoid overflow and preserve precision.
       */
      void handle_message( const time_message& msg );
      /** @} */
      void handle_message( const notice_message& msg );
      void handle_message( const request_message& msg );
      void handle_message( const sync_request_message& msg );
      void handle_message( const signed_block& msg ) = delete; // signed_block_ptr overload used instead
      void handle_message( const signed_block_ptr& msg );
      void handle_message( const packed_transaction& msg ) = delete; // packed_transaction_ptr overload used instead
      void handle_message( const packed_transaction_ptr& msg );

      void process_signed_block( const signed_block_ptr& msg );

      fc::optional<fc::variant_object> _logger_variant;
      const fc::variant_object& get_logger_variant()  {
         if (!_logger_variant) {
            _logger_variant.emplace(fc::mutable_variant_object()
               ("_name", peer_name())
               ("_id", node_id)
               ("_sid", ((string)node_id).substr(0, 7))
               ("_ip", remote_endpoint_ip)
               ("_port", remote_endpoint_port)
               ("_lip", local_endpoint_ip)
               ("_lport", local_endpoint_port)
            );
         }
         return *_logger_variant;
      }
   };

   const string connection::unknown = "<unknown>";


   // called from connection strand
   struct msg_handler : public fc::visitor<void> {
      connection_ptr c;
      explicit msg_handler( const connection_ptr& conn) : c(conn) {}

      void operator()( const signed_block& msg ) const {
         EOS_ASSERT( false, plugin_config_exception, "operator()(signed_block&&) should be called" );
      }
      void operator()( signed_block& msg ) const {
         EOS_ASSERT( false, plugin_config_exception, "operator()(signed_block&&) should be called" );
      }
      void operator()( const packed_transaction& msg ) const {
         EOS_ASSERT( false, plugin_config_exception, "operator()(packed_transaction&&) should be called" );
      }
      void operator()( packed_transaction& msg ) const {
         EOS_ASSERT( false, plugin_config_exception, "operator()(packed_transaction&&) should be called" );
      }
      void operator()( const topology_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle const topology_message" );
      }

      void operator()( signed_block&& msg ) const {
         // continue call to handle_message on connection strand
         shared_ptr<signed_block> ptr = std::make_shared<signed_block>( std::move( msg ) );
         c->handle_message( ptr );
      }

      void operator()( packed_transaction&& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle packed_transaction" );
         shared_ptr<packed_transaction> ptr = std::make_shared<packed_transaction>( std::move( msg ) );
         c->handle_message( ptr );
      }

      void operator()( topology_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle topology_message" );
         if ( my_impl->topology_plug != nullptr ) {
            my_impl->topology_plug->handle_message( msg );
         }
         else {
            fc_wlog( logger, "no topology plugin available to handle received topology message" );
         }
      }

      void operator()( const handshake_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle handshake_message" );
         c->handle_message( msg );
      }

      void operator()( const chain_size_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle chain_size_message" );
         c->handle_message( msg );
      }

      void operator()( const go_away_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle go_away_message" );
         c->handle_message( msg );
      }

      void operator()( const time_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle time_message" );
         c->handle_message( msg );
      }

      void operator()( const notice_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle notice_message" );
         c->handle_message( msg );
      }

      void operator()( const request_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle request_message" );
         c->handle_message( msg );
      }

      void operator()( const sync_request_message& msg ) const {
         // continue call to handle_message on connection strand
         fc_dlog( logger, "handle sync_request_message" );
         c->handle_message( msg );
      }
   };

   template<typename Function>
   void for_each_connection( Function f ) {
      std::shared_lock<std::shared_mutex> g( my_impl->connections_mtx );
      for( auto& c : my_impl->connections ) {
         if( !f( c ) ) return;
      }
   }

   template<typename Function>
   void for_each_block_connection( Function f ) {
      std::shared_lock<std::shared_mutex> g( my_impl->connections_mtx );
      for( auto& c : my_impl->connections ) {
         if( c->is_transactions_only_connection() ) continue;
         if( !f( c ) ) return;
      }
   }

   //---------------------------------------------------------------------------

   connection::connection( string endpoint )
      : peer_addr( endpoint ),
        strand( my_impl->thread_pool->get_executor() ),
        socket( new tcp::socket( my_impl->thread_pool->get_executor() ) ),
        connection_id( ++my_impl->current_connection_id ),
        response_expected_timer( my_impl->thread_pool->get_executor() ),
        node_id(),
        read_delay_timer( my_impl->thread_pool->get_executor() ),
        last_handshake_recv(),
        last_handshake_sent()
   {
      fc_ilog( logger, "creating connection to ${n}", ("n", endpoint) );
   }

   connection::connection()
      : peer_addr(),
        strand( my_impl->thread_pool->get_executor() ),
        socket( new tcp::socket( my_impl->thread_pool->get_executor() ) ),
        connection_id( ++my_impl->current_connection_id ),
        response_expected_timer( my_impl->thread_pool->get_executor() ),
        node_id(),
        read_delay_timer( my_impl->thread_pool->get_executor() ),
        last_handshake_recv(),
        last_handshake_sent()
   {
      fc_ilog( logger, "accepted network connection" );
   }

   void connection::update_endpoints() {
      boost::system::error_code ec;
      auto rep = socket->remote_endpoint(ec);
      remote_endpoint_ip = ec ? unknown : rep.address().to_string();
      remote_endpoint_port = ec ? unknown : std::to_string(rep.port());

      auto lep = socket->local_endpoint(ec);
      local_endpoint_ip = ec ? unknown : lep.address().to_string();
      local_endpoint_port = ec ? unknown : std::to_string(lep.port());
   }

   void connection::set_connection_type( const string& peer_add ) {
      // host:port:[<trx>|<blk>]
      string::size_type colon = peer_add.find(':');
      string::size_type colon2 = peer_add.find(':', colon + 1);
      string::size_type end = colon2 == string::npos
            ? string::npos : peer_add.find_first_of( " :+=.,<>!$%^&(*)|-#@\t", colon2 + 1 ); // future proof by including most symbols without using regex
      string host = peer_add.substr( 0, colon );
      string port = peer_add.substr( colon + 1, colon2 == string::npos ? string::npos : colon2 - (colon + 1));
      string type = colon2 == string::npos ? "" : end == string::npos ?
            peer_add.substr( colon2 + 1 ) : peer_add.substr( colon2 + 1, end - (colon2 + 1) );

      if( type.empty() ) {
         fc_ilog( logger, "Setting connection type for: ${peer} to both transactions and blocks", ("peer", peer_add) );
         connection_type = both;
      } else if( type == "trx" ) {
         fc_ilog( logger, "Setting connection type for: ${peer} to transactions only", ("peer", peer_add) );
         connection_type = transactions_only;
      } else if( type == "blk" ) {
         fc_ilog( logger, "Setting connection type for: ${peer} to blocks only", ("peer", peer_add) );
         connection_type = blocks_only;
      } else {
         fc_wlog( logger, "Unknown connection type: ${t}", ("t", type) );
      }
   }

   connection_status connection::get_status()const {
      connection_status stat;
      stat.peer = peer_addr;
      stat.connecting = connecting;
      stat.syncing = syncing;
      std::lock_guard<std::mutex> g( conn_mtx );
      stat.last_handshake = last_handshake_recv;
      return stat;
   }

   bool connection::start_session() {
      verify_strand_in_this_thread( strand, __func__, __LINE__ );

      update_endpoints();
      boost::asio::ip::tcp::no_delay nodelay( true );
      boost::system::error_code ec;
      socket->set_option( nodelay, ec );
      if( ec ) {
         fc_elog( logger, "connection failed (set_option) ${peer}: ${e1}", ("peer", peer_name())( "e1", ec.message() ) );
         close();
         return false;
      } else {
         fc_dlog( logger, "connected to ${peer}", ("peer", peer_name()) );
         socket_open = true;
         start_read_message();
         return true;
      }
   }

   bool connection::connected() {
      return socket_is_open() && !connecting;
   }

   bool connection::current() {
      return (connected() && !syncing);
   }

   void connection::flush_queues() {
      buffer_queue.clear_write_queue();
   }

   void connection::close( bool reconnect ) {
      strand.post( [self = shared_from_this(), reconnect]() {
         connection::_close( self.get(), reconnect );
      });
   }

   void connection::_close( connection* self, bool reconnect ) {
      self->socket_open = false;
      boost::system::error_code ec;
      if( self->socket->is_open() ) {
         self->socket->shutdown( tcp::socket::shutdown_both, ec );
         self->socket->close();
      }
      self->socket.reset( new tcp::socket( my_impl->thread_pool->get_executor() ) );
      self->flush_queues();
      self->connecting = false;
      self->syncing = false;
      self->consecutive_rejected_blocks = 0;
      ++self->consecutive_immediate_connection_close;
      bool has_last_req = false;
      {
         std::lock_guard<std::mutex> g_conn( self->conn_mtx );
         has_last_req = !!self->last_req;
         self->last_handshake_recv = handshake_message();
         self->last_handshake_sent = handshake_message();
         self->last_close = fc::time_point::now();
      }
      if( has_last_req ) {
         my_impl->dispatcher->retry_fetch( self->shared_from_this() );
      }
      self->peer_requested.reset();
      self->sent_handshake_count = 0;
      self->node_id = fc::sha256();
      my_impl->sync_master->sync_reset_lib_num( self->shared_from_this() );
      fc_dlog( logger, "closed, canceling wait on ${p}", ("p", self->peer_name()) ); // peer_name(), do not hold conn_mtx
      self->cancel_wait();
      {
         std::lock_guard<std::mutex> g( self->read_delay_timer_mtx );
         self->read_delay_timer.cancel();
         self->read_delay_count = 0;
      }

      if( reconnect ) {
         my_impl->start_conn_timer( std::chrono::milliseconds( 100 ), connection_wptr() );
      }
   }

   void connection::blk_send_branch() {
      uint32_t head_num = 0;
      block_id_type head_id;
      std::tie( std::ignore, std::ignore, head_num,
                std::ignore, std::ignore, head_id ) = my_impl->get_chain_info();

      fc_dlog(logger, "head_num = ${h}",("h",head_num));
      if(head_num == 0) {
         notice_message note;
         note.known_blocks.mode = normal;
         note.known_blocks.pending = 0;
         enqueue(note);
         return;
      }
      block_id_type remote_head_id;
      uint32_t remote_head_num = 0;
      std::unique_lock<std::mutex> g_conn( conn_mtx );
      if( last_handshake_recv.generation >= 1 ) {
         remote_head_id = last_handshake_recv.head_id;
         remote_head_num = block_header::num_from_id(remote_head_id);
         fc_dlog( logger, "maybe truncating branch at = ${h}:${id}", ("h", remote_head_num)( "id", remote_head_id ) );
      }

      block_id_type lib_id = last_handshake_recv.last_irreversible_block_id;
      g_conn.unlock();

      if( !peer_requested ) {
         peer_requested = sync_state( block_header::num_from_id(lib_id)+1,
                                      block_header::num_from_id(head_id),
                                      block_header::num_from_id(lib_id) );
      } else {
         uint32_t start = std::min( peer_requested->last + 1, block_header::num_from_id(lib_id)+1 );
         uint32_t end   = std::max( peer_requested->end_block, block_header::num_from_id(head_id) );
         peer_requested = sync_state( start, end, start - 1 );
      }
      if( peer_requested->start_block <= peer_requested->end_block ) {
         fc_dlog( logger, "enqueue ${s} - ${e}", ("s", peer_requested->start_block)( "e", peer_requested->end_block ) );
         enqueue_sync_block();
      } else {
         peer_requested.reset();
      }
   }

   void connection::blk_send( const block_id_type& blkid ) {
      connection_wptr weak = shared_from_this();
      app().post( priority::low, [blkid, weak{std::move(weak)}]() {
         connection_ptr c = weak.lock();
         if( !c ) return;
         try {
            controller& cc = my_impl->chain_plug->chain();
            signed_block_ptr b = cc.fetch_block_by_id( blkid );
            if( b ) {
               fc_dlog( logger, "found block for id at num ${n}", ("n", b->block_num()) );
               my_impl->dispatcher->add_peer_block( blkid, c->connection_id );
               c->strand.post( [c, b{std::move(b)}]() {
                  c->enqueue_block( b );
               } );
            } else {
               fc_ilog( logger, "fetch block by id returned null, id ${id} for ${p}",
                        ("id", blkid)( "p", c->peer_address() ) );
            }
         } catch( const assert_exception& ex ) {
            fc_elog( logger, "caught assert on fetch_block_by_id, ${ex}, id ${id} for ${p}",
                     ("ex", ex.to_string())( "id", blkid )( "p", c->peer_address() ) );
         } catch( ... ) {
            fc_elog( logger, "caught other exception fetching block id ${id} for ${p}",
                     ("id", blkid)( "p", c->peer_address() ) );
         }
      });
   }

   void connection::stop_send() {
      syncing = false;
   }

   void connection::send_handshake() {
      strand.post( [c = shared_from_this()]() {
         std::unique_lock<std::mutex> g_conn( c->conn_mtx );
         c->populate_handshake( c->last_handshake_sent );
         c->last_handshake_sent.generation = ++c->sent_handshake_count;
         auto last_handshake_sent = c->last_handshake_sent;
         g_conn.unlock();
         fc_dlog( logger, "Sending handshake generation ${g} to ${ep}, lib ${lib}, head ${head}",
                  ("g", last_handshake_sent.generation)( "ep", c->peer_name() )
                  ( "lib", last_handshake_sent.last_irreversible_block_num )
                  ( "head", last_handshake_sent.head_num ) );
         c->enqueue( last_handshake_sent );
      });
   }

   void connection::collect_samples() {
      link_sample ls;
      ls.link = topo_id;
      fc_ilog( logger, "collecting samples, topo_id = ${id}",("id", topo_id));
      {
         std::lock_guard<std::mutex> g( sample_mtx );
         ls.up.emplace_back(topology_sample(queue_depth, buffer_queue.write_queue_size()));
         ls.up.emplace_back(topology_sample(queue_max_depth, queue_hwm));
         tstamp ql = last_time_recv.xmt - last_time_recv.org;
         uint32_t ql32 = ql <= 0xffffffff ? uint32_t(ql) : 0xffffffff;
         ls.down.emplace_back(topology_sample(queue_latency,ql32));
         ql = last_time_sent.xmt - last_time_sent.org;
         ql32 = ql <= 0xffffffff ? uint32_t(ql) : 0xffffffff;
         ql = last_time_recv.rec - last_time_sent.xmt;
         ql32 = ql <= 0xffffffff ? uint32_t(ql) : 0xffffffff;
         ls.down.emplace_back(topology_sample(net_latency,ql32));
         ls.up.emplace_back(topology_sample(bytes_sent,bytes_sent_sample));
         ls.up.emplace_back(topology_sample(messages_sent,messages_sent_sample));
         ls.up.emplace_back(topology_sample(fork_instances,fork_instances_sample));
         ls.up.emplace_back(topology_sample(fork_depth,fork_depth_sample));
         ls.up.emplace_back(topology_sample(fork_max_depth,fork_hwm));
         bytes_sent_sample = 0;
         messages_sent_sample = 0;
         queue_hwm = 0;
         fork_hwm = 0;
      }
      my_impl->topology_plug->update_samples (ls);
   }

#if 0
   void connection::send_time() {
      time_message xpkt;
      xpkt.org = last_time_recv.rec;
      xpkt.rec = last_time_recv.dst;
      xpkt.xmt = get_tstamp();
      last_time_sent.org = xpkt.xmt;
      enqueue_time(xpkt);
   }
#endif

   void connection::send_time() {
      time_message xpkt;
      xpkt.org = get_tstamp();
      xpkt.xmt = 0;
      xpkt.rec = 0;
      xpkt.dst = 0;
      {
         std::lock_guard<std::mutex> g( sample_mtx );
         time_message_queued++;
      }
      enqueue_time(xpkt);
   }

   void connection::send_time(const time_message& msg) {
      time_message xpkt;
      xpkt.org = msg.xmt;
      xpkt.rec = msg.dst;
      xpkt.xmt = get_tstamp();
      enqueue_time(xpkt);
   }

   void connection::queue_write(const std::shared_ptr<vector<char>>& buff,
                                bool trigger_send,
                                int priority,
                                std::function<void(boost::system::error_code, std::size_t)> callback,
                                bool to_sync_queue) {
      if( !buffer_queue.add_write_queue( buff, callback, to_sync_queue )) {
         fc_wlog( logger, "write_queue full ${s} bytes, giving up on connection ${p}",
                  ("s", buffer_queue.write_queue_size())("p", peer_name()) );
         close();
         return;
      }
      if( buffer_queue.write_queue_size() > queue_hwm) {
         queue_hwm = buffer_queue.write_queue_size();
      }
      if( buffer_queue.is_out_queue_empty() && trigger_send) {
         do_queue_write( priority );
      }
   }

   void connection::do_queue_write(int priority) {
      if( !buffer_queue.ready_to_send() )
         return;
      connection_ptr c(shared_from_this());
      int do_xmt_time = 0;
      if (my_impl->topology_plug != nullptr) {
         std::lock_guard<std::mutex> g( sample_mtx );
         do_xmt_time = time_message_queued;
         time_message_queued = 0;
      }
      std::vector<boost::asio::const_buffer> bufs;
      uint32_t bytes{0};
      uint32_t msgs{0};
      buffer_queue.fill_out_buffer( bufs, bytes, msgs, do_xmt_time);

      strand.dispatch( [c{std::move(c)}, bufs{std::move(bufs)}, priority]() {
         boost::asio::async_write( *c->socket, bufs,
            boost::asio::bind_executor( c->strand, [c, socket=c->socket, priority]( boost::system::error_code ec, std::size_t w ) {
            try {
               // May have closed connection and cleared buffer_queue
               if( !c->socket_is_open() ) return;

               c->buffer_queue.out_callback( ec, w );

               if( ec ) {
                  if( ec.value() != boost::asio::error::eof ) {
                     fc_elog( logger, "Error sending to peer ${p}: ${i}", ("p", c->peer_name())( "i", ec.message() ) );
                  } else {
                     fc_wlog( logger, "connection closure detected on write to ${p}", ("p", c->peer_name()) );
                  }
                  c->close();
                  return;
               }
               c->buffer_queue.clear_out_queue();
               c->enqueue_sync_block();
               c->do_queue_write( priority );
            } catch( const std::exception& ex ) {
               fc_elog( logger, "Exception in do_queue_write to ${p} ${s}", ("p", c->peer_name())( "s", ex.what() ) );
            } catch( const fc::exception& ex ) {
               fc_elog( logger, "Exception in do_queue_write to ${p} ${s}", ("p", c->peer_name())( "s", ex.to_string() ) );
            } catch( ... ) {
               fc_elog( logger, "Exception in do_queue_write to ${p}", ("p", c->peer_name()) );
            }
         }));
      });
      if( my_impl->topology_plug != nullptr){
         std::lock_guard<std::mutex> g( sample_mtx );
         bytes_sent_sample += bytes;
         messages_sent_sample += msgs;
      }
   }

   void connection::cancel_sync(go_away_reason reason) {
      fc_dlog( logger, "cancel sync reason = ${m}, write queue size ${o} bytes peer ${p}",
               ("m", reason_str( reason ))( "o", buffer_queue.write_queue_size() )( "p", peer_address() ) );
      cancel_wait();
      flush_queues();
      switch (reason) {
      case validation :
      case fatal_other : {
         no_retry = reason;
         enqueue( go_away_message( reason ));
         break;
      }
      default:
         fc_dlog(logger, "sending empty request but not calling sync wait on ${p}", ("p",peer_address()));
         enqueue( ( sync_request_message ) {0,0} );
      }
   }

   bool connection::enqueue_sync_block() {
      if( !peer_requested ) {
         return false;
      } else {
         fc_dlog( logger, "enqueue sync block ${num}", ("num", peer_requested->last + 1) );
      }
      uint32_t num = ++peer_requested->last;
      bool trigger_send = true;
      if(num == peer_requested->end_block) {
         peer_requested.reset();
      }
      connection_wptr weak = shared_from_this();
      app().post( priority::medium, [num, trigger_send, weak{std::move(weak)}]() {
         connection_ptr c = weak.lock();
         if( !c ) return;
         controller& cc = my_impl->chain_plug->chain();
         signed_block_ptr sb = cc.fetch_block_by_number( num );
         if( sb ) {
            c->strand.post( [c, sb{std::move(sb)}, trigger_send]() {
               c->enqueue_block( sb, trigger_send, true );
            });
         }
      });

      return true;
   }

   void connection::enqueue_time( const time_message& m, bool trigger_send) {
      verify_strand_in_this_thread( strand, __func__, __LINE__ );
      go_away_reason close_after_send = no_reason;
      const uint32_t payload_size = time_placeholder_len + sizeof(m);

      const char* const header = reinterpret_cast<const char* const>(&payload_size); // avoid variable size encoding of uint32_t
      constexpr size_t header_size = sizeof(payload_size);
      static_assert( header_size == message_header_size, "invalid message_header_size" );
      const size_t buffer_size = header_size + payload_size;

      auto send_buffer = std::make_shared<vector<char>>(buffer_size);
      fc::datastream<char*> ds( send_buffer->data(), buffer_size);
      ds.write( header, header_size );
      ds.write( time_placeholder,time_placeholder_len);
      ds.write( reinterpret_cast<const char *>(&m), sizeof(m));
      enqueue_buffer( send_buffer, trigger_send, priority::low, close_after_send );
   }

   void connection::enqueue( const net_message& m, bool trigger_send ) {
      verify_strand_in_this_thread( strand, __func__, __LINE__ );
      go_away_reason close_after_send = no_reason;
      if (m.contains<go_away_message>()) {
         close_after_send = m.get<go_away_message>().reason;
      }
      const uint32_t payload_size = fc::raw::pack_size( m );

      const char* const header = reinterpret_cast<const char* const>(&payload_size); // avoid variable size encoding of uint32_t
      constexpr size_t header_size = sizeof(payload_size);
      static_assert( header_size == message_header_size, "invalid message_header_size" );
      const size_t buffer_size = header_size + payload_size;

      auto send_buffer = std::make_shared<vector<char>>(buffer_size);
      fc::datastream<char*> ds( send_buffer->data(), buffer_size);
      ds.write( header, header_size );
      fc::raw::pack( ds, m );
      enqueue_buffer( send_buffer, trigger_send, priority::low, close_after_send );
   }

   template< typename T>
   static std::shared_ptr<std::vector<char>> create_send_buffer( uint32_t which, const T& v ) {
      // match net_message static_variant pack
      const uint32_t which_size = fc::raw::pack_size( unsigned_int( which ) );
      const uint32_t payload_size = which_size + fc::raw::pack_size( v );

      const char* const header = reinterpret_cast<const char* const>(&payload_size); // avoid variable size encoding of uint32_t
      constexpr size_t header_size = sizeof( payload_size );
      static_assert( header_size == message_header_size, "invalid message_header_size" );
      const size_t buffer_size = header_size + payload_size;

      auto send_buffer = std::make_shared<vector<char>>( buffer_size );
      fc::datastream<char*> ds( send_buffer->data(), buffer_size );
      ds.write( header, header_size );
      fc::raw::pack( ds, unsigned_int( which ) );
      fc::raw::pack( ds, v );

      return send_buffer;
   }

   static std::shared_ptr<std::vector<char>> create_send_buffer( const signed_block_ptr& sb ) {
      // this implementation is to avoid copy of signed_block to net_message
      // matches which of net_message for signed_block
      fc_dlog( logger, "sending block ${bn}", ("bn", sb->block_num()) );
      return create_send_buffer( signed_block_which, *sb );
   }

   static std::shared_ptr<std::vector<char>> create_send_buffer( const packed_transaction& trx ) {
      // this implementation is to avoid copy of packed_transaction to net_message
      // matches which of net_message for packed_transaction
      return create_send_buffer( packed_transaction_which, trx );
   }

   static std::shared_ptr<std::vector<char>> create_send_buffer( const topology_message& tm ) {
      return create_send_buffer( topology_message_which, tm );
   }

   void connection::enqueue_block( const signed_block_ptr& sb, bool trigger_send, bool to_sync_queue) {
      fc_dlog( logger, "enqueue block ${num}", ("num", sb->block_num()) );
      verify_strand_in_this_thread( strand, __func__, __LINE__ );
      enqueue_buffer( create_send_buffer( sb ), trigger_send, priority::low, no_reason, to_sync_queue);
   }

   void connection::enqueue_buffer( const std::shared_ptr<std::vector<char>>& send_buffer,
                                    bool trigger_send, int priority, go_away_reason close_after_send,
                                    bool to_sync_queue)
   {
      connection_ptr self = shared_from_this();
      queue_write(send_buffer,trigger_send, priority,
                  [conn{std::move(self)}, close_after_send](boost::system::error_code ec, std::size_t ) {
                        if (close_after_send != no_reason) {
                           fc_elog( logger, "sent a go away message: ${r}, closing connection to ${p}",
                                    ("r", reason_str(close_after_send))("p", conn->peer_name()) );
                           conn->close();
                           return;
                        }
                  },
                  to_sync_queue);
   }

   // thread safe
   void connection::cancel_wait() {
      std::lock_guard<std::mutex> g( response_expected_timer_mtx );
      response_expected_timer.cancel();
   }

   // thread safe
   void connection::sync_wait() {
      connection_ptr c(shared_from_this());
      std::lock_guard<std::mutex> g( response_expected_timer_mtx );
      response_expected_timer.expires_from_now( my_impl->resp_expected_period );
      response_expected_timer.async_wait(
            boost::asio::bind_executor( c->strand, [c]( boost::system::error_code ec ) {
               c->sync_timeout( ec );
            } ) );
   }

   // thread safe
   void connection::fetch_wait() {
      connection_ptr c( shared_from_this() );
      std::lock_guard<std::mutex> g( response_expected_timer_mtx );
      response_expected_timer.expires_from_now( my_impl->resp_expected_period );
      response_expected_timer.async_wait(
            boost::asio::bind_executor( c->strand, [c]( boost::system::error_code ec ) {
               c->fetch_timeout(ec);
            } ) );
   }

   // called from connection strand
   void connection::sync_timeout( boost::system::error_code ec ) {
      if( !ec ) {
         my_impl->sync_master->sync_reassign_fetch( shared_from_this(), benign_other );
      } else if( ec == boost::asio::error::operation_aborted ) {
      } else {
         fc_elog( logger, "setting timer for sync request got error ${ec}", ("ec", ec.message()) );
      }
   }

   // locks conn_mtx, do not call while holding conn_mtx
   const string connection::peer_name() {
      std::unique_lock<std::mutex> g_conn( conn_mtx );
      if( !last_handshake_recv.p2p_address.empty() ) {
         return last_handshake_recv.p2p_address;
      }
      g_conn.unlock();
      if( !peer_address().empty() ) {
         return peer_address();
      }
      if( remote_endpoint_port != unknown ) {
         return remote_endpoint_ip + ":" + remote_endpoint_port;
      }
      return "connecting client";
   }

   void connection::fetch_timeout( boost::system::error_code ec ) {
      if( !ec ) {
         my_impl->dispatcher->retry_fetch( shared_from_this() );
      } else if( ec == boost::asio::error::operation_aborted ) {
         if( !connected() ) {
            fc_dlog( logger, "fetch timeout was cancelled due to dead connection" );
         }
      } else {
         fc_elog( logger, "setting timer for fetch request got error ${ec}", ("ec", ec.message() ) );
      }
   }

   void connection::request_sync_blocks(uint32_t start, uint32_t end) {
      sync_request_message srm = {start,end};
      enqueue( net_message(srm) );
      sync_wait();
   }

   //-----------------------------------------------------------

    sync_manager::sync_manager( uint32_t req_span )
      :sync_known_lib_num( 0 )
      ,sync_last_requested_num( 0 )
      ,sync_next_expected_num( 1 )
      ,sync_req_span( req_span )
      ,sync_source()
      ,sync_state(in_sync)
   {
      chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT( chain_plug, chain::missing_chain_plugin_exception, ""  );
   }

   constexpr auto sync_manager::stage_str(stages s) {
    switch (s) {
    case in_sync : return "in sync";
    case lib_catchup: return "lib catchup";
    case head_catchup : return "head catchup";
    default : return "unkown";
    }
  }

   void sync_manager::set_state(stages newstate) {
      if( sync_state == newstate ) {
         return;
      }
      fc_dlog( logger, "old state ${os} becoming ${ns}", ("os", stage_str( sync_state ))( "ns", stage_str( newstate ) ) );
      sync_state = newstate;
   }

   bool sync_manager::block_while_syncing_with_other_peer( const connection_ptr& c ) const {
      if( syncing_with_peer() ) {
         std::lock_guard<std::mutex> g( sync_mtx );
         return c != sync_source;
      }
      return false;
   }

   void sync_manager::sync_reset_lib_num(const connection_ptr& c) {
      std::unique_lock<std::mutex> g( sync_mtx );
      if( sync_state == in_sync ) {
         sync_source.reset();
      }
      if( !c ) return;
      if( c->current() ) {
         std::lock_guard<std::mutex> g_conn( c->conn_mtx );
         if( c->last_handshake_recv.last_irreversible_block_num > sync_known_lib_num ) {
            sync_known_lib_num = c->last_handshake_recv.last_irreversible_block_num;
         }
      } else if( c == sync_source ) {
         sync_last_requested_num = 0;
         request_next_chunk( std::move(g) );
      }
   }

   // call with g_sync locked
   void sync_manager::request_next_chunk( std::unique_lock<std::mutex> g_sync, const connection_ptr& conn ) {
      uint32_t fork_head_block_num = 0;
      uint32_t lib_block_num = 0;
      std::tie( lib_block_num, std::ignore, fork_head_block_num,
                std::ignore, std::ignore, std::ignore ) = my_impl->get_chain_info();

      if( fork_head_block_num < sync_last_requested_num && sync_source && sync_source->current() ) {
         fc_ilog( logger, "ignoring request, head is ${h} last req = ${r} source is ${p}",
                  ("h", fork_head_block_num)( "r", sync_last_requested_num )( "p", sync_source->peer_name() ) );
         return;
      }

      /* ----------
       * next chunk provider selection criteria
       * a provider is supplied and able to be used, use it.
       * otherwise select the next available from the list, round-robin style.
       */

      if (conn && conn->current() ) {
         sync_source = conn;
      } else {
         std::shared_lock<std::shared_mutex> g( my_impl->connections_mtx );
         if( my_impl->connections.size() == 0 ) {
            sync_source.reset();
         } else if( my_impl->connections.size() == 1 ) {
            if (!sync_source) {
               sync_source = *my_impl->connections.begin();
            }
         } else {
            // init to a linear array search
            auto cptr = my_impl->connections.begin();
            auto cend = my_impl->connections.end();
            // do we remember the previous source?
            if (sync_source) {
               //try to find it in the list
               cptr = my_impl->connections.find( sync_source );
               cend = cptr;
               if( cptr == my_impl->connections.end() ) {
                  //not there - must have been closed! cend is now connections.end, so just flatten the ring.
                  sync_source.reset();
                  cptr = my_impl->connections.begin();
               } else {
                  //was found - advance the start to the next. cend is the old source.
                  if( ++cptr == my_impl->connections.end() && cend != my_impl->connections.end() ) {
                     cptr = my_impl->connections.begin();
                  }
               }
            }

            //scan the list of peers looking for another able to provide sync blocks.
            if( cptr != my_impl->connections.end() ) {
               auto cstart_it = cptr;
               do {
                  //select the first one which is current and break out.
                  if( !(*cptr)->is_transactions_only_connection() && (*cptr)->current() ) {
                     sync_source = *cptr;
                     break;
                  }
                  if( ++cptr == my_impl->connections.end() )
                     cptr = my_impl->connections.begin();
               } while( cptr != cstart_it );
            }
            // no need to check the result, either source advanced or the whole list was checked and the old source is reused.
         }
      }

      // verify there is an available source
      if( !sync_source || !sync_source->current() || sync_source->is_transactions_only_connection() ) {
         fc_elog( logger, "Unable to continue syncing at this time");
         sync_known_lib_num = lib_block_num;
         sync_last_requested_num = 0;
         set_state( in_sync ); // probably not, but we can't do anything else
         return;
      }

      if( sync_last_requested_num != sync_known_lib_num ) {
         uint32_t start = sync_next_expected_num;
         uint32_t end = start + sync_req_span - 1;
         if( end > sync_known_lib_num )
            end = sync_known_lib_num;
         if( end > 0 && end >= start ) {
            sync_last_requested_num = end;
            connection_ptr c = sync_source;
            g_sync.unlock();
            c->strand.post( [c, start, end]() {
               fc_ilog( logger, "requesting range ${s} to ${e}, from ${n}", ("n", c->peer_name())( "s", start )( "e", end ) );
               c->request_sync_blocks( start, end );
            } );
         }
      }
   }

   // static, thread safe
   void sync_manager::send_handshakes() {
      for_each_connection( []( auto& ci ) {
         if( ci->current() ) {
            ci->send_handshake();
         }
         return true;
      } );
   }

   bool sync_manager::is_sync_required( uint32_t fork_head_block_num ) {
      fc_dlog( logger, "last req = ${req}, last recv = ${recv} known = ${known} our head = ${head}",
               ("req", sync_last_requested_num)( "recv", sync_next_expected_num )( "known", sync_known_lib_num )
               ("head", fork_head_block_num ) );

      return( sync_last_requested_num < sync_known_lib_num ||
              fork_head_block_num < sync_last_requested_num );
   }

   void sync_manager::start_sync(const connection_ptr& c, uint32_t target) {
      std::unique_lock<std::mutex> g_sync( sync_mtx );
      if( target > sync_known_lib_num) {
         sync_known_lib_num = target;
      }

      uint32_t lib_num = 0;
      uint32_t fork_head_block_num = 0;
      std::tie( lib_num, std::ignore, fork_head_block_num,
                std::ignore, std::ignore, std::ignore ) = my_impl->get_chain_info();

      if( !is_sync_required( fork_head_block_num ) ) {
         fc_dlog( logger, "We are already caught up, my irr = ${b}, head = ${h}, target = ${t}",
                  ("b", lib_num)( "h", fork_head_block_num )( "t", target ) );
         return;
      }

      if( sync_state == in_sync ) {
         set_state( lib_catchup );
         sync_next_expected_num = std::max( lib_num + 1, sync_next_expected_num );
      }

      fc_ilog( logger, "Catching up with chain, our last req is ${cc}, theirs is ${t} peer ${p}",
               ("cc", sync_last_requested_num)( "t", target )( "p", c->peer_name() ) );

      request_next_chunk( std::move( g_sync ), c );
   }

   // called from connection strand
   void sync_manager::sync_reassign_fetch(const connection_ptr& c, go_away_reason reason) {
      std::unique_lock<std::mutex> g( sync_mtx );
      fc_ilog( logger, "reassign_fetch, our last req is ${cc}, next expected is ${ne} peer ${p}",
               ("cc", sync_last_requested_num)( "ne", sync_next_expected_num )( "p", c->peer_name() ) );

      if( c == sync_source ) {
         c->cancel_sync(reason);
         sync_last_requested_num = 0;
         request_next_chunk( std::move(g) );
      }
   }

   void sync_manager::recv_handshake( const connection_ptr& c, const handshake_message& msg ) {

      if( c->is_transactions_only_connection() ) return;

      uint32_t lib_num = 0;
      uint32_t peer_lib = msg.last_irreversible_block_num;
      uint32_t head = 0;
      block_id_type head_id;
      std::tie( lib_num, std::ignore, head,
                std::ignore, std::ignore, head_id ) = my_impl->get_chain_info();

      sync_reset_lib_num(c);

      //--------------------------------
      // sync need checks; (lib == last irreversible block)
      //
      // 0. my head block id == peer head id means we are all caught up block wise
      // 1. my head block num < peer lib - start sync locally
      // 2. my lib > peer head num - send an last_irr_catch_up notice if not the first generation
      //
      // 3  my head block num < peer head block num - update sync state and send a catchup request
      // 4  my head block num >= peer block num send a notice catchup if this is not the first generation
      //
      //-----------------------------

      if (head_id == msg.head_id) {
         fc_dlog(logger, "sync check state 0");
         c->syncing = false;
         notice_message note;
         note.known_blocks.mode = none;
         note.known_trx.mode = catch_up;
         note.known_trx.pending = 0;
         c->enqueue( note );
         return;
      }
      if (head < peer_lib) {
         fc_dlog(logger, "sync check state 1");
         c->syncing = false;
         // wait for receipt of a notice message before initiating sync
         if (c->protocol_version < proto_explicit_sync) {
            start_sync( c, peer_lib );
         }
         return;
      }
      if (lib_num > msg.head_num ) {
         fc_dlog(logger, "sync check state 2");
         if (msg.generation > 1 || c->protocol_version > proto_base) {
            notice_message note;
            note.known_trx.pending = lib_num;
            note.known_trx.mode = last_irr_catch_up;
            note.known_blocks.mode = last_irr_catch_up;
            note.known_blocks.pending = head;
            c->enqueue( note );
         }
         c->syncing = true;
         return;
      }

      if (head < msg.head_num ) {
         fc_dlog(logger, "sync check state 3");
         c->syncing = false;
         if (!verify_catchup(c, msg.head_num, msg.head_id)) {
            request_message req;
            req.req_blocks.mode = catch_up;
            req.req_trx.mode = none;
            c->enqueue( req );
         }
         return;
      } else {
         fc_dlog(logger, "sync check state 4");
         if (msg.generation > 1 ||  c->protocol_version > proto_base) {
            notice_message note;
            note.known_trx.mode = none;
            note.known_blocks.mode = catch_up;
            note.known_blocks.pending = head;
            note.known_blocks.ids.push_back(head_id);
            c->enqueue( note );
         }
         c->syncing = true;
         request_message req;
         req.req_blocks.mode = catch_up;
         req.req_trx.mode = none;
         c->enqueue( req );
         return;
      }
      c->syncing = false;
      fc_elog( logger, "sync check failed to resolve status" );
   }

   bool sync_manager::verify_catchup(const connection_ptr& c, uint32_t num, const block_id_type& id) {
      request_message req;
      req.req_blocks.mode = catch_up;
      for_each_block_connection( [num, &id, &req]( const auto& cc ) {
         std::lock_guard<std::mutex> g_conn( cc->conn_mtx );
         if( cc->fork_head_num > num || cc->fork_head == id ) {
            req.req_blocks.mode = none;
            return false;
         }
         return true;
      } );
      if( req.req_blocks.mode == catch_up ) {
         {
            std::lock_guard<std::mutex> g_conn( c->conn_mtx );
            c->fork_head = id;
            c->fork_head_num = num;
         }
         std::lock_guard<std::mutex> g( sync_mtx );
         fc_ilog( logger, "got a catch_up notice while in ${s}, fork head num = ${fhn} "
                          "target LIB = ${lib} next_expected = ${ne}",
                  ("s", stage_str( sync_state ))( "fhn", num )( "lib", sync_known_lib_num )
                  ("ne", sync_next_expected_num ) );
         if( sync_state == lib_catchup )
            return false;
         set_state( head_catchup );
      } else {
         std::lock_guard<std::mutex> g_conn( c->conn_mtx );
         c->fork_head = block_id_type();
         c->fork_head_num = 0;
      }
      req.req_trx.mode = none;
      c->enqueue( req );
      return true;
   }

   void sync_manager::sync_recv_notice( const connection_ptr& c, const notice_message& msg) {
      fc_ilog( logger, "sync_manager got ${m} block notice", ("m", modes_str( msg.known_blocks.mode )) );
      EOS_ASSERT( msg.known_blocks.mode == catch_up || msg.known_blocks.mode == last_irr_catch_up, plugin_exception,
                  "sync_recv_notice only called on catch_up" );
      if (msg.known_blocks.mode == catch_up) {
         if (msg.known_blocks.ids.size() == 0) {
            fc_elog( logger,"got a catch up with ids size = 0" );
         } else {
            verify_catchup(c, msg.known_blocks.pending, msg.known_blocks.ids.back());
         }
      } else if (msg.known_blocks.mode == last_irr_catch_up) {
         {
            std::lock_guard<std::mutex> g_conn( c->conn_mtx );
            c->last_handshake_recv.last_irreversible_block_num = msg.known_trx.pending;
         }
         sync_reset_lib_num(c);
         start_sync(c, msg.known_trx.pending);
      }
   }

   // called from connection strand
   void sync_manager::rejected_block( const connection_ptr& c, uint32_t blk_num ) {
      std::unique_lock<std::mutex> g( sync_mtx );
      if( sync_state != in_sync ) {
         if( ++c->consecutive_rejected_blocks > def_max_consecutive_rejected_blocks ) {
            fc_wlog( logger, "block ${bn} not accepted from ${p}, closing connection", ("bn", blk_num)( "p", c->peer_name() ) );
            sync_last_requested_num = 0;
            sync_source.reset();
            g.unlock();
            c->close();
            send_handshakes();
         }
      }
   }

   // called from connection strand
   void sync_manager::sync_recv_block(const connection_ptr& c, const block_id_type& blk_id, uint32_t blk_num) {
      fc_dlog( logger, "got block ${bn} from ${p}", ("bn", blk_num)( "p", c->peer_name() ) );
      if( app().is_quiting() ) {
         c->close( false );
         return;
      }
      c->consecutive_rejected_blocks = 0;
      std::unique_lock<std::mutex> g_sync( sync_mtx );
      stages state = sync_state;
      fc_dlog( logger, "state ${s}", ("s", stage_str( state )) );
      if( state == lib_catchup ) {
         if (blk_num != sync_next_expected_num) {
            if( ++c->consecutive_rejected_blocks > def_max_consecutive_rejected_blocks ) {
               auto sync_next_expected = sync_next_expected_num;
               g_sync.unlock();
               fc_wlog( logger, "expected block ${ne} but got ${bn}, closing connection: ${p}",
                        ("ne", sync_next_expected)( "bn", blk_num )( "p", c->peer_name() ) );
               c->close();
            }
            return;
         }
         sync_next_expected_num = blk_num + 1;
      }
      if( state == head_catchup ) {
         fc_dlog( logger, "sync_manager in head_catchup state" );
         set_state( in_sync );
         sync_source.reset();
         g_sync.unlock();

         block_id_type null_id;
         bool set_state_to_head_catchup = false;
         for_each_block_connection( [&null_id, blk_num, &blk_id, &c, &set_state_to_head_catchup]( const auto& cp ) {
            std::unique_lock<std::mutex> g_cp_conn( cp->conn_mtx );
            uint32_t fork_head_num = cp->fork_head_num;
            block_id_type fork_head_id = cp->fork_head;
            g_cp_conn.unlock();
            if( fork_head_id == null_id ) {
               // continue
            } else if( fork_head_num < blk_num || fork_head_id == blk_id ) {
               std::lock_guard<std::mutex> g_conn( c->conn_mtx );
               c->fork_head = null_id;
               c->fork_head_num = 0;
            } else {
               set_state_to_head_catchup = true;
            }
            return true;
         } );

         if( set_state_to_head_catchup ) {
            g_sync.lock();
            set_state( head_catchup );
            g_sync.unlock();
         } else {
            send_handshakes();
         }
      } else if( state == lib_catchup ) {
         if( blk_num == sync_known_lib_num ) {
            fc_dlog( logger, "All caught up with last known last irreversible block resending handshake" );
            set_state( in_sync );
            g_sync.unlock();
            send_handshakes();
         } else if( blk_num == sync_last_requested_num ) {
            request_next_chunk( std::move( g_sync) );
         } else {
            g_sync.unlock();
            fc_dlog( logger, "calling sync_wait on connection ${p}", ("p", c->peer_name()) );
            c->sync_wait();
         }
      }
   }

   //------------------------------------------------------------------------

   // thread safe
   bool dispatch_manager::add_peer_block( const block_id_type& blkid, uint32_t connection_id) {
      std::lock_guard<std::mutex> g( blk_state_mtx );
      auto bptr = blk_state.get<by_id>().find( std::make_tuple( connection_id, std::ref( blkid )));
      bool added = (bptr == blk_state.end());
      if( added ) {
         blk_state.insert( {blkid, block_header::num_from_id( blkid ), connection_id, true} );
      } else if( !bptr->have_block ) {
         blk_state.modify( bptr, []( auto& pb ) {
            pb.have_block = true;
         });
      }
      return added;
   }

   bool dispatch_manager::add_peer_block_id( const block_id_type& blkid, uint32_t connection_id) {
      std::lock_guard<std::mutex> g( blk_state_mtx );
      auto bptr = blk_state.get<by_id>().find( std::make_tuple( connection_id, std::ref( blkid )));
      bool added = (bptr == blk_state.end());
      if( added ) {
         blk_state.insert( {blkid, block_header::num_from_id( blkid ), connection_id, false} );
      }
      return added;
   }

   bool dispatch_manager::peer_has_block( const block_id_type& blkid, uint32_t connection_id ) const {
      std::lock_guard<std::mutex> g(blk_state_mtx);
      const auto blk_itr = blk_state.get<by_id>().find( std::make_tuple( connection_id, std::ref( blkid )));
      return blk_itr != blk_state.end();
   }

   bool dispatch_manager::have_block( const block_id_type& blkid ) const {
      std::lock_guard<std::mutex> g(blk_state_mtx);
      // by_block_id sorts have_block by greater so have_block == true will be the first one found
      const auto& index = blk_state.get<by_block_id>();
      auto blk_itr = index.find( blkid );
      if( blk_itr != index.end() ) {
         return blk_itr->have_block;
      }
      return false;
   }

   size_t dispatch_manager::num_entries( uint32_t connection_id ) const {
      std::lock_guard<std::mutex> g(blk_state_mtx);
      return blk_state.get<by_id>().count( connection_id );
   }

   bool dispatch_manager::add_peer_txn( const node_transaction_state& nts ) {
      std::lock_guard<std::mutex> g( local_txns_mtx );
      auto tptr = local_txns.get<by_id>().find( std::make_tuple( std::ref( nts.id ), nts.connection_id ) );
      bool added = (tptr == local_txns.end());
      if( added ) {
         local_txns.insert( nts );
      }
      return added;
   }

   // thread safe
   void dispatch_manager::update_txns_block_num( const signed_block_ptr& sb ) {
      update_block_num ubn( sb->block_num() );
      std::lock_guard<std::mutex> g( local_txns_mtx );
      for( const auto& recpt : sb->transactions ) {
         const transaction_id_type& id = (recpt.trx.which() == 0) ? recpt.trx.get<transaction_id_type>()
                                                                  : recpt.trx.get<packed_transaction>().id();
         auto range = local_txns.get<by_id>().equal_range( id );
         for( auto itr = range.first; itr != range.second; ++itr ) {
            local_txns.modify( itr, ubn );
         }
      }
   }

   // thread safe
   void dispatch_manager::update_txns_block_num( const transaction_id_type& id, uint32_t blk_num ) {
      update_block_num ubn( blk_num );
      std::lock_guard<std::mutex> g( local_txns_mtx );
      auto range = local_txns.get<by_id>().equal_range( id );
      for( auto itr = range.first; itr != range.second; ++itr ) {
         local_txns.modify( itr, ubn );
      }
   }

   bool dispatch_manager::peer_has_txn( const transaction_id_type& tid, uint32_t connection_id ) const {
      std::lock_guard<std::mutex> g( local_txns_mtx );
      const auto tptr = local_txns.get<by_id>().find( std::make_tuple( std::ref( tid ), connection_id ) );
      return tptr != local_txns.end();
   }

   bool dispatch_manager::have_txn( const transaction_id_type& tid ) const {
      std::lock_guard<std::mutex> g( local_txns_mtx );
      const auto tptr = local_txns.get<by_id>().find( tid );
      return tptr != local_txns.end();
   }

   void dispatch_manager::expire_txns( uint32_t lib_num ) {
      size_t start_size = 0, end_size = 0;

      std::unique_lock<std::mutex> g( local_txns_mtx );
      start_size = local_txns.size();
      auto& old = local_txns.get<by_expiry>();
      auto ex_lo = old.lower_bound( fc::time_point_sec( 0 ) );
      auto ex_up = old.upper_bound( time_point::now() );
      old.erase( ex_lo, ex_up );
      g.unlock(); // allow other threads opportunity to use local_txns

      g.lock();
      auto& stale = local_txns.get<by_block_num>();
      stale.erase( stale.lower_bound( 1 ), stale.upper_bound( lib_num ) );
      end_size = local_txns.size();
      g.unlock();

      fc_dlog( logger, "expire_local_txns size ${s} removed ${r}", ("s", start_size)( "r", start_size - end_size ) );
   }

   void dispatch_manager::expire_blocks( uint32_t lib_num ) {
      std::lock_guard<std::mutex> g(blk_state_mtx);
      auto& stale_blk = blk_state.get<by_block_num>();
      stale_blk.erase( stale_blk.lower_bound(1), stale_blk.upper_bound(lib_num) );
   }

   // thread safe
   void dispatch_manager::bcast_block(const block_state_ptr& bs) {
      fc_dlog( logger, "bcast block ${b}", ("b", bs->block_num) );

      if( my_impl->sync_master->syncing_with_peer() ) return;
      bool have_connection = false;
      for_each_block_connection( [&have_connection]( auto& cp ) {
         peer_dlog( cp, "socket_is_open ${s}, connecting ${c}, syncing ${ss}",
                    ("s", cp->socket_is_open())("c", cp->connecting.load())("ss", cp->syncing.load()) );

         if( !cp->current() ) {
            return true;
         }
         have_connection = true;
         return false;
      } );

      if( !have_connection ) return;
      std::shared_ptr<std::vector<char>> send_buffer = create_send_buffer( bs->block );

      for_each_block_connection( [this, bs, send_buffer]( auto& cp ) {
         if( !cp->current() ) {
            return true;
         }
         cp->strand.post( [this, cp, bs, send_buffer]() {
            uint32_t bnum = bs->block_num;
            std::unique_lock<std::mutex> g_conn( cp->conn_mtx );
            bool has_block = cp->last_handshake_recv.last_irreversible_block_num >= bnum;
            g_conn.unlock();
            if( !has_block ) {
               if( !add_peer_block( bs->id, cp->connection_id ) ) {
                  return;
               }
               fc_dlog( logger, "bcast block ${b} to ${p}", ("b", bnum)( "p", cp->peer_name() ) );
               cp->enqueue_buffer( send_buffer, true, priority::medium, no_reason );
            }
         });
         return true;
      } );
   }

   void dispatch_manager::bcast_notice( const block_id_type& id ) {
      if( my_impl->sync_master->syncing_with_peer() ) return;

      fc_dlog( logger, "bcast notice ${b}", ("b", block_header::num_from_id( id )) );
      notice_message note;
      note.known_blocks.mode = normal;
      note.known_blocks.pending = 1; // 1 indicates this is a block id notice
      note.known_blocks.ids.emplace_back( id );

      for_each_block_connection( [this, note]( auto& cp ) {
         if( !cp->current() ) {
            return true;
         }
         cp->strand.post( [this, cp, note]() {
            // check protocol_version here since only accessed from strand
            if( cp->protocol_version < block_id_notify ) return;
            const block_id_type& id = note.known_blocks.ids.back();
            if( peer_has_block( id, cp->connection_id ) ) {
               return;
            }
            fc_dlog( logger, "bcast block id ${b} to ${p}", ("b", block_header::num_from_id( id ))("p", cp->peer_name()) );
            cp->enqueue( note );
         } );
         return true;
      } );
   }

   // called from connection strand
   void dispatch_manager::recv_block(const connection_ptr& c, const block_id_type& id, uint32_t bnum) {
      std::unique_lock<std::mutex> g( c->conn_mtx );
      if (c &&
          c->last_req &&
          c->last_req->req_blocks.mode != none &&
          !c->last_req->req_blocks.ids.empty() &&
          c->last_req->req_blocks.ids.back() == id) {
         fc_dlog( logger, "reseting last_req" );
         c->last_req.reset();
      }
      g.unlock();

      fc_dlog(logger, "canceling wait on ${p}", ("p",c->peer_name()));
      c->cancel_wait();
   }

   void dispatch_manager::rejected_block(const block_id_type& id) {
      fc_dlog( logger, "rejected block ${id}", ("id", id) );
   }

   void dispatch_manager::bcast_topology_message (const topology_message& tm) {
      std::shared_ptr<std::vector<char>> send_buffer;
      for_each_connection
         ( [tm, &send_buffer]( auto &cp )
           {
              if( !cp->current() ) {
                 return true;
              }
              if( !my_impl->topology_plug->forward_topology_message (tm, cp->topo_id) ) {
                 fc_dlog( logger, "skipping topology to ${n}", ("n", cp->peer_name()) );
                 return true;
              }
              if( !send_buffer ) {
                 send_buffer = create_send_buffer( tm );
              }

              cp->strand.post( [cp, send_buffer]() {
                  fc_dlog( logger, "sending topology to ${n}", ("n", cp->peer_name()) );
                  cp->enqueue_buffer( send_buffer, true, priority::low, no_reason );
              } );
              return true;

           });
   }

   void dispatch_manager::bcast_transaction(const transaction_metadata_ptr& ptrx) {
      const auto& id = ptrx->id();
      const packed_transaction& trx = *ptrx->packed_trx();
      time_point_sec trx_expiration = trx.expiration();
      node_transaction_state nts = {id, trx_expiration, 0, 0};

      std::shared_ptr<std::vector<char>> send_buffer;
      for_each_connection( [this, &trx, &nts, &send_buffer]( auto& cp ) {
         if( cp->is_blocks_only_connection() || !cp->current() ) {
            return true;
         }
         nts.connection_id = cp->connection_id;
         if( !add_peer_txn(nts) ) {
            return true;
         }
         if( !send_buffer ) {
            send_buffer = create_send_buffer( trx );
         }

         cp->strand.post( [cp, send_buffer]() {
            fc_dlog( logger, "sending trx to ${n}", ("n", cp->peer_name()) );
            cp->enqueue_buffer( send_buffer, true, priority::low, no_reason );
         } );
         return true;
      } );
   }

   void dispatch_manager::recv_transaction(const connection_ptr& c, const transaction_metadata_ptr& txn) {
      node_transaction_state nts = {txn->id(), txn->packed_trx()->expiration(), 0, c->connection_id};
      add_peer_txn( nts );
   }

   void dispatch_manager::rejected_transaction(const transaction_id_type& id, uint32_t head_blk_num) {
      fc_dlog( logger, "not sending rejected transaction ${tid}", ("tid", id) );
      // keep rejected transaction around for awhile so we don't broadcast it
      // update its block number so it will be purged when current block number is lib
      update_txns_block_num( id, head_blk_num );
   }

   // called from connection strand
   void dispatch_manager::recv_notice(const connection_ptr& c, const notice_message& msg, bool generated) {
      if (msg.known_trx.mode == normal) {
      } else if (msg.known_trx.mode != none) {
         fc_elog( logger, "passed a notice_message with something other than a normal on none known_trx" );
         return;
      }
      if (msg.known_blocks.mode == normal) {
         // known_blocks.ids is never > 1
         if( !msg.known_blocks.ids.empty() ) {
            if( num_entries( c->connection_id ) > def_max_peer_block_ids_per_connection ) {
               fc_elog( logger, "received too many notice_messages, disconnecting" );
               c->close( false );
            }
            const block_id_type& blkid = msg.known_blocks.ids.back();
            if( have_block( blkid )) {
               add_peer_block( blkid, c->connection_id );
               return;
            } else {
               add_peer_block_id( blkid, c->connection_id );
            }
            if( msg.known_blocks.pending == 1 ) { // block id notify
               return;
            }
         }
      } else if (msg.known_blocks.mode != none) {
         fc_elog( logger, "passed a notice_message with something other than a normal on none known_blocks" );
         return;
      }
   }

   void dispatch_manager::retry_fetch(const connection_ptr& c) {
      fc_dlog( logger, "retry fetch" );
      request_message last_req;
      block_id_type bid;
      {
         std::lock_guard<std::mutex> g_c_conn( c->conn_mtx );
         if( !c->last_req ) {
            return;
         }
         fc_wlog( logger, "failed to fetch from ${p}", ("p", c->peer_address()) );
         if( c->last_req->req_blocks.mode == normal && !c->last_req->req_blocks.ids.empty() ) {
            bid = c->last_req->req_blocks.ids.back();
         } else {
            fc_wlog( logger, "no retry, block mpde = ${b} trx mode = ${t}",
                     ("b", modes_str( c->last_req->req_blocks.mode ))( "t", modes_str( c->last_req->req_trx.mode ) ) );
            return;
         }
         last_req = *c->last_req;
      }
      for_each_block_connection( [this, &c, &last_req, &bid]( auto& conn ) {
         if( conn == c )
            return true;
         {
            std::lock_guard<std::mutex> guard( conn->conn_mtx );
            if( conn->last_req ) {
               return true;
            }
         }

         bool sendit = peer_has_block( bid, conn->connection_id );
         if( sendit ) {
            conn->strand.post( [conn, last_req{std::move(last_req)}]() {
               conn->enqueue( last_req );
               conn->fetch_wait();
               std::lock_guard<std::mutex> g_conn_conn( conn->conn_mtx );
               conn->last_req = last_req;
            } );
            return false;
         }
         return true;
      } );

      // at this point no other peer has it, re-request or do nothing?
      fc_wlog( logger, "no peer has last_req" );
      if( c->connected() ) {
         c->enqueue( last_req );
         c->fetch_wait();
      }
   }

   //------------------------------------------------------------------------

   // called from any thread
   bool connection::resolve_and_connect() {
      if( no_retry != go_away_reason::no_reason) {
         fc_dlog( logger, "Skipping connect due to go_away reason ${r}",("r", reason_str( no_retry )));
         return false;
      }

      string::size_type colon = peer_address().find(':');
      if (colon == std::string::npos || colon == 0) {
         fc_elog( logger, "Invalid peer address. must be \"host:port[:<blk>|<trx>]\": ${p}", ("p", peer_address()) );
         return false;
      }

      connection_ptr c = shared_from_this();

      if( consecutive_immediate_connection_close > def_max_consecutive_immediate_connection_close ) {
         auto connector_period_us = std::chrono::duration_cast<std::chrono::microseconds>( my_impl->connector_period );
         std::lock_guard<std::mutex> g( c->conn_mtx );
         if( last_close == fc::time_point() || last_close > fc::time_point::now() - fc::microseconds( connector_period_us.count() ) ) {
            return true; // true so doesn't remove from valid connections
         }
      }

      strand.post([c]() {
         string::size_type colon = c->peer_address().find(':');
         string::size_type colon2 = c->peer_address().find(':', colon + 1);
         string host = c->peer_address().substr( 0, colon );
         string port = c->peer_address().substr( colon + 1, colon2 == string::npos ? string::npos : colon2 - (colon + 1));
         idump((host)(port));
         c->set_connection_type( c->peer_address() );
         tcp::resolver::query query( tcp::v4(), host, port );
         // Note: need to add support for IPv6 too

         auto resolver = std::make_shared<tcp::resolver>( my_impl->thread_pool->get_executor() );
         connection_wptr weak_conn = c;
         resolver->async_resolve( query, boost::asio::bind_executor( c->strand,
            [resolver, weak_conn]( const boost::system::error_code& err, tcp::resolver::results_type endpoints ) {
               auto c = weak_conn.lock();
               if( !c ) return;
               if( !err ) {
                  c->connect( resolver, endpoints );
               } else {
                  fc_elog( logger, "Unable to resolve ${add}: ${error}", ("add", c->peer_name())( "error", err.message() ) );
                  ++c->consecutive_immediate_connection_close;
               }
         } ) );
      } );
      return true;
   }

   // called from connection strand
   void connection::connect( const std::shared_ptr<tcp::resolver>& resolver, tcp::resolver::results_type endpoints ) {
      if( no_retry != go_away_reason::no_reason) {
         return;
      }
      connecting = true;
      pending_message_buffer.reset();
      boost::asio::async_connect( *socket, endpoints,
         boost::asio::bind_executor( strand,
               [resolver, c = shared_from_this(), socket=socket]( const boost::system::error_code& err, const tcp::endpoint& endpoint ) {
            if( !err && socket->is_open() ) {
               if( c->start_session() ) {
                  c->send_handshake();
               }
            } else {
               fc_elog( logger, "connection failed to ${peer}: ${error}", ("peer", c->peer_name())( "error", err.message()));
               c->close( false );
            }
      } ) );
   }

   void net_plugin_impl::start_listen_loop() {
      connection_ptr new_connection = std::make_shared<connection>();
      new_connection->connecting = true;
      new_connection->strand.post( [this, new_connection = std::move( new_connection )](){
         acceptor->async_accept( *new_connection->socket,
            boost::asio::bind_executor( new_connection->strand, [new_connection, socket=new_connection->socket, this]( boost::system::error_code ec ) {
            if( !ec ) {
               uint32_t visitors = 0;
               uint32_t from_addr = 0;
               boost::system::error_code rec;
               const auto& paddr_add = socket->remote_endpoint( rec ).address();
               string paddr_str;
               if( rec ) {
                  fc_elog( logger, "Error getting remote endpoint: ${m}", ("m", rec.message()));
               } else {
                  paddr_str = paddr_add.to_string();
                  for_each_connection( [&visitors, &from_addr, &paddr_str]( auto& conn ) {
                     if( conn->socket_is_open()) {
                        if( conn->peer_address().empty()) {
                           ++visitors;
                           if( paddr_str == conn->remote_address()) {
                              ++from_addr;
                           }
                        }
                     }
                     return true;
                  } );
                  if( from_addr < max_nodes_per_host && (max_client_count == 0 || visitors < max_client_count)) {
                     if( new_connection->start_session()) {
                        std::lock_guard<std::shared_mutex> g_unique( connections_mtx );
                        connections.insert( new_connection );
                     }
                  } else {
                     if( from_addr >= max_nodes_per_host ) {
                        fc_elog( logger, "Number of connections (${n}) from ${ra} exceeds limit ${l}",
                                 ("n", from_addr + 1)( "ra", paddr_str )( "l", max_nodes_per_host ));
                     } else {
                        fc_elog( logger, "Error max_client_count ${m} exceeded", ("m", max_client_count));
                     }
                     // new_connection never added to connections and start_session not called, lifetime will end
                     socket->shutdown( tcp::socket::shutdown_both );
                     socket->close();
                  }
               }
            } else {
               fc_elog( logger, "Error accepting connection: ${m}", ("m", ec.message()));
               // For the listed error codes below, recall start_listen_loop()
               switch (ec.value()) {
                  case ECONNABORTED:
                  case EMFILE:
                  case ENFILE:
                  case ENOBUFS:
                  case ENOMEM:
                  case EPROTO:
                     break;
                  default:
                     return;
               }
            }
            start_listen_loop();
         }));
      } );
   }

   // only called from strand thread
   void connection::start_read_message() {
      try {
         std::size_t minimum_read =
               std::atomic_exchange<decltype(outstanding_read_bytes.load())>( &outstanding_read_bytes, 0 );
         minimum_read = minimum_read != 0 ? minimum_read : message_header_size;

         if (my_impl->use_socket_read_watermark) {
            const size_t max_socket_read_watermark = 4096;
            std::size_t socket_read_watermark = std::min<std::size_t>(minimum_read, max_socket_read_watermark);
            boost::asio::socket_base::receive_low_watermark read_watermark_opt(socket_read_watermark);
            socket->set_option(read_watermark_opt);
         }

         auto completion_handler = [minimum_read](boost::system::error_code ec, std::size_t bytes_transferred) -> std::size_t {
            if (ec || bytes_transferred >= minimum_read ) {
               return 0;
            } else {
               return minimum_read - bytes_transferred;
            }
         };

         if( buffer_queue.write_queue_size() > def_max_write_queue_size ||
             trx_in_progress_size > def_max_trx_in_progress_size )
         {
            // too much queued up, reschedule
            uint32_t write_queue_size = buffer_queue.write_queue_size();
            uint32_t trx_in_progress_size = this->trx_in_progress_size.load();
            if( write_queue_size > def_max_write_queue_size ) {
               peer_wlog( this, "write_queue full ${s} bytes", ("s", write_queue_size) );
            } else {
               peer_wlog( this, "max trx in progress ${s} bytes", ("s", trx_in_progress_size) );
            }
            if( write_queue_size > 2*def_max_write_queue_size ||
                trx_in_progress_size > 2*def_max_trx_in_progress_size ||
                ++read_delay_count > def_max_read_delays )
            {
               fc_elog( logger, "queues over full, giving up on connection, closing connection to: ${p}",
                        ("p", peer_name()) );
               close( false );
               return;
            }
            std::lock_guard<std::mutex> g( read_delay_timer_mtx );
            read_delay_timer.cancel();
            read_delay_timer.expires_from_now( def_read_delay_for_full_write_queue );
            connection_wptr weak_conn = shared_from_this();
            read_delay_timer.async_wait( boost::asio::bind_executor(strand, [weak_conn]( boost::system::error_code ec ) {
               if( ec == boost::asio::error::operation_aborted ) return;
               auto conn = weak_conn.lock();
               if( !conn ) return;
               conn->start_read_message();
            } ) );
            return;
         }
         read_delay_count = 0;

         boost::asio::async_read( *socket,
            pending_message_buffer.get_buffer_sequence_for_boost_async_read(), completion_handler,
            boost::asio::bind_executor( strand,
              [conn = shared_from_this(), socket=socket]( boost::system::error_code ec, std::size_t bytes_transferred ) {
               // may have closed connection and cleared pending_message_buffer
               if( !conn->socket_is_open() ) return;

               bool close_connection = false;
               try {
                  if( !ec ) {
                     conn->rec = get_tstamp();
                     if (bytes_transferred > conn->pending_message_buffer.bytes_to_write()) {
                        fc_elog( logger,"async_read_some callback: bytes_transfered = ${bt}, buffer.bytes_to_write = ${btw}",
                                 ("bt",bytes_transferred)("btw",conn->pending_message_buffer.bytes_to_write()) );
                     }
                     EOS_ASSERT(bytes_transferred <= conn->pending_message_buffer.bytes_to_write(), plugin_exception, "");
                     conn->pending_message_buffer.advance_write_ptr(bytes_transferred);
                     while (conn->pending_message_buffer.bytes_to_read() > 0) {
                        uint32_t bytes_in_buffer = conn->pending_message_buffer.bytes_to_read();

                        if (bytes_in_buffer < message_header_size) {
                           conn->outstanding_read_bytes = message_header_size - bytes_in_buffer;
                           break;
                        } else {
                           uint32_t message_length;
                           auto index = conn->pending_message_buffer.read_index();
                           conn->pending_message_buffer.peek(&message_length, sizeof(message_length), index);
                           if(message_length > def_send_buffer_size*2 || message_length == 0) {
                              fc_elog( logger,"incoming message length unexpected (${i})", ("i", message_length) );
                              close_connection = true;
                              break;
                           }

                           auto total_message_bytes = message_length + message_header_size;

                           if (bytes_in_buffer >= total_message_bytes) {
                              conn->pending_message_buffer.advance_read_ptr(message_header_size);
                              conn->consecutive_immediate_connection_close = 0;
                              if (!conn->process_next_message(message_length)) {
                                 return;
                              }
                           } else {
                              auto outstanding_message_bytes = total_message_bytes - bytes_in_buffer;
                              auto available_buffer_bytes = conn->pending_message_buffer.bytes_to_write();
                              if (outstanding_message_bytes > available_buffer_bytes) {
                                 conn->pending_message_buffer.add_space( outstanding_message_bytes - available_buffer_bytes );
                              }

                              conn->outstanding_read_bytes = outstanding_message_bytes;
                              break;
                           }
                        }
                     }
                     if( !close_connection ) conn->start_read_message();
                  } else {
                     if (ec.value() != boost::asio::error::eof) {
                        fc_elog( logger, "Error reading message: ${m}", ( "m", ec.message() ) );
                     } else {
                        fc_ilog( logger, "Peer closed connection" );
                     }
                     close_connection = true;
                  }
               }
               catch(const std::exception &ex) {
                  fc_elog( logger, "Exception in handling read data: ${s}", ("s",ex.what()) );
                  close_connection = true;
               }
               catch(const fc::exception &ex) {
                  fc_elog( logger, "Exception in handling read data ${s}", ("s",ex.to_string()) );
                  close_connection = true;
               }
               catch (...) {
                  fc_elog( logger, "Undefined exception handling read data" );
                  close_connection = true;
               }

               if( close_connection ) {
                  fc_elog( logger, "Closing connection to: ${p}", ("p", conn->peer_name()) );
                  conn->close();
               }
         }));
      } catch (...) {
         fc_elog( logger, "Undefined exception in start_read_message, closing connection to: ${p}", ("p", peer_name()) );
         close();
      }
   }

   // called from connection strand
   bool connection::process_next_message( uint32_t message_length ) {
      try {
         // if next message is a block we already have, exit early
         auto peek_ds = pending_message_buffer.create_peek_datastream();
         unsigned_int which{};
         fc::raw::unpack( peek_ds, which );
         if( which == signed_block_which ) {
            block_header bh;
            fc::raw::unpack( peek_ds, bh );

            block_id_type blk_id = bh.id();
            if( my_impl->dispatcher->have_block( blk_id ) ) {
               connection_ptr c = shared_from_this();
               fc_dlog( logger, "canceling wait on ${p}, already received block ${num}",
                        ("p", c->peer_name())("num", block_header::num_from_id( blk_id )) );
               c->consecutive_rejected_blocks = 0;
               c->cancel_wait();

               pending_message_buffer.advance_read_ptr( message_length );
               return true;
            }
         }

         auto ds = pending_message_buffer.create_datastream();
         net_message msg;
         fc::raw::unpack( ds, msg );
         msg_handler m( shared_from_this() );
         if( msg.contains<signed_block>() ) {
            m( std::move( msg.get<signed_block>() ) );
         }
         else if( msg.contains<packed_transaction>() ) {
            m( std::move( msg.get<packed_transaction>() ) );
         }
         else {

            msg.visit( m );
         }
      } catch( const fc::exception& e ) {
         fc_elog( logger, "Exception in handling message from ${p}: ${s}",
                  ("p", peer_name())("s", e.to_detail_string()) );
         close();
         return false;
      }
      return true;
   }

   // call only from main application thread
   void net_plugin_impl::update_chain_info() {
      controller& cc = chain_plug->chain();
      std::lock_guard<std::mutex> g( chain_info_mtx );
      chain_lib_num = cc.last_irreversible_block_num();
      chain_lib_id = cc.last_irreversible_block_id();
      chain_head_blk_num = cc.head_block_num();
      chain_head_blk_id = cc.head_block_id();
      chain_fork_head_blk_num = cc.fork_db_pending_head_block_num();
      chain_fork_head_blk_id = cc.fork_db_pending_head_block_id();
      fc_dlog( logger, "updating chain info lib ${lib}, head ${head}, fork ${fork}",
               ("lib", chain_lib_num)("head", chain_head_blk_num)("fork", chain_fork_head_blk_num) );
   }

   //         lib_num, head_blk_num, fork_head_blk_num, lib_id, head_blk_id, fork_head_blk_id
   std::tuple<uint32_t, uint32_t, uint32_t, block_id_type, block_id_type, block_id_type>
   net_plugin_impl::get_chain_info() const {
      std::lock_guard<std::mutex> g( chain_info_mtx );
      return std::make_tuple(
            chain_lib_num, chain_head_blk_num, chain_fork_head_blk_num,
            chain_lib_id, chain_head_blk_id, chain_fork_head_blk_id );
   }

   bool connection::is_valid( const handshake_message& msg ) {
      // Do some basic validation of an incoming handshake_message, so things
      // that really aren't handshake messages can be quickly discarded without
      // affecting state.
      bool valid = true;
      if (msg.last_irreversible_block_num > msg.head_num) {
         fc_wlog( logger, "Handshake message validation: last irreversible block (${i}) is greater than head block (${h})",
                  ("i", msg.last_irreversible_block_num)("h", msg.head_num) );
         valid = false;
      }
      if (msg.p2p_address.empty()) {
         fc_wlog( logger, "Handshake message validation: p2p_address is null string" );
         valid = false;
      }
      if (msg.os.empty()) {
         fc_wlog( logger, "Handshake message validation: os field is null string" );
         valid = false;
      }
      if ((msg.sig != chain::signature_type() || msg.token != sha256()) && (msg.token != fc::sha256::hash(msg.time))) {
         fc_wlog( logger, "Handshake message validation: token field invalid" );
         valid = false;
      }
      return valid;
   }

   void connection::handle_message( const chain_size_message& msg ) {
      peer_ilog(this, "received chain_size_message");
   }

   void connection::handle_message( const handshake_message& msg ) {
      peer_ilog( this, "received handshake_message" );
      if( !is_valid( msg ) ) {
         peer_elog( this, "bad handshake message");
         enqueue( go_away_message( fatal_other ) );
         return;
      }
      fc_dlog( logger, "received handshake gen ${g} from ${ep}, lib ${lib}, head ${head}",
               ("g", msg.generation)( "ep", peer_name() )
               ( "lib", msg.last_irreversible_block_num )( "head", msg.head_num ) );

      connecting = false;
      if (msg.generation == 1) {
         if( msg.node_id == node_id) {
            fc_elog( logger, "Self connection detected node_id ${id}. Closing connection", ("id", node_id) );
            enqueue( go_away_message( self ) );
            return;
         }

         if( peer_address().empty() ) {
            set_connection_type( msg.p2p_address );
         }

         std::unique_lock<std::mutex> g_conn( conn_mtx );
         if( peer_address().empty() || last_handshake_recv.node_id == fc::sha256()) {
            g_conn.unlock();
            fc_dlog(logger, "checking for duplicate" );
            std::shared_lock<std::shared_mutex> g_cnts( my_impl->connections_mtx );
            for(const auto& check : my_impl->connections) {
               if(check.get() == this)
                  continue;
               if(check->connected() && check->peer_name() == msg.p2p_address) {
                  // It's possible that both peers could arrive here at relatively the same time, so
                  // we need to avoid the case where they would both tell a different connection to go away.
                  // Using the sum of the initial handshake times of the two connections, we will
                  // arbitrarily (but consistently between the two peers) keep one of them.
                  std::unique_lock<std::mutex> g_check_conn( check->conn_mtx );
                  auto check_time = check->last_handshake_sent.time + check->last_handshake_recv.time;
                  g_check_conn.unlock();
                  g_conn.lock();
                  auto c_time = last_handshake_sent.time;
                  g_conn.unlock();
                  if (msg.time + c_time <= check_time)
                     continue;

                  g_cnts.unlock();
                  fc_dlog( logger, "sending go_away duplicate to ${ep}", ("ep",msg.p2p_address) );
                  go_away_message gam(duplicate);
                  gam.node_id = node_id;
                  enqueue(gam);
                  no_retry = duplicate;
                  my_impl->sync_master->send_handshakes();
                  return;
               }
            }
         } else {
            fc_dlog( logger, "skipping duplicate check, addr == ${pa}, id = ${ni}",
                     ("pa", peer_address())( "ni", last_handshake_recv.node_id ) );
            g_conn.unlock();
         }

         if( msg.chain_id != my_impl->chain_id ) {
            fc_elog( logger, "Peer on a different chain. Closing connection" );
            enqueue( go_away_message(go_away_reason::wrong_chain) );
            return;
         }
         protocol_version = my_impl->to_protocol_version(msg.network_version);
         if( protocol_version != net_version ) {
            if( my_impl->network_version_match ) {
               fc_elog( logger, "Peer network version does not match expected ${nv} but got ${mnv}",
                        ("nv", net_version)("mnv", protocol_version) );
               enqueue( go_away_message( wrong_version ) );
               return;
            } else {
               fc_ilog( logger, "Local network version: ${nv} Remote version: ${mnv}",
                        ("nv", net_version)("mnv", protocol_version));
            }
         }

         if( node_id != msg.node_id ) {
            node_id = msg.node_id;
         }

         if( !my_impl->authenticate_peer( msg ) ) {
            fc_elog( logger, "Peer not authenticated.  Closing connection." );
            enqueue( go_away_message( authentication ) );
            return;
         }
         size_t topo_info = msg.p2p_address.find(topology_enabler);
         if (my_impl->topology_plug != nullptr) {
            link_descriptor ld;
            eosio::node_id topo_peer;
            node_descriptor peer_node;
            peer_node.my_id = 0;
            if (topo_info == string::npos) {
               size_t pos = msg.p2p_address.find(" - ");
               if (pos == string::npos) {
                  fc_elog( logger, "peer has invalid p2p_address '${p}'", ("p",msg.p2p_address));
                  enqueue( go_away_message( authentication ));
                  return;
               }
               string paddr = msg.p2p_address.substr(0,pos);
               peer_node.location = "no topology:" + paddr;
               peer_node.status = no_topology;
               peer_node.role = full;
               stringstream ss;
               ss << msg.network_version;
               peer_node.version = ss.str();
            }
            else {
               topo_info += strlen(topology_enabler);

               string pnjson = msg.p2p_address.substr(topo_info);
               fc_dlog( logger, "parsing node descriptor from ${nd}",("nd",pnjson));
               peer_node = dejsonify<node_descriptor>(pnjson);
               topo_peer = my_impl->topology_plug->make_node_id(msg.node_id);
            }
            topo_peer = my_impl->topology_plug->add_node(move(peer_node));
            eosio::node_id ni = my_impl->topology_plug->make_node_id(my_impl->node_id);
            ld.active = peer_addr.empty() ? topo_peer : ni;
            ld.passive = !peer_addr.empty() ? topo_peer : ni;
            ld.role = combined;
            ld.hops = 1; // need to compute using icmp ttl counts
            ilog("calliing add_link");
            topo_id = my_impl->topology_plug->add_link(move(ld));
         }


         uint32_t peer_lib = msg.last_irreversible_block_num;
         connection_wptr weak = shared_from_this();
         app().post( priority::low, [peer_lib, chain_plug = my_impl->chain_plug, weak{std::move(weak)},
                                     msg_lib_id = msg.last_irreversible_block_id]() {
            connection_ptr c = weak.lock();
            if( !c ) return;
            controller& cc = chain_plug->chain();
            uint32_t lib_num = cc.last_irreversible_block_num();

            fc_dlog( logger, "handshake, check for fork lib_num = ${ln} peer_lib = ${pl}", ("ln", lib_num)( "pl", peer_lib ) );

            if( peer_lib <= lib_num && peer_lib > 0 ) {
               bool on_fork = false;
               try {
                  block_id_type peer_lib_id = cc.get_block_id_for_num( peer_lib );
                  on_fork = (msg_lib_id != peer_lib_id);
               } catch( const unknown_block_exception& ex ) {
                  fc_wlog( logger, "peer last irreversible block ${pl} is unknown", ("pl", peer_lib) );
                  on_fork = true;
               } catch( ... ) {
                  fc_wlog( logger, "caught an exception getting block id for ${pl}", ("pl", peer_lib) );
                  on_fork = true;
               }
               if( on_fork ) {
                  c->strand.post( [c]() {
                     fc_elog( logger, "Peer chain is forked" );
                     c->enqueue( go_away_message( forked ) );
                  } );
               }
            }
         });

         if( sent_handshake_count == 0 ) {
            send_handshake();
         }
      }

      std::unique_lock<std::mutex> g_conn( conn_mtx );
      last_handshake_recv = msg;
      g_conn.unlock();
      _logger_variant.reset();
      my_impl->sync_master->recv_handshake( shared_from_this(), msg );
   }

   void connection::handle_message( const go_away_message& msg ) {
      peer_wlog( this, "received go_away_message, reason = ${r}", ("r", reason_str( msg.reason )) );
      no_retry = msg.reason;
      if( msg.reason == duplicate ) {
         node_id = msg.node_id;
      }
      flush_queues();
      close();
   }

   void connection::handle_message( const time_message& msg ) {
      last_time_recv.dst = get_tstamp();
      last_time_recv.rec = rec;

      /* We've already lost however many microseconds it took to dispatch
       * the message, but it can't be helped.
       */

      peer_ilog(this, "received time_message");
      // If the transmit timestamp is zero, the peer is horribly broken.
      if(msg.xmt == 0)
         return;                 /* invalid timestamp */

      if(msg.xmt == last_time_recv.xmt)
         return;                 /* duplicate packet */
      last_time_recv.org = msg.org;
      last_time_recv.xmt = msg.xmt;
      if( msg.org == 0 ) {
         send_time( msg );
         return;  // We don't have enough data to perform the calculation yet.
      }

      double offset = (double(msg.rec - last_time_sent.org) + double(msg.xmt - msg.dst)) / 2;
      double NsecPerUsec{1000};

      if( logger.is_enabled( fc::log_level::all ) )
         logger.log( FC_LOG_MESSAGE( all, "Clock offset is ${o}ns (${us}us)",
                                     ("o", offset)( "us", offset / NsecPerUsec ) ) );
      last_time_sent.org = 0;
      last_time_recv.rec = 0;
   }

   void connection::handle_message( const notice_message& msg ) {
      // peer tells us about one or more blocks or txns. When done syncing, forward on
      // notices of previously unknown blocks or txns,
      //
      peer_ilog( this, "received notice_message" );
      connecting = false;
      if( msg.known_blocks.ids.size() > 1 ) {
         fc_elog( logger, "Invalid notice_message, known_blocks.ids.size ${s}, closing connection: ${p}",
                  ("s", msg.known_blocks.ids.size())("p", peer_address()) );
         close( false );
         return;
      }
      if( msg.known_trx.mode != none ) {
         fc_dlog( logger, "this is a ${m} notice with ${n} transactions",
                  ("m", modes_str( msg.known_trx.mode ))( "n", msg.known_trx.pending ) );
      }
      switch (msg.known_trx.mode) {
      case none:
         break;
      case last_irr_catch_up: {
         std::unique_lock<std::mutex> g_conn( conn_mtx );
         last_handshake_recv.head_num = msg.known_trx.pending;
         g_conn.unlock();
         break;
      }
      case catch_up : {
         break;
      }
      case normal: {
         my_impl->dispatcher->recv_notice( shared_from_this(), msg, false );
      }
      }

      if( msg.known_blocks.mode != none ) {
         fc_dlog( logger, "this is a ${m} notice with ${n} blocks",
                  ("m", modes_str( msg.known_blocks.mode ))( "n", msg.known_blocks.pending ) );
      }
      switch (msg.known_blocks.mode) {
      case none : {
         break;
      }
      case last_irr_catch_up:
      case catch_up: {
         my_impl->sync_master->sync_recv_notice( shared_from_this(), msg );
         break;
      }
      case normal : {
         my_impl->dispatcher->recv_notice( shared_from_this(), msg, false );
         break;
      }
      default: {
         peer_elog( this, "bad notice_message : invalid known_blocks.mode ${m}",
                    ("m", static_cast<uint32_t>(msg.known_blocks.mode)) );
      }
      }
   }

   void connection::handle_message( const request_message& msg ) {
      if( msg.req_blocks.ids.size() > 1 ) {
         fc_elog( logger, "Invalid request_message, req_blocks.ids.size ${s}, closing ${p}",
                  ("s", msg.req_blocks.ids.size())( "p", peer_name() ) );
         close();
         return;
      }

      switch (msg.req_blocks.mode) {
      case catch_up :
         peer_ilog( this, "received request_message:catch_up" );
         blk_send_branch();
         break;
      case normal :
         peer_ilog( this, "received request_message:normal" );
         if( !msg.req_blocks.ids.empty() ) {
            blk_send( msg.req_blocks.ids.back() );
         }
         break;
      default:;
      }


      switch (msg.req_trx.mode) {
      case catch_up :
         break;
      case none :
         if( msg.req_blocks.mode == none ) {
            stop_send();
         }
         // no break
      case normal :
         if( !msg.req_trx.ids.empty() ) {
            fc_elog( logger, "Invalid request_message, req_trx.ids.size ${s}", ("s", msg.req_trx.ids.size()) );
            close();
            return;
         }
         break;
      default:;
      }
   }

   void connection::handle_message( const sync_request_message& msg ) {
      fc_dlog( logger, "peer requested ${start} to ${end}", ("start", msg.start_block)("end", msg.end_block) );
      if( msg.end_block == 0 ) {
         peer_requested.reset();
         flush_queues();
      } else {
         peer_requested = sync_state( msg.start_block, msg.end_block, msg.start_block-1);
         enqueue_sync_block();
      }
   }

   size_t calc_trx_size( const packed_transaction_ptr& trx ) {
      // transaction is stored packed and unpacked, double packed_size and size of signed as an approximation of use
      return (trx->get_packed_transaction().size() * 2 + sizeof(trx->get_signed_transaction())) * 2 +
             trx->get_packed_context_free_data().size() * 4 +
             trx->get_signatures().size() * sizeof(signature_type);
   }

   void connection::handle_message( const packed_transaction_ptr& trx ) {
      if( my_impl->db_read_mode == eosio::db_read_mode::READ_ONLY ) {
         fc_dlog( logger, "got a txn in read-only mode - dropping" );
         return;
      }

      auto ptrx = std::make_shared<transaction_metadata>( trx, my_impl->chain_plug->chain().configured_subjective_signature_length_limit() );
      const auto& tid = ptrx->id();
      peer_dlog( this, "received packed_transaction ${id}", ("id", tid) );

      bool have_trx = my_impl->dispatcher->have_txn( tid );
      my_impl->dispatcher->recv_transaction( shared_from_this(), ptrx );
      if( have_trx ) {
         fc_dlog( logger, "got a duplicate transaction - dropping ${id}", ("id", tid) );
         return;
      }

      trx_in_progress_size += calc_trx_size( ptrx->packed_trx() );
      my_impl->chain_plug->accept_transaction( ptrx,
            [weak = weak_from_this(), ptrx](const static_variant<fc::exception_ptr, transaction_trace_ptr>& result) {
         // next (this lambda) called from application thread
         bool accepted = false;
         if (result.contains<fc::exception_ptr>()) {
            fc_dlog( logger, "bad packed_transaction : ${m}", ("m", result.get<fc::exception_ptr>()->what()) );
         } else {
            const transaction_trace_ptr& trace = result.get<transaction_trace_ptr>();
            if (!trace->except) {
               fc_dlog( logger, "chain accepted transaction, bcast ${id}", ("id", trace->id) );
               accepted = true;
            }

            if( !accepted ) {
               fc_elog( logger, "bad packed_transaction : ${m}", ("m", trace->except->what()));
            }
         }

         boost::asio::post( my_impl->thread_pool->get_executor(), [accepted, weak{std::move(weak)}, ptrx{std::move(ptrx)}]() {
            if( accepted ) {
               my_impl->dispatcher->bcast_transaction( ptrx );
            } else {
               uint32_t head_blk_num = 0;
               std::tie( std::ignore, head_blk_num, std::ignore, std::ignore, std::ignore, std::ignore ) = my_impl->get_chain_info();
               my_impl->dispatcher->rejected_transaction( ptrx->id(), head_blk_num );
            }
            connection_ptr conn = weak.lock();
            if( conn ) {
               conn->trx_in_progress_size -= calc_trx_size( ptrx->packed_trx() );
            }
         });
      });
   }

   // called from connection strand
   void connection::handle_message( const signed_block_ptr& ptr ) {
      app().post(priority::high, [ptr, weak = weak_from_this()] {
         connection_ptr c = weak.lock();
         if( c ) c->process_signed_block( ptr );
      });
      my_impl->dispatcher->bcast_notice( ptr->id() );
   }

   // called from application thread
   void connection::process_signed_block( const signed_block_ptr& msg ) {
      controller& cc = my_impl->chain_plug->chain();
      block_id_type blk_id = msg->id();
      uint32_t blk_num = msg->block_num();
      // use c in this method instead of this to highlight that all methods called on c-> must be thread safe
      connection_ptr c = shared_from_this();

      // if we have closed connection then stop processing
      if( !c->socket_is_open() )
         return;

      if( my_impl->sync_master->block_while_syncing_with_other_peer(c) )
         return;

      try {
         if( cc.fetch_block_by_id(blk_id) ) {
            c->strand.post( [sync_master = my_impl->sync_master.get(),
                             dispatcher = my_impl->dispatcher.get(), c, blk_id, blk_num]() {
               dispatcher->add_peer_block( blk_id, c->connection_id );
               sync_master->sync_recv_block( c, blk_id, blk_num );
            });
            return;
         }
      } catch( ...) {
         // should this even be caught?
         fc_elog( logger,"Caught an unknown exception trying to recall blockID" );
      }

      fc::microseconds age( fc::time_point::now() - msg->timestamp);
      peer_ilog( c, "received signed_block : #${n} block age in secs = ${age}",
                 ("n", blk_num)( "age", age.to_seconds() ) );

      go_away_reason reason = fatal_other;
      try {
         my_impl->chain_plug->accept_block(msg);
         my_impl->update_chain_info();
         reason = no_reason;
      } catch( const unlinkable_block_exception &ex) {
         peer_elog(c, "bad signed_block : ${m}", ("m",ex.what()));
         reason = unlinkable;
      } catch( const block_validate_exception &ex) {
         peer_elog(c, "bad signed_block : ${m}", ("m",ex.what()));
         fc_elog( logger, "block_validate_exception accept block #${n} syncing from ${p}",("n",blk_num)("p",c->peer_name()) );
         reason = validation;
      } catch( const assert_exception &ex) {
         peer_elog(c, "bad signed_block : ${m}", ("m",ex.what()));
         fc_elog( logger, "unable to accept block on assert exception ${n} from ${p}",("n",ex.to_string())("p",c->peer_name()));
      } catch( const fc::exception &ex) {
         peer_elog(c, "bad signed_block : ${m}", ("m",ex.what()));
         fc_elog( logger, "accept_block threw a non-assert exception ${x} from ${p}",( "x",ex.to_string())("p",c->peer_name()));
      } catch( ...) {
         peer_elog(c, "bad signed_block : unknown exception");
         fc_elog( logger, "handle sync block caught something else from ${p}",("num",blk_num)("p",c->peer_name()));
      }

      if( reason == no_reason ) {
         boost::asio::post( my_impl->thread_pool->get_executor(), [dispatcher = my_impl->dispatcher.get(), c, msg]() {
            dispatcher->add_peer_block( msg->id(), c->connection_id );
            dispatcher->update_txns_block_num( msg );
         });
         c->strand.post( [sync_master = my_impl->sync_master.get(), dispatcher = my_impl->dispatcher.get(), c, blk_id, blk_num]() {
            dispatcher->recv_block( c, blk_id, blk_num );
            sync_master->sync_recv_block( c, blk_id, blk_num );
         });
      } else {
         c->strand.post( [sync_master = my_impl->sync_master.get(), dispatcher = my_impl->dispatcher.get(), c, blk_id, blk_num]() {
            sync_master->rejected_block( c, blk_num );
            dispatcher->rejected_block( blk_id );
         });
      }
   }

   // called from any thread
   void net_plugin_impl::start_conn_timer(boost::asio::steady_timer::duration du, std::weak_ptr<connection> from_connection) {
      if( in_shutdown ) return;
      std::lock_guard<std::mutex> g( connector_check_timer_mtx );
      ++connector_checks_in_flight;
      connector_check_timer->expires_from_now( du );
      connector_check_timer->async_wait( [my = shared_from_this(), from_connection](boost::system::error_code ec) {
            std::unique_lock<std::mutex> g( my->connector_check_timer_mtx );
            int num_in_flight = --my->connector_checks_in_flight;
            g.unlock();
            if( !ec ) {
               my->connection_monitor(from_connection, num_in_flight == 0 );
            } else {
               if( num_in_flight == 0 ) {
                  if( my->in_shutdown ) return;
                  fc_elog( logger, "Error from connection check monitor: ${m}", ("m", ec.message()));
                  my->start_conn_timer( my->connector_period, std::weak_ptr<connection>() );
               }
            }
      });
   }

   // thread safe
   void net_plugin_impl::start_expire_timer() {
      if( in_shutdown ) return;
      std::lock_guard<std::mutex> g( expire_timer_mtx );
      expire_timer->expires_from_now( txn_exp_period);
      expire_timer->async_wait( [my = shared_from_this()]( boost::system::error_code ec ) {
         if( !ec ) {
            my->expire();
         } else {
            if( my->in_shutdown ) return;
            fc_elog( logger, "Error from transaction check monitor: ${m}", ("m", ec.message()) );
            my->start_expire_timer();
         }
      } );
   }

   // thread_safe
   void net_plugin_impl::strobe() {
      if( in_shutdown ) return;
      std::lock_guard<std::mutex> g( sample_timer_mtx );
      sample_timer->expires_from_now(sample_interval);
      sample_timer->async_wait( [my = shared_from_this()]( boost::system::error_code ec ) {
            my->strobe();
            if( ec ) {
               if( my->in_shutdown ) return;
               fc_wlog( logger, "Peer sample strobed sooner than expected: ${m}", ("m", ec.message()) );
            }

            for_each_connection( []( auto& c ) {
               if( c->socket_is_open() ) {
                  c->strand.post( [c]() {
                     c->collect_samples();
                  } );
               }
               return true;
            } );
         } );
   }

   // thread safe
   void net_plugin_impl::ticker() {
      if( in_shutdown ) return;
      std::lock_guard<std::mutex> g( keepalive_timer_mtx );
      keepalive_timer->expires_from_now(keepalive_interval);
      keepalive_timer->async_wait( [my = shared_from_this()]( boost::system::error_code ec ) {
            my->ticker();
            if( ec ) {
               if( my->in_shutdown ) return;
               fc_wlog( logger, "Peer keepalive ticked sooner than expected: ${m}", ("m", ec.message()) );
            }
            for_each_connection( []( auto& c ) {
               if( c->socket_is_open() ) {
                  c->strand.post( [c]() {
                     c->send_time();
                  } );
               }
               return true;
            } );
         } );
   }

   void net_plugin_impl::start_monitors() {
      {
         std::lock_guard<std::mutex> g( connector_check_timer_mtx );
         connector_check_timer.reset(new boost::asio::steady_timer( my_impl->thread_pool->get_executor() ));
      }
      {
         std::lock_guard<std::mutex> g( expire_timer_mtx );
         expire_timer.reset( new boost::asio::steady_timer( my_impl->thread_pool->get_executor() ) );
      }
      start_conn_timer(connector_period, std::weak_ptr<connection>());
      start_expire_timer();
   }

   void net_plugin_impl::expire() {
      auto now = time_point::now();
      uint32_t lib = 0;
      std::tie( lib, std::ignore, std::ignore, std::ignore, std::ignore, std::ignore ) = get_chain_info();
      dispatcher->expire_blocks( lib );
      dispatcher->expire_txns( lib );
      fc_dlog( logger, "expire_txns ${n}us", ("n", time_point::now() - now) );

      start_expire_timer();
   }

   // called from any thread
   void net_plugin_impl::connection_monitor(std::weak_ptr<connection> from_connection, bool reschedule ) {
      auto max_time = fc::time_point::now();
      max_time += fc::milliseconds(max_cleanup_time_ms);
      auto from = from_connection.lock();
      std::unique_lock<std::shared_mutex> g( connections_mtx );
      auto it = (from ? connections.find(from) : connections.begin());
      if (it == connections.end()) it = connections.begin();
      size_t num_rm = 0;
      while (it != connections.end()) {
         if (fc::time_point::now() >= max_time) {
            connection_wptr wit = *it;
            g.unlock();
            fc_dlog( logger, "Exiting connection monitor early, ran out of time: ${t}", ("t", max_time - fc::time_point::now()) );
            if( reschedule ) {
               start_conn_timer( std::chrono::milliseconds( 1 ), wit ); // avoid exhausting
            }
            return;
         }
         if( !(*it)->socket_is_open() && !(*it)->connecting) {
            if( (*it)->peer_address().length() > 0) {
               if( !(*it)->resolve_and_connect() ) {
                  it = connections.erase(it);
                  continue;
               }
            } else {
               it = connections.erase(it);
               continue;
            }
         }
         ++it;
      }
      g.unlock();
      fc_dlog( logger, "connection monitor, removed ${n} connections", ("n", num_rm) );
      if( reschedule ) {
         start_conn_timer( connector_period, std::weak_ptr<connection>());
      }
   }

   void net_plugin_impl::on_topology_update(const topology_message& tm) {
      update_chain_info();
      boost::asio::post( my_impl->thread_pool->get_executor(), [this, tm]() {
            dispatcher->bcast_topology_message( tm );
      });
   }

   // called from application thread
   void net_plugin_impl::on_accepted_block(const block_state_ptr& block) {
      update_chain_info();
      dispatcher->strand.post( [this, block]() {
         fc_dlog( logger, "signaled, blk num = ${num}, id = ${id}", ("num", block->block_num)("id", block->id) );
         dispatcher->bcast_block( block );
      });
   }

   // called from application thread
   void net_plugin_impl::on_irreversible_block( const block_state_ptr& block) {
      fc_dlog( logger, "on_irreversible_block, blk num = ${num}, id = ${id}", ("num", block->block_num)("id", block->id) );
      update_chain_info();
   }

   // called from application thread
   void net_plugin_impl::transaction_ack(const std::pair<fc::exception_ptr, transaction_metadata_ptr>& results) {
      boost::asio::post( my_impl->thread_pool->get_executor(), [this, results]() {
         const auto& id = results.second->id();
         if (results.first) {
            fc_dlog( logger, "signaled NACK, trx-id = ${id} : ${why}", ("id", id)( "why", results.first->to_detail_string() ) );

            uint32_t head_blk_num = 0;
            std::tie( std::ignore, head_blk_num, std::ignore, std::ignore, std::ignore, std::ignore ) = get_chain_info();
            dispatcher->rejected_transaction(id, head_blk_num);
         } else {
            fc_dlog( logger, "signaled ACK, trx-id = ${id}", ("id", id) );
            dispatcher->bcast_transaction(results.second);
         }
      });
   }

   bool net_plugin_impl::authenticate_peer(const handshake_message& msg) const {
      if(allowed_connections == None)
         return false;

      if(allowed_connections == Any)
         return true;

      if(allowed_connections & (Producers | Specified)) {
         auto allowed_it = std::find(allowed_peers.begin(), allowed_peers.end(), msg.key);
         auto private_it = private_keys.find(msg.key);
         bool found_producer_key = false;
         if(producer_plug != nullptr)
            found_producer_key = producer_plug->is_producer_key(msg.key);
         if( allowed_it == allowed_peers.end() && private_it == private_keys.end() && !found_producer_key) {
            fc_elog( logger, "Peer ${peer} sent a handshake with an unauthorized key: ${key}.",
                     ("peer", msg.p2p_address)("key", msg.key) );
            return false;
         }
      }

      namespace sc = std::chrono;
      sc::system_clock::duration msg_time(msg.time);
      auto time = sc::system_clock::now().time_since_epoch();
      if(time - msg_time > peer_authentication_interval) {
         fc_elog( logger, "Peer ${peer} sent a handshake with a timestamp skewed by more than ${time}.",
                  ("peer", msg.p2p_address)("time", "1 second")); // TODO Add to_variant for std::chrono::system_clock::duration
         return false;
      }

      if(msg.sig != chain::signature_type() && msg.token != sha256()) {
         sha256 hash = fc::sha256::hash(msg.time);
         if(hash != msg.token) {
            fc_elog( logger, "Peer ${peer} sent a handshake with an invalid token.", ("peer", msg.p2p_address) );
            return false;
         }
         chain::public_key_type peer_key;
         try {
            peer_key = crypto::public_key(msg.sig, msg.token, true);
         }
         catch (fc::exception& /*e*/) {
            fc_elog( logger, "Peer ${peer} sent a handshake with an unrecoverable key.", ("peer", msg.p2p_address) );
            return false;
         }
         if((allowed_connections & (Producers | Specified)) && peer_key != msg.key) {
            fc_elog( logger, "Peer ${peer} sent a handshake with an unauthenticated key.", ("peer", msg.p2p_address) );
            return false;
         }
      }
      else if(allowed_connections & (Producers | Specified)) {
         fc_dlog( logger, "Peer sent a handshake with blank signature and token, but this node accepts only authenticated connections." );
         return false;
      }
      return true;
   }

   chain::public_key_type net_plugin_impl::get_authentication_key() const {
      if(!private_keys.empty())
         return private_keys.begin()->first;
      /*producer_plugin* pp = app().find_plugin<producer_plugin>();
      if(pp != nullptr && pp->get_state() == abstract_plugin::started)
         return pp->first_producer_public_key();*/
      return chain::public_key_type();
   }

   chain::signature_type net_plugin_impl::sign_compact(const chain::public_key_type& signer, const fc::sha256& digest) const
   {
      auto private_key_itr = private_keys.find(signer);
      if(private_key_itr != private_keys.end())
         return private_key_itr->second.sign(digest);
      if(producer_plug != nullptr && producer_plug->get_state() == abstract_plugin::started)
         return producer_plug->sign_compact(signer, digest);
      return chain::signature_type();
   }

   // call from connection strand
   void connection::populate_handshake( handshake_message& hello ) {
      hello.network_version = net_version_base + net_version;
      hello.chain_id = my_impl->chain_id;
      hello.node_id = my_impl->node_id;
      hello.key = my_impl->get_authentication_key();
      hello.time = std::chrono::system_clock::now().time_since_epoch().count();
      hello.token = fc::sha256::hash(hello.time);
      hello.sig = my_impl->sign_compact(hello.key, hello.token);
      // If we couldn't sign, don't send a token.
      if(hello.sig == chain::signature_type())
         hello.token = sha256();
      hello.p2p_address = my_impl->p2p_address;
      if( is_transactions_only_connection() ) hello.p2p_address += ":trx";
      if( is_blocks_only_connection() ) hello.p2p_address += ":blk";
      hello.p2p_address += " - ";
      if (my_impl->topology_plug == nullptr) {
         hello.p2p_address += hello.node_id.str().substr(0,7);
      }
      else {
         node_descriptor nd;
         nd.my_id = my_impl->topology_plug->make_node_id(my_impl->node_id);
         nd.location = my_impl->topology_plug->bp_name() + ":" + my_impl->p2p_address;
         nd.role = producer;
         nd.status = running;
         stringstream ss;
         ss << hello.network_version;
         nd.version = ss.str();
         //nd.producers = ...
         string node_desc_str = fc::json::to_string(nd);
         hello.p2p_address += topology_enabler + node_desc_str;
      }
      fc_dlog(logger,"set p2p_address = ${p2p}",("p2p", hello.p2p_address));
#if defined( __APPLE__ )
      hello.os = "osx";
#elif defined( __linux__ )
      hello.os = "linux";
#elif defined( _WIN32 )
      hello.os = "win32";
#else
      hello.os = "other";
#endif
      hello.agent = my_impl->user_agent_name;

      std::tie( hello.last_irreversible_block_num, std::ignore, hello.head_num,
                hello.last_irreversible_block_id, std::ignore, hello.head_id ) = my_impl->get_chain_info();
   }

   net_plugin::net_plugin()
      :my( new net_plugin_impl ) {
      my_impl = my.get();
   }

   net_plugin::~net_plugin() {
   }

   void net_plugin::set_program_options( options_description& /*cli*/, options_description& cfg )
   {
      cfg.add_options()
         ( "p2p-listen-endpoint", bpo::value<string>()->default_value( "0.0.0.0:9876" ), "The actual host:port used to listen for incoming p2p connections.")
         ( "p2p-server-address", bpo::value<string>(), "An externally accessible host:port for identifying this node. Defaults to p2p-listen-endpoint.")
         ( "p2p-peer-address", bpo::value< vector<string> >()->composing(),
           "The public endpoint of a peer node to connect to. Use multiple p2p-peer-address options as needed to compose a network.\n"
           "  Syntax: host:port[:<trx>|<blk>]\n"
           "  The optional 'trx' and 'blk' indicates to node that only transactions 'trx' or blocks 'blk' should be sent."
           "  Examples:\n"
           "    p2p.eos.io:9876\n"
           "    p2p.trx.eos.io:9876:trx\n"
           "    p2p.blk.eos.io:9876:blk\n")
         ( "p2p-max-nodes-per-host", bpo::value<int>()->default_value(def_max_nodes_per_host), "Maximum number of client nodes from any single IP address")
         ( "agent-name", bpo::value<string>()->default_value("\"EOS Test Agent\""), "The name supplied to identify this node amongst the peers.")
         ( "allowed-connection", bpo::value<vector<string>>()->multitoken()->default_value({"any"}, "any"), "Can be 'any' or 'producers' or 'specified' or 'none'. If 'specified', peer-key must be specified at least once. If only 'producers', peer-key is not required. 'producers' and 'specified' may be combined.")
         ( "peer-key", bpo::value<vector<string>>()->composing()->multitoken(), "Optional public key of peer allowed to connect.  May be used multiple times.")
         ( "peer-private-key", boost::program_options::value<vector<string>>()->composing()->multitoken(),
           "Tuple of [PublicKey, WIF private key] (may specify multiple times)")
         ( "max-clients", bpo::value<int>()->default_value(def_max_clients), "Maximum number of clients from which connections are accepted, use 0 for no limit")
         ( "connection-cleanup-period", bpo::value<int>()->default_value(def_conn_retry_wait), "number of seconds to wait before cleaning up dead connections")
         ( "max-cleanup-time-msec", bpo::value<int>()->default_value(10), "max connection cleanup time per cleanup call in millisec")
         ( "network-version-match", bpo::value<bool>()->default_value(false),
           "True to require exact match of peer network version.")
         ( "net-threads", bpo::value<uint16_t>()->default_value(my->thread_pool_size),
           "Number of worker threads in net_plugin thread pool" )
         ( "sync-fetch-span", bpo::value<uint32_t>()->default_value(def_sync_fetch_span), "number of blocks to retrieve in a chunk from any individual peer during synchronization")
         ( "use-socket-read-watermark", bpo::value<bool>()->default_value(false), "Enable expirimental socket read watermark optimization")
         ( "peer-log-format", bpo::value<string>()->default_value( "[\"${_name}\" ${_ip}:${_port}]" ),
           "The string used to format peers when logging messages about them.  Variables are escaped with ${<variable name>}.\n"
           "Available Variables:\n"
           "   _name  \tself-reported name\n\n"
           "   _id    \tself-reported ID (64 hex characters)\n\n"
           "   _sid   \tfirst 8 characters of _peer.id\n\n"
           "   _ip    \tremote IP address of peer\n\n"
           "   _port  \tremote port number of peer\n\n"
           "   _lip   \tlocal IP address connected to peer\n\n"
           "   _lport \tlocal port number connected to peer\n\n")
        ;
   }

   void net_plugin::plugin_initialize( const variables_map& options ) {
      fc_ilog( logger, "Initialize net plugin" );
      try {
         peer_log_format = options.at( "peer-log-format" ).as<string>();

         my->network_version_match = options.at( "network-version-match" ).as<bool>();

         my->sync_master.reset( new sync_manager( options.at( "sync-fetch-span" ).as<uint32_t>()));

         my->connector_period = std::chrono::seconds( options.at( "connection-cleanup-period" ).as<int>());
         my->max_cleanup_time_ms = options.at("max-cleanup-time-msec").as<int>();
         my->txn_exp_period = def_txn_expire_wait;
         my->resp_expected_period = def_resp_expected_wait;
         my->max_client_count = options.at( "max-clients" ).as<int>();
         my->max_nodes_per_host = options.at( "p2p-max-nodes-per-host" ).as<int>();

         my->use_socket_read_watermark = options.at( "use-socket-read-watermark" ).as<bool>();

         if( options.count( "p2p-listen-endpoint" ) && options.at("p2p-listen-endpoint").as<string>().length()) {
            my->p2p_address = options.at( "p2p-listen-endpoint" ).as<string>();
         }
         if( options.count( "p2p-server-address" ) ) {
            my->p2p_server_address = options.at( "p2p-server-address" ).as<string>();
         }

         my->thread_pool_size = options.at( "net-threads" ).as<uint16_t>();
         EOS_ASSERT( my->thread_pool_size > 0, chain::plugin_config_exception,
                     "net-threads ${num} must be greater than 0", ("num", my->thread_pool_size) );

         if( options.count( "p2p-peer-address" )) {
            my->supplied_peers = options.at( "p2p-peer-address" ).as<vector<string> >();
         }
         if( options.count( "agent-name" )) {
            my->user_agent_name = options.at( "agent-name" ).as<string>();
         }

         if( options.count( "allowed-connection" )) {
            const std::vector<std::string> allowed_remotes = options["allowed-connection"].as<std::vector<std::string>>();
            for( const std::string& allowed_remote : allowed_remotes ) {
               if( allowed_remote == "any" )
                  my->allowed_connections |= net_plugin_impl::Any;
               else if( allowed_remote == "producers" )
                  my->allowed_connections |= net_plugin_impl::Producers;
               else if( allowed_remote == "specified" )
                  my->allowed_connections |= net_plugin_impl::Specified;
               else if( allowed_remote == "none" )
                  my->allowed_connections = net_plugin_impl::None;
            }
         }

         if( my->allowed_connections & net_plugin_impl::Specified )
            EOS_ASSERT( options.count( "peer-key" ),
                        plugin_config_exception,
                       "At least one peer-key must accompany 'allowed-connection=specified'" );

         if( options.count( "peer-key" )) {
            const std::vector<std::string> key_strings = options["peer-key"].as<std::vector<std::string>>();
            for( const std::string& key_string : key_strings ) {
               my->allowed_peers.push_back( dejsonify<chain::public_key_type>( key_string ));
            }
         }

         if( options.count( "peer-private-key" )) {
            const std::vector<std::string> key_id_to_wif_pair_strings = options["peer-private-key"].as<std::vector<std::string>>();
            for( const std::string& key_id_to_wif_pair_string : key_id_to_wif_pair_strings ) {
               auto key_id_to_wif_pair = dejsonify<std::pair<chain::public_key_type, std::string>>(
                     key_id_to_wif_pair_string );
               my->private_keys[key_id_to_wif_pair.first] = fc::crypto::private_key( key_id_to_wif_pair.second );
            }
         }

         my->chain_plug = app().find_plugin<chain_plugin>();
         EOS_ASSERT( my->chain_plug, chain::missing_chain_plugin_exception, ""  );
         my->chain_id = my->chain_plug->get_chain_id();
         fc::rand_pseudo_bytes( my->node_id.data(), my->node_id.data_size());

      } FC_LOG_AND_RETHROW()
   }

   void net_plugin::plugin_startup() {
      handle_sighup();

      fc_ilog( logger, "my node_id is ${id}", ("id", my->node_id ));

      my->producer_plug = app().find_plugin<producer_plugin>();
      my->topology_plug = app().find_plugin<topology_plugin>();
      if( my->topology_plug != nullptr ) {
         my->topology_plug->topo_update.connect([my = my]( const topology_message & t) {
                                                    my->on_topology_update ( t );
                                                 });
      }
      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT( my->chain_plug, chain::missing_chain_plugin_exception, ""  );
      my->chain_id = my->chain_plug->get_chain_id();
      fc::rand_pseudo_bytes( my->node_id.data(), my->node_id.data_size());
      ilog( "my node_id is ${id}", ("id", my->node_id));

      my->thread_pool.emplace( "net", my->thread_pool_size );

      my->dispatcher.reset( new dispatch_manager( my_impl->thread_pool->get_executor() ) );

      tcp::endpoint listen_endpoint;
      if( my->p2p_address.size() > 0 ) {
         auto host = my->p2p_address.substr( 0, my->p2p_address.find( ':' ));
         auto port = my->p2p_address.substr( host.size() + 1, my->p2p_address.size());
         tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str());
         // Note: need to add support for IPv6 too?

         tcp::resolver resolver( my->thread_pool->get_executor() );
         listen_endpoint = *resolver.resolve( query );

         my->acceptor.reset( new tcp::acceptor( my_impl->thread_pool->get_executor() ) );

         if( !my->p2p_server_address.empty() ) {
            my->p2p_address = my->p2p_server_address;
         } else {
            if( listen_endpoint.address().to_v4() == address_v4::any()) {
               boost::system::error_code ec;
               auto host = host_name( ec );
               if( ec.value() != boost::system::errc::success ) {

                  FC_THROW_EXCEPTION( fc::invalid_arg_exception,
                                      "Unable to retrieve host_name. ${msg}", ("msg", ec.message()));

               }
               auto port = my->p2p_address.substr( my->p2p_address.find( ':' ), my->p2p_address.size());
               my->p2p_address = host + port;
            }
         }
      }

      if (my->topology_plug != nullptr) {
         node_descriptor nd;
         nd.my_id = 0;
         fc_ilog( logger,"init nd.id = ${id}",("id",nd.my_id));
         nd.location = my->topology_plug->bp_name() + ":" + my->p2p_address;
         eosio::chain_apis::read_only::get_producers_params parm({false, "", 100});
         auto prodlist = my->chain_plug->get_read_only_api().get_producers(parm);
         if ( prodlist.rows.empty() ) {
            nd.role = full;
            nd.status = standby;
         } else {
            nd.role = producer;
            nd.status = running;
            for (auto &row : prodlist.rows) {
               string prodname = row["owner"].as_string();
               nd.producers.push_back( chain::name(prodname) );
            }
         }
         my->node_id = my->topology_plug->gen_long_id(nd);
         uint64_t sid = my->topology_plug->make_node_id(my->node_id);
         fc_ilog( logger, "setting up local node ${id}",("id",sid) );
         eosio::node_id shortid = my->topology_plug->add_node( std::move(nd) );
         my->topology_plug->set_local_node_id(shortid);

         {
            std::lock_guard<std::mutex> g( my->keepalive_timer_mtx );
            my->sample_timer.reset( new boost::asio::steady_timer(my->thread_pool->get_executor() ));
         }
         my->sample_interval = std::chrono::seconds( my->topology_plug->sample_interval_sec());
         my->strobe();
      }

      {
         std::lock_guard<std::mutex> g( my->keepalive_timer_mtx );
         my->keepalive_timer.reset( new boost::asio::steady_timer( my->thread_pool->get_executor() ) );
      }
      my->ticker();

      if( my->acceptor ) {
         my->acceptor->open(listen_endpoint.protocol());
         my->acceptor->set_option(tcp::acceptor::reuse_address(true));
         try {
           my->acceptor->bind(listen_endpoint);
         } catch (const std::exception& e) {
           fc_elog( logger, "net_plugin::plugin_startup failed to bind to port ${port}",
                    ("port", listen_endpoint.port()));
           throw e;
         }
         my->acceptor->listen();
         fc_ilog( logger, "starting listener, max clients is ${mc}",("mc",my->max_client_count) );
         my->start_listen_loop();
      }
      chain::controller&cc = my->chain_plug->chain();
      {
         cc.accepted_block.connect( [my = my]( const block_state_ptr& s ) {
            my->on_accepted_block( s );
         } );
         cc.irreversible_block.connect( [my = my]( const block_state_ptr& s ) {
            my->on_irreversible_block( s );
         } );
      }

      my->incoming_transaction_ack_subscription = app().get_channel<compat::channels::transaction_ack>().subscribe(
            boost::bind(&net_plugin_impl::transaction_ack, my.get(), _1));

      my->db_read_mode = cc.get_read_mode();
      if( my->db_read_mode == chain::db_read_mode::READ_ONLY ) {
         my->max_nodes_per_host = 0;
         fc_ilog( logger, "node in read-only mode setting max_nodes_per_host to 0 to prevent connections" );
      }

      my->start_monitors();

      my->update_chain_info();

      for( const auto& seed_node : my->supplied_peers ) {
         connect( seed_node );
      }
   }

   void net_plugin::handle_sighup() {
      fc::logger::update( logger_name, logger );
   }

   void net_plugin::plugin_shutdown() {
      try {
         fc_ilog( logger, "shutdown.." );
         my->in_shutdown = true;
         {
            std::lock_guard<std::mutex> g( my->connector_check_timer_mtx );
            if( my->connector_check_timer )
               my->connector_check_timer->cancel();
         }{
            std::lock_guard<std::mutex> g( my->expire_timer_mtx );
            if( my->expire_timer )
               my->expire_timer->cancel();
         }{
            std::lock_guard<std::mutex> g( my->keepalive_timer_mtx );
            if( my->keepalive_timer )
               my->keepalive_timer->cancel();
         }

         if( my->acceptor ) {
            boost::system::error_code ec;
            my->acceptor->cancel( ec );
            my->acceptor->close( ec );
         }

         {
            fc_ilog( logger, "close ${s} connections", ("s", my->connections.size()) );
            std::lock_guard<std::shared_mutex> g( my->connections_mtx );
            for( auto& con : my->connections ) {
               fc_dlog( logger, "close: ${p}", ("p", con->peer_name()) );
               con->close( false );
            }
            my->connections.clear();
         }

         if( my->thread_pool ) {
            my->thread_pool->stop();
         }
         app().post( 0, [me = my](){} ); // keep my pointer alive until queue is drained
         fc_ilog( logger, "exit shutdown" );
      }
      FC_CAPTURE_AND_RETHROW()
   }

   /**
    *  Used to trigger a new connection from RPC API
    */
   string net_plugin::connect( const string& host ) {
      std::lock_guard<std::shared_mutex> g( my->connections_mtx );
      if( my->find_connection( host ) )
         return "already connected";

      connection_ptr c = std::make_shared<connection>( host );
      fc_dlog( logger, "calling active connector: ${h}", ("h", host) );
      if( c->resolve_and_connect() ) {
         fc_dlog( logger, "adding new connection to the list: ${c}", ("c", c->peer_name()) );
         my->connections.insert( c );
      }
      return "added connection";
   }

   string net_plugin::disconnect( const string& host ) {
      std::lock_guard<std::shared_mutex> g( my->connections_mtx );
      for( auto itr = my->connections.begin(); itr != my->connections.end(); ++itr ) {
         if( (*itr)->peer_address() == host ) {
            fc_ilog( logger, "disconnecting: ${p}", ("p", (*itr)->peer_name()) );
            (*itr)->close();
            my->connections.erase(itr);
            return "connection removed";
         }
      }
      return "no known connection for host";
   }

   optional<connection_status> net_plugin::status( const string& host )const {
      std::shared_lock<std::shared_mutex> g( my->connections_mtx );
      auto con = my->find_connection( host );
      if( con )
         return con->get_status();
      return optional<connection_status>();
   }

   vector<connection_status> net_plugin::connections()const {
      vector<connection_status> result;
      std::shared_lock<std::shared_mutex> g( my->connections_mtx );
      result.reserve( my->connections.size() );
      for( const auto& c : my->connections ) {
         result.push_back( c->get_status() );
      }
      return result;
   }

   // call with connections_mtx
   connection_ptr net_plugin_impl::find_connection( const string& host )const {
      for( const auto& c : connections )
         if( c->peer_address() == host ) return c;
      return connection_ptr();
   }

   constexpr uint16_t net_plugin_impl::to_protocol_version(uint16_t v) {
      if (v >= net_version_base) {
         v -= net_version_base;
         return (v > net_version_range) ? 0 : v;
      }
      return 0;
   }

}
