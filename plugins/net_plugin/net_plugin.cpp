/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/types.hpp>

#include <eosio/net_plugin/net_plugin.hpp>
#include <eosio/net_plugin/protocol.hpp>
#include <eosio/net_plugin/message_buffer.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/utilities/key_conversion.hpp>
#include <eosio/chain/contracts/types.hpp>

#include <fc/network/ip.hpp>
#include <fc/io/json.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/appender.hpp>
#include <fc/container/flat.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/exception/exception.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/intrusive/set.hpp>

namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
}

namespace eosio {
   static appbase::abstract_plugin& _net_plugin = app().register_plugin<net_plugin>();

   using std::vector;

   using boost::asio::ip::tcp;
   using boost::asio::ip::address_v4;
   using boost::asio::ip::host_name;
   using boost::intrusive::rbtree;

   using fc::time_point;
   using fc::time_point_sec;
   using eosio::chain::transaction_id_type;
   namespace bip = boost::interprocess;
   using chain::contracts::uint16;

   class connection;
   class sync_manager;
   class big_msg_manager;

   using connection_ptr = std::shared_ptr<connection>;
   using connection_wptr = std::weak_ptr<connection>;

   using socket_ptr = std::shared_ptr<tcp::socket>;

   using net_message_ptr = shared_ptr<net_message>;

   struct node_transaction_state {
      transaction_id_type id;
      time_point_sec  expires;  /// time after which this may be purged.
                                /// Expires increased while the txn is
                                /// "in flight" to anoher peer
      packed_transaction packed_txn;
      vector<char>    serialized_txn; /// the received raw bundle
      uint32_t        block_num = 0; /// block transaction was included in
      uint32_t        true_block = 0; /// used to reset block_uum when request is 0
      uint16_t        requests = 0; /// the number of "in flight" requests for this txn
   };

   struct update_entry {
      const packed_transaction &txn;
      update_entry(const packed_transaction &msg) : txn(msg) {}

      void operator() (node_transaction_state& nts) {
         nts.packed_txn = txn;
         net_message msg(txn);
         uint32_t packsiz = fc::raw::pack_size(msg);
         uint32_t bufsiz = packsiz + sizeof(packsiz);
         nts.serialized_txn.resize(bufsiz);
         fc::datastream<char*> ds( nts.serialized_txn.data(), bufsiz );
         ds.write( reinterpret_cast<char*>(&packsiz), sizeof(packsiz) );
         fc::raw::pack( ds, msg );
      }
   };

   struct update_in_flight {
      int32_t incr;
      update_in_flight (int32_t delta) : incr (delta) {}
      void operator() (node_transaction_state& nts) {
         int32_t exp = nts.expires.sec_since_epoch();
         nts.expires = fc::time_point_sec (exp + incr * 60);
         if( nts.requests == 0 ) {
            nts.true_block = nts.block_num;
            nts.block_num = 0;
         }
         nts.requests += incr;
         if( nts.requests == 0 ) {
            nts.block_num = nts.true_block;
         }
      }
   } incr_in_flight(1), decr_in_flight(-1);

   struct by_expiry;
   struct by_block_num;

   typedef multi_index_container<
      node_transaction_state,
      indexed_by<
         ordered_unique<
            tag< by_id >,
            member < node_transaction_state,
                     transaction_id_type,
                     &node_transaction_state::id > >,
         ordered_non_unique<
            tag< by_expiry >,
            member< node_transaction_state,
                    fc::time_point_sec,
                    &node_transaction_state::expires >
            >,

         ordered_non_unique<
            tag<by_block_num>,
            member< node_transaction_state,
                    uint32_t,
                    &node_transaction_state::block_num > >
         >
      >
   node_transaction_index;

   class net_plugin_impl {
   public:
      unique_ptr<tcp::acceptor>        acceptor;
      tcp::endpoint                    listen_endpoint;
      string                           p2p_address;
      uint32_t                         max_client_count = 0;
      uint32_t                         num_clients = 0;

      vector<string>                   supplied_peers;
      vector<chain::public_key_type>   allowed_peers; ///< peer keys allowed to connect
      std::map<chain::public_key_type,
               chain::private_key_type> private_keys; ///< overlapping with producer keys, also authenticating non-producing nodes

      enum possible_connections : char {
         None = 0,
            Producers = 1 << 0,
            Specified = 1 << 1,
            Any = 1 << 2
            };
      possible_connections             allowed_connections{None};

      connection_ptr find_connection( string host )const;

      std::set< connection_ptr >       connections;
      bool                             done = false;
      unique_ptr< sync_manager >       sync_master;
      unique_ptr< big_msg_manager >    big_msg_master;

      unique_ptr<boost::asio::steady_timer> connector_check;
      unique_ptr<boost::asio::steady_timer> transaction_check;
      unique_ptr<boost::asio::steady_timer> keepalive_timer;
      boost::asio::steady_timer::duration   connector_period;
      boost::asio::steady_timer::duration   txn_exp_period;
      boost::asio::steady_timer::duration   resp_expected_period;
      boost::asio::steady_timer::duration   keepalive_interval{std::chrono::seconds{32}};

      const std::chrono::system_clock::duration peer_authentication_interval{std::chrono::seconds{1}}; ///< Peer clock may be no more than 1 second skewed from our clock, including network latency.

      int16_t                       network_version = 0;
      bool                          network_version_match = false;
      chain_id_type                 chain_id;
      fc::sha256                    node_id;

      string                        user_agent_name;
      chain_plugin*                 chain_plug;
      bool                          send_whole_blocks = false;
      int                           started_sessions = 0;

      node_transaction_index        local_txns;

      shared_ptr<tcp::resolver>     resolver;

      void connect( connection_ptr c );
      void connect( connection_ptr c, tcp::resolver::iterator endpoint_itr );
      void start_session( connection_ptr c );
      void start_listen_loop( );
      void start_read_message( connection_ptr c);

      void   close( connection_ptr c );
      size_t count_open_sockets() const;

      template<typename VerifierFunc>
      void send_all( const net_message &msg, VerifierFunc verify );

      static void transaction_ready( const transaction_metadata&, const packed_transaction& txn);
      void broadcast_block_impl( const signed_block &sb);

      bool is_valid( const handshake_message &msg);

      void handle_message( connection_ptr c, const handshake_message &msg);
      void handle_message( connection_ptr c, const go_away_message &msg );
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
      void handle_message( connection_ptr c, const time_message &msg);
      /** @} */
      void handle_message( connection_ptr c, const notice_message &msg);
      void handle_message( connection_ptr c, const request_message &msg);
      void handle_message( connection_ptr c, const sync_request_message &msg);
      void handle_message( connection_ptr c, const signed_block_summary &msg);
      void handle_message( connection_ptr c, const signed_block &msg);
      void handle_message( connection_ptr c, const packed_transaction &msg);
      void handle_message( connection_ptr c, const signed_transaction &msg);

      void start_conn_timer( );
      void start_txn_timer( );
      void start_monitors( );

      void expire_txns( );
      void connection_monitor( );
      /** \name Peer Timestamps
       *  Time message handling
       *  @{
       */
      /** \brief Peer heartbeat ticker.
       */
      void ticker();
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

      // static const fc::string logger_name;
      // static fc::logger logger;
   };

   const fc::string logger_name("net_plugin_impl");
   fc::logger logger(logger_name);

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
   constexpr auto     def_max_clients = 25; // 0 for unlimited clients
   constexpr auto     def_conn_retry_wait = 30;
   constexpr auto     def_txn_expire_wait = std::chrono::seconds(3);
   constexpr auto     def_resp_expected_wait = std::chrono::seconds(1);
   constexpr auto     def_sync_fetch_span = 100;
   constexpr auto     def_max_just_send = 1500; // "mtu" * 1
   constexpr auto     def_send_whole_blocks = true;

   constexpr auto     message_header_size = 4;

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
      bool                is_noticed_to_peer = false; ///< have we sent peer notice we know it (true if we receive from this peer)
      uint32_t            block_num = 0; ///< the block number the transaction was included in
      time_point          requested_time; /// in case we fetch large trx
   };

   struct update_block_num {
      uint32_t new_bnum;
      update_block_num(uint32_t bnum) : new_bnum(bnum) {}
      void operator() (node_transaction_state& nts) {
         if (nts.requests ) {
            nts.true_block = new_bnum;
         }
         else {
            nts.block_num = new_bnum;
         }
      }
      void operator() (transaction_state& ts) {
         ts.block_num = new_bnum;
      }
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
      bool          is_noticed;
      time_point    requested_time;
   };

   struct update_request_time {
      void operator() (struct transaction_state &ts) {
         ts.requested_time = time_point::now();
      }
      void operator () (struct block_state &bs) {
         bs.requested_time = time_point::now();
      }
   } set_request_time;

   typedef multi_index_container<
      block_state,
      indexed_by<
         ordered_unique< tag<by_id>, member<block_state, block_id_type, &block_state::id > >
         >
      > block_state_index;


   struct update_known_by_peer {
      void operator() (block_state& bs) {
         bs.is_known = true;
      }
      void operator() (transaction_state& ts) {
         ts.is_known_by_peer = true;
      }
   } set_is_known;

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

   struct handshake_initializer {
      static void populate(handshake_message &hello);
   };

   class connection : public std::enable_shared_from_this<connection> {
   public:
      explicit connection( string endpoint );

      explicit connection( socket_ptr s );
      ~connection();
      void initialize();

      block_state_index       blk_state;
      transaction_state_index trx_state;
      optional<sync_state>    peer_requested;  // this peer is requesting info from us
      socket_ptr              socket;

      message_buffer<1024*1024>    pending_message_buffer;
      vector<char>            blk_buffer;

      struct queued_write {
         std::shared_ptr<vector<char>> buff;
         std::function<void(boost::system::error_code, std::size_t)> cb;
      };
      deque<queued_write>     write_queue;

      fc::sha256              node_id;
      handshake_message       last_handshake_recv;
      handshake_message       last_handshake_sent;
      int16_t                 sent_handshake_count;
      bool                    connecting;
      bool                    syncing;
      int                     write_depth;
      string                  peer_addr;
      unique_ptr<boost::asio::steady_timer> response_expected;
      optional<request_message> pending_fetch;
      go_away_reason         no_retry;
      block_id_type          fork_head;
      uint32_t               fork_head_num;

      connection_status get_status()const {
         connection_status stat;
         stat.peer = peer_addr;
         stat.connecting = connecting;
         stat.syncing = syncing;
         stat.last_handshake = last_handshake_recv;
         return stat;
      }

      /** \name Peer Timestamps
       *  Time message handling
       *  @{
       */
      // Members set from network data
      tstamp                         org{0};          //!< originate timestamp
      tstamp                         rec{0};          //!< receive timestamp
      tstamp                         dst{0};          //!< destination timestamp
      tstamp                         xmt{0};          //!< transmit timestamp

      // Computed data
      double                         offset{0};       //!< peer offset

      static const size_t            ts_buffer_size{32};
      char                           ts[ts_buffer_size];          //!< working buffer for making human readable timestamps
      /** @} */

      bool connected();
      bool current();
      void reset();
      void close();
      void send_handshake();

      /** \name Peer Timestamps
       *  Time message handling
       */
      /** @{ */
      /** \brief Convert an std::chrono nanosecond rep to a human readable string
       */
      char* convert_tstamp(const tstamp& t);
      /**  \brief Populate and queue time_message
       */
      void send_time();
      /** \brief Populate and queue time_message immediately using incoming time_message
       */
      void send_time(const time_message& msg);
      /** \brief Read system time and convert to a 64 bit integer.
       *
       * There are only two calls on this routine in the program.  One
       * when a packet arrives from the network and the other when a
       * packet is placed on the send queue.  Calls the kernel time of
       * day routine and converts to a (at least) 64 bit integer.
       */
      tstamp get_time()
      {
         return std::chrono::system_clock::now().time_since_epoch().count();
      }
      /** @} */

      const string peer_name();

      void txn_send_pending(const vector<transaction_id_type> &ids);
      void txn_send(const vector<transaction_id_type> &txn_lis);

      void blk_send_branch();
      void blk_send(const vector<block_id_type> &txn_lis);
      void stop_send();

      void enqueue( transaction_id_type id );
      void enqueue( const net_message &msg, bool trigger_send = true );
      void cancel_sync(go_away_reason);
      void cancel_fetch();
      void flush_queues();
      bool enqueue_sync_block();
      void request_sync_blocks (uint32_t start, uint32_t end);

      void cancel_wait();
      void sync_wait();
      void fetch_wait();
      void sync_timeout(boost::system::error_code ec);
      void fetch_timeout(boost::system::error_code ec);

      void queue_write(std::shared_ptr<vector<char>> buff,
                       bool trigger_send,
                       std::function<void(boost::system::error_code, std::size_t)> cb);
      void do_queue_write();

      /** \brief Process the next message from the pending message buffer
       *
       * Process the next message from the pending_message_buffer.
       * message_length is the already determined length of the data
       * part of the message and impl in the net plugin implementation
       * that will handle the message.
       * Returns true is successful. Returns false if an error was
       * encountered unpacking or processing the message.
       */
      bool process_next_message(net_plugin_impl& impl, uint32_t message_length);
      // static const fc::string logger_name;
      // static fc::logger logger;
   };

   struct msgHandler : public fc::visitor<void> {
      net_plugin_impl &impl;
      connection_ptr c;
      msgHandler( net_plugin_impl &imp, connection_ptr conn) : impl(imp), c(conn) {}

      template <typename T>
      void operator()(const T &msg) const
      {
         impl.handle_message( c, msg);
      }
   };

   class sync_manager {
   private:
      enum stages {
         lib_catchup,
         head_catchup,
         in_sync
      };

      uint32_t       sync_known_lib_num;
      uint32_t       sync_last_requested_num;
      uint32_t       sync_next_expected_num;
      uint32_t       sync_req_span;
      uint32_t       last_repeated;
      connection_ptr source;
      stages         state;

      deque<block_id_type> _blocks;
      chain_plugin * chain_plug;

   public:
      sync_manager(uint32_t span);
      void set_state(stages s);
      bool is_active(connection_ptr conn);
      void reset_lib_num(connection_ptr conn);
      bool sync_required();
      void request_next_chunk(connection_ptr conn = connection_ptr() );
      void start_sync(connection_ptr c, uint32_t target);
      void send_handshakes();
      void reassign_fetch(connection_ptr c, go_away_reason reason);
      void verify_catchup(connection_ptr c, uint32_t num, block_id_type id);
      void recv_block(connection_ptr c, const block_id_type &blk_id, uint32_t blk_num, bool accepted);
      void recv_handshake(connection_ptr c, const handshake_message& msg);
      void recv_notice(connection_ptr c, const notice_message& msg);

      // static const fc::string logger_name;
      // static fc::logger logger;
   };

   class big_msg_manager {
   public:
      uint32_t just_send_it_max;
      connection_ptr pending_txn_source;
      vector<block_id_type> req_blks;
      vector<transaction_id_type> req_txn;

      void bcast_block (const signed_block_summary& msg, connection_ptr skip = connection_ptr());
      void bcast_transaction (const transaction_id_type& id,
                              time_point_sec expiration,
                              const packed_transaction& msg);
      void bcast_rejected_transaction (const packed_transaction& msg, uint64_t code);
      void recv_block (connection_ptr conn, const signed_block_summary& msg);
      void recv_transaction(connection_ptr c);
      void recv_notice (connection_ptr conn, const notice_message& msg);

      void retry_fetch (connection_ptr conn);
      // static const fc::string logger_name;
      // static fc::logger logger;
   };

   // const fc::string net_plugin_impl::logger_name("net_plugin_impl");
   // fc::logger net_plugin_impl::logger(net_plugin_impl::logger_name);
   // const fc::string connection::logger_name("connection");
   // fc::logger connection::logger(connection::logger_name);
   // const fc::string sync_manager::logger_name("sync_manager");
   // fc::logger sync_manager::logger(sync_manager::logger_name);
   // const fc::string big_msg_manager::logger_name("big_msg_manager");
   // fc::logger big_msg_manager::logger(big_msg_manager::logger_name);

   //---------------------------------------------------------------------------

   connection::connection( string endpoint )
      : blk_state(),
        trx_state(),
        peer_requested(),
        socket( std::make_shared<tcp::socket>( std::ref( app().get_io_service() ))),
        node_id(),
        last_handshake_recv(),
        last_handshake_sent(),
        sent_handshake_count(0),
        connecting(false),
        syncing(false),
        write_depth(0),
        peer_addr(endpoint),
        response_expected(),
        pending_fetch(),
        no_retry(no_reason),
        fork_head(),
        fork_head_num()
   {
      wlog( "created connection to ${n}", ("n", endpoint) );
      initialize();
   }

   connection::connection( socket_ptr s )
      : blk_state(),
        trx_state(),
        peer_requested(),
        socket( s ),
        node_id(),
        last_handshake_recv(),
        last_handshake_sent(),
        sent_handshake_count(0),
        connecting(true),
        syncing(false),
        write_depth(0),
        peer_addr(),
        response_expected(),
        pending_fetch(),
        no_retry(no_reason),
        fork_head(),
        fork_head_num(0)
   {
      wlog( "accepted network connection" );
      initialize();
   }

   connection::~connection() {
      if(peer_addr.empty())
         wlog( "released connection from client" );
      else
         wlog( "released connection to server at ${addr}", ("addr", peer_addr) );
   }

   void connection::initialize() {
      auto *rnd = node_id.data();
      rnd[0] = 0;
      response_expected.reset(new boost::asio::steady_timer(app().get_io_service()));
   }

   bool connection::connected() {
      return (socket->is_open() && !connecting);
   }

   bool connection::current() {
      return (connected() && !syncing);
   }

   void connection::reset() {
      peer_requested.reset();
      blk_state.clear();
      trx_state.clear();
   }

   void connection::flush_queues() {
      if (write_depth > 0) {
         while (write_queue.size() > 1) {
            write_queue.pop_back();
         }
      } else {
         write_queue.clear();
      }
   }

   void connection::close() {
      if(socket) {
         socket->close();
      }
      else {
         wlog("no socket to close!");
      }
      flush_queues();
      connecting = false;
      syncing = false;
      reset();
      sent_handshake_count = 0;
      last_handshake_recv = handshake_message();
      last_handshake_sent = handshake_message();
      my_impl->sync_master->reset_lib_num(shared_from_this());
      fc_dlog(logger, "canceling wait on ${p}", ("p",peer_name()));
      cancel_wait();
      pending_message_buffer.reset();
   }

   void connection::txn_send_pending(const vector<transaction_id_type> &ids) {
      for(auto tx = my_impl->local_txns.begin(); tx != my_impl->local_txns.end(); ++tx ){
         if(tx->serialized_txn.size() && tx->block_num == 0) {
            bool found = false;
            for(auto known : ids) {
               if( known == tx->id) {
                  found = true;
                  break;
               }
            }
            if(!found) {
               my_impl->local_txns.modify(tx,incr_in_flight);
               queue_write(std::make_shared<vector<char>>(tx->serialized_txn),
                           true,
                           [this, tx](boost::system::error_code ec, std::size_t ) {
                              my_impl->local_txns.modify(tx, decr_in_flight);
                           });
            }
         }
      }
   }

   void connection::txn_send(const vector<transaction_id_type> &ids) {
      for(auto t : ids) {
         auto tx = my_impl->local_txns.get<by_id>().find(t);
         if( tx != my_impl->local_txns.end() && tx->serialized_txn.size()) {
            my_impl->local_txns.modify( tx,incr_in_flight);
            queue_write(std::make_shared<vector<char>>(tx->serialized_txn),
                        true,
                        [this, tx](boost::system::error_code ec, std::size_t ) {
                           my_impl->local_txns.modify(tx, decr_in_flight);
                        });
         }
      }
   }

   void connection::blk_send_branch() {
      chain_controller &cc = my_impl->chain_plug->chain();
      uint32_t head_num = cc.head_block_num ();
      notice_message note;
      note.known_blocks.mode = normal;
      note.known_blocks.pending = 0;
      fc_dlog(logger, "head_num = ${h}",("h",head_num));
      if(head_num == 0) {
         enqueue(note);
         return;
      }
      block_id_type head_id;
      block_id_type lib_id;
      uint32_t lib_num;
      try {
         lib_num = cc.last_irreversible_block_num();
         if( lib_num != 0 )
            lib_id = cc.get_block_id_for_num(lib_num);
         head_id = cc.get_block_id_for_num(head_num);
      }
      catch (const assert_exception &ex) {
         elog( "unable to retrieve block info: ${n} for ${p}",("n",ex.to_string())("p",peer_name()));
         enqueue(note);
         return;
      }
      catch (const fc::exception &ex) {
      }
      catch (...) {
      }

      vector<optional<signed_block> > bstack;
      block_id_type null_id;
      for (auto bid = head_id; bid != null_id && bid != lib_id; ) {
         try {
            optional<signed_block> b = cc.fetch_block_by_id(bid);
            if ( b ) {
               bid = b->previous;
               bstack.push_back(b);
            }
            else {
               break;
            }
         } catch (...) {
            break;
         }
      }
      size_t count = 0;
      if (bstack.back()->previous == lib_id) {
         count = bstack.size();
         while (bstack.size()) {
            enqueue (*bstack.back());
            bstack.pop_back();
         }
      }

      fc_ilog(logger, "Sent ${n} blocks on my fork",("n",count));
      syncing = false;
   }

   void connection::blk_send(const vector<block_id_type> &ids) {
      chain_controller &cc = my_impl->chain_plug->chain();
      int count = 0;
      for(auto &blkid : ids) {
         ++count;
         try {
            optional<signed_block> b = cc.fetch_block_by_id(blkid);
            if(b) {
               fc_dlog(logger,"found block for id at num ${n}",("n",b->block_num()));
               enqueue(*b);
            }
            else {
               ilog("fetch block by id returned null, id ${id} on block ${c} of ${s} for ${p}",
                     ("id",blkid)("c",count)("s",ids.size())("p",peer_name()));
               break;
            }
         }
         catch (const assert_exception &ex) {
            elog( "caught assert on fetch_block_by_id, ${ex}, id ${id} on block ${c} of ${s} for ${p}",
                  ("ex",ex.to_string())("id",blkid)("c",count)("s",ids.size())("p",peer_name()));
            break;
         }
         catch (...) {
            elog( "caught othser exception fetching block id ${id} on block ${c} of ${s} for ${p}",
                  ("id",blkid)("c",count)("s",ids.size())("p",peer_name()));
            break;
         }
      }

   }

   void connection::stop_send() {
      syncing = false;
   }

   void connection::send_handshake( ) {
      handshake_initializer::populate(last_handshake_sent);
      last_handshake_sent.generation = ++sent_handshake_count;
      fc_dlog(logger, "Sending handshake generation ${g} to ${ep}",
              ("g",last_handshake_sent.generation)("ep", peer_addr));
      enqueue(last_handshake_sent);
   }

   char* connection::convert_tstamp(const tstamp& t)
   {
      const long long NsecPerSec{1000000000};
      time_t seconds = t / NsecPerSec;
      strftime(ts, ts_buffer_size, "%F %T", localtime(&seconds));
      snprintf(ts+19, ts_buffer_size-19, ".%lld", t % NsecPerSec);
      return ts;
   }

   void connection::send_time() {
      time_message xpkt;
      xpkt.org = rec;
      xpkt.rec = dst;
      xpkt.xmt = get_time();
      org = xpkt.xmt;
      enqueue(xpkt);
   }

   void connection::send_time(const time_message& msg) {
      time_message xpkt;
      xpkt.org = msg.xmt;
      xpkt.rec = msg.dst;
      xpkt.xmt = get_time();
      enqueue(xpkt);
   }

   void connection::queue_write(std::shared_ptr<vector<char>> buff,
                                bool trigger_send,
                                std::function<void(boost::system::error_code, std::size_t)> cb) {
      write_queue.push_back({buff, cb});
      if(write_queue.size() == 1 && trigger_send)
         do_queue_write();
   }

   void connection::do_queue_write() {
      if(write_queue.empty())
         return;
      write_depth++;
      connection_wptr c(shared_from_this());
      boost::asio::async_write(*socket, boost::asio::buffer(*write_queue.front().buff), [c](boost::system::error_code ec, std::size_t w) {
            try {
               auto conn = c.lock();
               if(!conn)
                  return;

               if (conn->write_queue.size() ) {
                  conn->write_queue.front().cb(ec, w);
               }
               conn->write_depth--;

               if(ec) {
                  string pname = conn ? conn->peer_name() : "no connection name";
                  if( ec.value() != boost::asio::error::eof) {
                     elog("Error sending to peer ${p}: ${i}", ("p",pname)("i", ec.message()));
                  }
                  else {
                     ilog("connection closure detected on write to ${p}",("p",pname));
                  }
                  my_impl->close(conn);
                  return;
               }
               conn->write_queue.pop_front();
               conn->enqueue_sync_block();
               conn->do_queue_write();
            }
            catch(const std::exception &ex) {
               auto conn = c.lock();
               string pname = conn ? conn->peer_name() : "no connection name";
               elog("Exception in do_queue_write to ${p} ${s}", ("p",pname)("s",ex.what()));
            }
            catch(const fc::exception &ex) {
               auto conn = c.lock();
               string pname = conn ? conn->peer_name() : "no connection name";
               elog("Exception in do_queue_write to ${p} ${s}", ("p",pname)("s",ex.to_string()));
            }
            catch(...) {
               auto conn = c.lock();
               string pname = conn ? conn->peer_name() : "no connection name";
               elog("Exception in do_queue_write to ${p}", ("p",pname) );
            }
         });
   }

   void connection::cancel_sync(go_away_reason reason) {
      fc_dlog(logger,"cancel sync reason = ${m}, write queue size ${o} peer ${p}",
              ("m",reason_str(reason)) ("o", write_queue.size())("p", peer_name()));
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
         fc_dlog(logger, "sending empty request but not calling sync wait on ${p}", ("p",peer_name()));
         enqueue( ( sync_request_message ) {0,0} );
      }
   }

   void connection::cancel_fetch() {
      my_impl->big_msg_master->retry_fetch(shared_from_this() );
   }

   bool connection::enqueue_sync_block() {
      chain_controller& cc = app().find_plugin<chain_plugin>()->chain();
      if (!peer_requested)
         return false;
      uint32_t num = ++peer_requested->last;
      bool trigger_send = num == peer_requested->start_block;
      if(num == peer_requested->end_block) {
         peer_requested.reset();
      }
      try {
         fc::optional<signed_block> sb = cc.fetch_block_by_number(num);
         if(sb) {
            enqueue( *sb, trigger_send);
            return true;
         }
      } catch ( ... ) {
         wlog( "write loop exception" );
      }
      return false;
   }

   void connection::enqueue( const net_message &m, bool trigger_send ) {
      go_away_reason close_after_send = no_reason;
      if (m.contains<go_away_message>()) {
         close_after_send = m.get<go_away_message>().reason;
      }

      uint32_t payload_size = fc::raw::pack_size( m );
      char * header = reinterpret_cast<char*>(&payload_size);
      size_t header_size = sizeof(payload_size);

      size_t buffer_size = header_size + payload_size;

      auto send_buffer = std::make_shared<vector<char>>(buffer_size);
      fc::datastream<char*> ds( send_buffer->data(), buffer_size);
      ds.write( header, header_size );
      fc::raw::pack( ds, m );
      write_depth++;
      queue_write(send_buffer,trigger_send,
                  [this, close_after_send](boost::system::error_code ec, std::size_t ) {
                     write_depth--;
                     if(close_after_send != no_reason) {
                        elog ("sent a go away message: ${r}, closing connection to ${p}",("r",reason_str(close_after_send))("p",peer_name()));
                        my_impl->close(shared_from_this());
                        return;
                     }
                  });
   }

   void connection::cancel_wait() {
      if (response_expected)
         response_expected->cancel();
   }

   void connection::sync_wait( ) {
      response_expected->expires_from_now( my_impl->resp_expected_period);
      connection_wptr c(shared_from_this());
      response_expected->async_wait( [c]( boost::system::error_code ec){
            connection_ptr conn = c.lock();
            if (!conn) {
               // connection was destroyed before this lambda was delivered
               return;
            }

            conn->sync_timeout(ec);
         } );
   }

   void connection::fetch_wait( ) {
      response_expected->expires_from_now( my_impl->resp_expected_period);
      connection_wptr c(shared_from_this());
      response_expected->async_wait( [c]( boost::system::error_code ec){
            connection_ptr conn = c.lock();
            if (!conn) {
               // connection was destroyed before this lambda was delivered
               return;
            }

            conn->fetch_timeout(ec);
         } );
   }

   void connection::sync_timeout( boost::system::error_code ec ) {
      if( !ec ) {
         my_impl->sync_master->reassign_fetch (shared_from_this(),benign_other);
      }
      else if( ec == boost::asio::error::operation_aborted) {
      }
      else {
         elog ("setting timer for sync request got error ${ec}",("ec", ec.message()));
      }
   }

   const string connection::peer_name() {
      if( !last_handshake_recv.p2p_address.empty() ) {
         return last_handshake_recv.p2p_address;
      }
      if( !peer_addr.empty() ) {
         return peer_addr;
      }
      return "connecting client";
   }

   void connection::fetch_timeout( boost::system::error_code ec ) {
      if( !ec ) {
         if( pending_fetch.valid() && !( pending_fetch->req_trx.empty( ) || pending_fetch->req_blocks.empty( ) ) ) {
            my_impl->big_msg_master->retry_fetch (shared_from_this() );
         }
      }
      else if( ec == boost::asio::error::operation_aborted ) {
         if( !connected( ) ) {
            fc_dlog(logger, "fetch timeout was cancelled due to dead connection");
         }
      }
      else {
         elog( "setting timer for fetch request got error ${ec}", ("ec", ec.message( ) ) );
      }
   }

   void connection::request_sync_blocks (uint32_t start, uint32_t end) {
      sync_request_message srm = {start,end};
      enqueue( net_message(srm));
      sync_wait();
   }

   bool connection::process_next_message(net_plugin_impl& impl, uint32_t message_length) {
      try {
         // If it is a signed_block, then save the raw message for the cache
         // This must be done before we unpack the message.
         // This code is copied from fc::io::unpack(..., unsigned_int)
         auto index = pending_message_buffer.read_index();
         uint64_t which = 0; char b = 0; uint8_t by = 0;
         do {
            pending_message_buffer.peek(&b, 1, index);
            which |= uint32_t(uint8_t(b) & 0x7f) << by;
            by += 7;
         } while( uint8_t(b) & 0x80 );

         if (which == uint64_t(net_message::tag<signed_block>::value)) {
            blk_buffer.resize(message_length);
            auto index = pending_message_buffer.read_index();
            pending_message_buffer.peek(blk_buffer.data(), message_length, index);
         }
         auto ds = pending_message_buffer.create_datastream();
         net_message msg;
         fc::raw::unpack(ds, msg);
         msgHandler m(impl, shared_from_this() );
         msg.visit(m);
      } catch(  const fc::exception& e ) {
         edump((e.to_detail_string() ));
         impl.close( shared_from_this() );
         return false;
      }
      return true;
   }


   //-----------------------------------------------------------

   sync_manager::sync_manager( uint32_t req_span )
      :sync_known_lib_num( 0 )
      ,sync_last_requested_num( 0 )
      ,sync_next_expected_num( 1 )
      ,sync_req_span( req_span )
      ,last_repeated( 0 )
      ,source()
      ,state(in_sync)
   {
      chain_plug = app( ).find_plugin<chain_plugin>( );
   }

   void sync_manager::set_state(stages newstate) {
      if (state == newstate) {
         return;
      }
      string os = state == in_sync ? "in sync" : state == lib_catchup ? "lib catchup" : "head catchup";
      state = newstate;
      string ns = state == in_sync ? "in sync" : state == lib_catchup ? "lib catchup" : "head catchup";
      fc_dlog(logger, "old state ${os} becoming ${ns}",("os",os)("ns",ns));
   }

   bool sync_manager::is_active(connection_ptr c) {
      if (state == head_catchup && c) {
         bool fhset = c->fork_head != block_id_type();
         fc_dlog(logger, "fork_head_num = ${fn} fork_head set = ${s}",
                 ("fn", c->fork_head_num)("s", fhset));
            return c->fork_head != block_id_type() && c->fork_head_num < chain_plug->chain().head_block_num();
      }
      return state != in_sync;
   }

   void sync_manager::reset_lib_num(connection_ptr c) {
      if(state == in_sync) {
         source.reset();
      }
      if( c->current() ) {
         if( c->last_handshake_recv.last_irreversible_block_num > sync_known_lib_num) {
            sync_known_lib_num =c->last_handshake_recv.last_irreversible_block_num;
         }
      } else if( c == source ) {
         sync_last_requested_num = 0;
         request_next_chunk();
      }
   }

   bool sync_manager::sync_required( ) {
      fc_dlog(logger, "last req = ${req}, last recv = ${recv} known = ${known} our head = ${head}",
              ("req",sync_last_requested_num)("recv",sync_next_expected_num)("known",sync_known_lib_num)("head",chain_plug->chain( ).head_block_num( )));

      return( sync_last_requested_num < sync_known_lib_num ||
              chain_plug->chain( ).head_block_num( ) < sync_last_requested_num );
   }

   void sync_manager::request_next_chunk( connection_ptr conn ) {
      uint32_t head_block = chain_plug->chain().head_block_num();

      if (head_block < sync_last_requested_num) {
         fc_ilog (logger, "ignoring request, head is ${h} last req = ${r}",
                  ("h",head_block)("r",sync_last_requested_num));
         return;
      }

      /* ----------
       * next chunk provider selection criteria
       * 1. a provider is supplied, use it.
       * 2. we only have 1 peer so use that.
       * 3. we have multiple peers, select the next available from the list
       */

      if (conn) {
         source = conn;
      }
      else if (my_impl->connections.size() == 1) {
         if (!source) {
            source = *my_impl->connections.begin();
         }
         if (!source->current()) {
            source.reset();
         }
      }
      else {
         auto cptr = my_impl->connections.find(source);
         if (cptr == my_impl->connections.end()) {
            elog ("unable to find previous source connection in connections list");
            source.reset();
            cptr = my_impl->connections.begin();
         } else {
            ++cptr;
         }
         while (my_impl->connections.size() > 1) {
            if (cptr == my_impl->connections.end()) {
               cptr = my_impl->connections.begin();
               if (!source) {
                  break;
               }
            }
            if (*cptr == source) {
               break;
            }
            if ((*cptr)->current()) {
               source = *cptr;
               break;
            }
            else {
               ++cptr;
            }
         }
      }

      if (!source) {
         elog("Unable to continue syncing at this time");
         sync_known_lib_num = chain_plug->chain().last_irreversible_block_num();
         sync_last_requested_num = 0;
         return;
      }

      if( sync_last_requested_num != sync_known_lib_num ) {
         uint32_t start = sync_next_expected_num;
         uint32_t end = start + sync_req_span - 1;
         if( end > sync_known_lib_num )
            end = sync_known_lib_num;
         if( end > 0 && end >= start ) {
            fc_dlog(logger, "conn ${n} requesting range ${s} to ${e}, calling sync_wait",
                    ("n",source->peer_name())("s",start)("e",end));
            source->request_sync_blocks(start, end);
            sync_last_requested_num = end;
         }
      }
   }

   void sync_manager::send_handshakes ()
   {
      for( auto &ci : my_impl->connections) {
         if( ci->current()) {
            ci->send_handshake();
         }
      }
   }

   void sync_manager::start_sync( connection_ptr c, uint32_t target) {
      if( target > sync_known_lib_num) {
         sync_known_lib_num = target;
      }

      if (!sync_required()) {
         uint32_t bnum = chain_plug->chain().last_irreversible_block_num();
         uint32_t hnum = chain_plug->chain().head_block_num();
         fc_dlog( logger, "We are already caught up, my irr = ${b}, head = ${h}, target = ${t}",
                  ("b",bnum)("h",hnum)("t",target));
         return;
      }

      if (state == in_sync) {
         set_state(lib_catchup);
         sync_next_expected_num = chain_plug->chain().last_irreversible_block_num() + 1;
      }

      fc_ilog(logger, "Catching up with chain, our last req is ${cc}, theirs is ${t} peer ${p}",
              ( "cc",sync_last_requested_num)("t",target)("p",c->peer_name()));

      request_next_chunk(c);
   }

   void sync_manager::reassign_fetch(connection_ptr c, go_away_reason reason) {
      fc_ilog(logger, "reassign_fetch, our last req is ${cc}, next expected is ${ne} peer ${p}",
              ( "cc",sync_last_requested_num)("ne",sync_next_expected_num)("p",c->peer_name()));

      if (c == source) {
         c->cancel_sync (reason);
         sync_last_requested_num = 0;
         request_next_chunk();
      }
   }

   void sync_manager::recv_handshake (connection_ptr c, const handshake_message &msg) {
      chain_controller& cc = chain_plug->chain();
      uint32_t lib_num = cc.last_irreversible_block_num( );
      uint32_t peer_lib = msg.last_irreversible_block_num;
      reset_lib_num(c);
      c->syncing = false;

      //--------------------------------
      // sync need checks; (lib == last irreversible block)
      //
      // 0. my head block id == peer head id means we are all caugnt up block wise
      // 1. my head block num < peer lib - start sync locally
      // 2. my lib > peer head num - send an last_irr_catch_up notice if not the first generation
      //
      // 3  my head block num <= peer head block num - update sync state and send a catchup request
      // 4  my head block num > peer block num ssend a notice catchup if this is not the first generation
      //
      //-----------------------------

      uint32_t head = cc.head_block_num( );
      block_id_type head_id = cc.head_block_id();
      if (head_id == msg.head_id) {
         fc_dlog(logger, "sync check state 0");
         // notify peer of our pending transactions
         notice_message note;
         note.known_blocks.mode = none;
         note.known_trx.mode = catch_up;
         note.known_trx.pending = my_impl->local_txns.size();
         c->enqueue( note );
         return;
      }
      if (head < peer_lib) {
         fc_dlog(logger, "sync check state 1");
         start_sync( c, peer_lib);
         return;
      }
      if (lib_num > msg.head_num ) {
         fc_dlog(logger, "sync check state 2");
         // for generation 1, we wlso sent a handshake so they will treat this as state 1
         if ( msg.generation > 1 ) {
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

      if (head <= msg.head_num ) {
         fc_dlog(logger, "sync check state 3");
         //         if (state == in_sync ) {
            verify_catchup (c, msg.head_num, msg.head_id);
            //         }
         return;
      }
      else {
         fc_dlog(logger, "sync check state 4");
         if ( msg.generation > 1 ) {
            notice_message note;
            note.known_trx.mode = none;
            note.known_blocks.mode = catch_up;
            note.known_blocks.pending = head;
            note.known_blocks.ids.push_back(head_id);
            c->enqueue( note );
         }
         c->syncing = true;
         return;
      }
      elog ("sync check failed to resolve status");
   }

   void sync_manager::verify_catchup(connection_ptr c, uint32_t num, block_id_type id) {
      request_message req;
      req.req_blocks.mode = catch_up;
      for (auto cc : my_impl->connections) {
         if (cc->fork_head == id ||
             cc->fork_head_num > num)
            req.req_blocks.mode = none;
         break;
      }
      if( req.req_blocks.mode == catch_up ) {
         c->fork_head = id;
         c->fork_head_num = num;
         set_state(head_catchup);
      }
      else {
         c->fork_head = block_id_type();
         c->fork_head_num = 0;
      }
      req.req_trx.mode = none;
      c->enqueue( req );
   }

   void sync_manager::recv_notice (connection_ptr c, const notice_message &msg) {
      fc_ilog (logger, "sync_manager got ${m} block notice",("m",modes_str(msg.known_blocks.mode)));
      if (msg.known_blocks.mode == catch_up) {
         if (msg.known_blocks.ids.size() == 0) {
            elog ("got a catch up with ids size = 0");
         }
         else {
            verify_catchup(c,  msg.known_blocks.pending, msg.known_blocks.ids.back());
         }
      }
      else {
         c->last_handshake_recv.last_irreversible_block_num = msg.known_trx.pending;
         reset_lib_num (c);
         start_sync(c, msg.known_blocks.pending);
      }
   }

   void sync_manager::recv_block (connection_ptr c, const block_id_type &blk_id, uint32_t blk_num, bool accepted) {
      if (!accepted) {
         uint32_t head_num = chain_plug->chain().head_block_num();
         if (head_num != last_repeated) {
            ilog ("block not accepted, try requesting one more time");
            last_repeated = head_num;
            send_handshakes();
         }
         else {
            ilog ("second attempt to retrive block ${n} failed",
                  ("n", head_num + 1));
            last_repeated = 0;
            sync_last_requested_num = 0;
            c->close();
         }
         return;
      }
      last_repeated = 0;
      if (state == lib_catchup) {
         if (blk_num != sync_next_expected_num) {
            fc_ilog (logger, "expected block ${ne} but got ${bn}",("ne",sync_next_expected_num)("bn",blk_num));
            c->close();
         }
         sync_next_expected_num = blk_num + 1;
      }
      if (state == head_catchup) {
         fc_dlog (logger, "sync_manager in head_catchup state");
         set_state(in_sync);
         source.reset();

         block_id_type null_id;
         for (auto cp : my_impl->connections) {
            if (cp->fork_head == null_id) {
               continue;
            }
            if (cp->fork_head == blk_id || cp->fork_head_num < blk_num) {
               c->fork_head = null_id;
               c->fork_head_num = 0;
            }
            else {
               set_state(head_catchup);
            }
         }
      }
      else if (state == lib_catchup) {
         if( blk_num == sync_known_lib_num ) {
            fc_dlog( logger, "All caught up with last known last irreversible block resending handshake");
            set_state(in_sync);
            send_handshakes();
         }
         else if (blk_num == sync_last_requested_num) {
            request_next_chunk();
         }
         else {
            fc_dlog(logger,"calling sync_wait on connection ${p}",("p",c->peer_name()));
            c->sync_wait();
         }
      }
   }

   //------------------------------------------------------------------------

   void big_msg_manager::bcast_block (const signed_block_summary &bsum, connection_ptr skip) {
      net_message msg(bsum);
      uint32_t packsiz = fc::raw::pack_size(msg);
      uint32_t msgsiz = packsiz + sizeof(packsiz);
      notice_message pending_notify;
      block_id_type bid = bsum.id();
      pending_notify.known_blocks.mode = normal;
      pending_notify.known_blocks.ids.push_back( bid );
      pending_notify.known_trx.mode = none;
      if (msgsiz > just_send_it_max) {
         fc_ilog(logger, "block size is ${ms}, sending notify",("ms", msgsiz));
         my_impl->send_all(pending_notify, [skip, bid](connection_ptr c) -> bool {
               if (c == skip || !c->current())
                  return false;
               const auto& bs = c->blk_state.find(bid);
               bool unknown = bs == c->blk_state.end();
               if (unknown) {
                  c->blk_state.insert((block_state){bid,false,true,time_point()});
               }
               else {
                  elog("${p} already has knowledge of block ${b}", ("p",c->peer_name())("b",bid));
               }
               return unknown;
            });
      }
      else {
         for (auto cp : my_impl->connections) {
            if (cp == skip || !cp->current()) {
               continue;
            }
            const auto& prev = cp->blk_state.find (bsum.previous);
            if (prev != cp->blk_state.end() && !prev->is_known) {
               cp->blk_state.insert((block_state){bid,false,true,time_point()});
               cp->enqueue( pending_notify );
            }
            else {
               cp->blk_state.insert((block_state){bid,true,true,time_point()});
               cp->enqueue( bsum );
            }
         }
      }
   }

   void big_msg_manager::bcast_rejected_transaction (const packed_transaction& txn, uint64_t code) {
      if (code == 0 || code == eosio::chain::tx_duplicate::code_value) {
         //do not forward duplicates or those with unknown exception types
         return;
      }
      transaction_id_type tid = txn.get_transaction().id();
      fc_wlog(logger,"sending rejected transaction ${tid}",("tid",tid));
      bcast_transaction (tid, time_point_sec(), txn);
   }

   void big_msg_manager::bcast_transaction (const transaction_id_type & txnid,
                                            time_point_sec expiration,
                                            const packed_transaction& txn) {
      connection_ptr skip = pending_txn_source;
      pending_txn_source.reset();

      for (auto ref = req_txn.begin(); ref != req_txn.end(); ++ref) {
         if (*ref == txnid) {
            req_txn.erase(ref);
            break;
         }
      }

      if( my_impl->local_txns.get<by_id>().find( txnid ) != my_impl->local_txns.end( ) ) { //found
         fc_dlog(logger, "found txnid in local_txns" );
         return;
      }
      bool remember = expiration != time_point_sec();
      uint32_t packsiz = 0;
      uint32_t bufsiz = 0;
      if (remember) {
         net_message msg(txn);
         packsiz = fc::raw::pack_size(msg);
         bufsiz = packsiz + sizeof(packsiz);
         vector<char> buff(bufsiz);
         fc::datastream<char*> ds( buff.data(), bufsiz);
         ds.write( reinterpret_cast<char*>(&packsiz), sizeof(packsiz) );
         fc::raw::pack( ds, msg );
         node_transaction_state nts = {txnid,
                                       expiration,
                                       txn,
                                       std::move(buff),
                                       0, 0, 0};
         my_impl->local_txns.insert(std::move(nts));
      }
      if( !skip && bufsiz <= just_send_it_max) {
         my_impl->send_all( txn, [remember,txnid](connection_ptr c) -> bool {
               if( c->syncing ) {
                  return false;
               }
               const auto& bs = c->trx_state.find(txnid);
               bool unknown = bs == c->trx_state.end();
               if( unknown) {
                  if (remember) {
                     c->trx_state.insert(transaction_state({txnid,true,true,0,time_point() }));
                  }
                  fc_dlog(logger, "sending whole txn to ${n}", ("n",c->peer_name() ) );
               }
               return unknown;
            });
      }
      else {
         notice_message pending_notify;
         pending_notify.known_trx.mode = normal;
         pending_notify.known_trx.ids.push_back( txnid );
         pending_notify.known_blocks.mode = none;
         my_impl->send_all(pending_notify, [skip, remember, txnid](connection_ptr c) -> bool {
               if (c == skip || c->syncing) {
                  return false;
               }
               const auto& bs = c->trx_state.find(txnid);
               bool unknown = bs == c->trx_state.end();
               if( unknown) {
                  fc_ilog(logger, "sending notice to ${n}", ("n",c->peer_name() ) );
                  if (remember) {
                     c->trx_state.insert(transaction_state({txnid,false,true,0, time_point() }));
                  }
               }
               return unknown;
            });
      }

   }

   void big_msg_manager::recv_block (connection_ptr c, const signed_block_summary& msg) {
      block_id_type blk_id = msg.id();
      uint32_t num = msg.block_num();
      for (auto ref = req_blks.begin(); ref != req_blks.end(); ++ref) {
         if (*ref == blk_id) {
            req_blks.erase(ref);
            fc_dlog(logger, "received a requested block");
            notice_message note;
            note.known_blocks.mode = normal;
            note.known_blocks.ids.push_back( blk_id );
            note.known_trx.mode = none;
            my_impl->send_all(note, [blk_id](connection_ptr conn) -> bool {
                  const auto& bs = conn->blk_state.find(blk_id);
                  bool unknown = bs == conn->blk_state.end();
                  if (unknown) {
                     conn->blk_state.insert(block_state({blk_id,false,true,fc::time_point() }));
                  }
                  return unknown;
               });
            return;
         }
      }

      if( !my_impl->sync_master->is_active(c) ) {
         fc_dlog(logger,"got a block to forward");
         net_message nmsg(msg);
         if(fc::raw::pack_size(nmsg) < just_send_it_max ) {
            fc_dlog(logger, "forwarding the signed block");
            my_impl->send_all( msg, [c, blk_id, num](connection_ptr conn) -> bool {
                  bool sendit = false;
                  if( c != conn && !conn->syncing ) {
                     auto b = conn->blk_state.get<by_id>().find(blk_id);
                     if(b == conn->blk_state.end()) {
                        conn->blk_state.insert( (block_state){blk_id,true,true,fc::time_point()});
                        sendit = true;
                     } else if (!b->is_known) {
                        conn->blk_state.modify(b,set_is_known);
                        sendit = true;
                     }
                  }
                  fc_dlog(logger, "${action} block ${num} to ${c}",
                          ("action", sendit ? "sending" : "skipping")
                          ("num",num)
                          ("c", conn->peer_name() ));
                  return sendit;
               });
         }
         else {
            notice_message note;
            note.known_blocks.mode = normal;
            note.known_blocks.ids.push_back( blk_id );
            note.known_trx.mode = none;
            my_impl->send_all(note, [blk_id](connection_ptr conn) -> bool {
                  const auto& bs = conn->blk_state.find(blk_id);
                  bool unknown = bs == conn->blk_state.end();
                  if (unknown) {
                     conn->blk_state.insert(block_state({blk_id,false,true,fc::time_point() }));
                  }
                  return unknown;
               });
         }
      }
      else {
         fc_dlog(logger, "not forwarding from active syncing connection ${p}",("p",c->peer_name()));
      }
   }

   void big_msg_manager::recv_transaction (connection_ptr c) {
      pending_txn_source = c;
      fc_dlog(logger, "canceling wait on ${p}", ("p",c->peer_name()));
      c->cancel_wait();
   }

   void big_msg_manager::recv_notice (connection_ptr c, const notice_message& msg) {
      request_message req;
      bool send_req = false;
      chain_controller &cc = my_impl->chain_plug->chain();
      if (msg.known_trx.mode == normal) {
         req.req_trx.mode = normal;
         req.req_trx.pending = 0;
         for( const auto& t : msg.known_trx.ids ) {
            const auto &tx = my_impl->local_txns.get<by_id>( ).find( t );

            if( tx == my_impl->local_txns.end( ) ) {
               fc_dlog(logger,"did not find ${id}",("id",t));

               c->trx_state.insert( (transaction_state){t,true,true,0,
                        time_point()} );

               req.req_trx.ids.push_back( t );
               req_txn.push_back( t );
            }
            else {
               fc_dlog(logger,"big msg manager found txn id in table, ${id}",("id", t));
            }
         }
         send_req = !req.req_trx.ids.empty();
         fc_dlog(logger,"big msg manager send_req ids list has ${ids} entries", ("ids", req.req_trx.ids.size()));
      }
      else if (msg.known_trx.mode != none) {
         elog ("passed a notice_message with something other than a normal on none known_trx");
         return;
      }
      if (msg.known_blocks.mode == normal) {
         req.req_blocks.mode = normal;
         for( const auto& blkid : msg.known_blocks.ids) {
            optional<signed_block> b;
            try {
               b = cc.fetch_block_by_id(blkid);
            } catch (const assert_exception &ex) {
               ilog( "caught assert on fetch_block_by_id, ${ex}",("ex",ex.what()));
               // keep going, client can ask another peer
            } catch (...) {
               elog( "failed to retrieve block for id");
            }
            if (!b) {
               c->blk_state.insert((block_state){blkid,true,true,time_point::now()});
               send_req = true;
               req.req_blocks.ids.push_back( blkid );
               req_blks.push_back( blkid );
            }
         }
      }
      else if (msg.known_blocks.mode != none) {
         elog ("passed a notice_message with something other than a normal on none known_blocks");
         return;
      }
      fc_dlog( logger, "send req = ${sr}", ("sr",send_req));
      if( send_req) {
         c->enqueue(req);
         c->fetch_wait();
      }

   }

   void big_msg_manager::retry_fetch( connection_ptr c ) {
   }

   //------------------------------------------------------------------------

   void net_plugin_impl::connect( connection_ptr c ) {
      if( c->no_retry != go_away_reason::no_reason) {
         fc_dlog( logger, "Skipping connect due to go_away reason ${r}",("r", reason_str( c->no_retry )));
         return;
      }

      auto colon = c->peer_addr.find(':');

      if (colon == std::string::npos || colon == 0) {
         elog ("Invalid peer address. must be \"host:port\": ${p}", ("p",c->peer_addr));
         return;
      }

      auto host = c->peer_addr.substr( 0, colon );
      auto port = c->peer_addr.substr( colon + 1);
      idump((host)(port));
      tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
      // Note: need to add support for IPv6 too

      resolver->async_resolve( query,
                               [c, this]( const boost::system::error_code& err,
                                          tcp::resolver::iterator endpoint_itr ){
                                  if( !err ) {
                                     connect( c, endpoint_itr );
                                  } else {
                                     elog( "Unable to resolve ${peer_addr}: ${error}",
                                           (  "peer_addr", c->peer_name() )("error", err.message() ) );
                                  }
                               });
   }

   void net_plugin_impl::connect( connection_ptr c, tcp::resolver::iterator endpoint_itr ) {
      if( c->no_retry != go_away_reason::no_reason) {
         string rsn = reason_str(c->no_retry);
         return;
      }
      auto current_endpoint = *endpoint_itr;
      ++endpoint_itr;
      c->connecting = true;
      c->socket->async_connect( current_endpoint, [c, endpoint_itr, this] ( const boost::system::error_code& err ) {
            if( !err ) {
               start_session( c );
               c->send_handshake ();
            } else {
               if( endpoint_itr != tcp::resolver::iterator() ) {
                  c->close();
                  connect( c, endpoint_itr );
               }
               else {
                  elog( "connection failed to ${peer}: ${error}",
                        ( "peer", c->peer_name())("error",err.message()));
                  c->connecting = false;
                  my_impl->close(c);
               }
            }
         } );
   }

   void net_plugin_impl::start_session( connection_ptr con ) {
      boost::asio::ip::tcp::no_delay nodelay( true );
      con->socket->set_option( nodelay );
      start_read_message( con );
      ++started_sessions;

      // for now, we can just use the application main loop.
      //     con->readloop_complete  = bf::async( [=](){ read_loop( con ); } );
      //     con->writeloop_complete = bf::async( [=](){ write_loop con ); } );
   }


   void net_plugin_impl::start_listen_loop( ) {
      auto socket = std::make_shared<tcp::socket>( std::ref( app().get_io_service() ) );
      acceptor->async_accept( *socket, [socket,this]( boost::system::error_code ec ) {
            if( !ec ) {
               uint32_t visitors = 0;
               for (auto &conn : connections) {
                  if(conn->current() && conn->peer_addr.empty()) {
                     visitors++;
                  }
               }
               if (num_clients != visitors) {
                  ilog ("checking max client, visitors = ${v} num clients ${n}",("v",visitors)("n",num_clients));
                  num_clients = visitors;
               }
               if( max_client_count == 0 || num_clients < max_client_count ) {
                  ++num_clients;
                  connection_ptr c = std::make_shared<connection>( socket );
                  connections.insert( c );
                  start_session( c );
               } else {
                  elog( "Error max_client_count ${m} exceeded",
                        ( "m", max_client_count) );
                  socket->close( );
               }
               start_listen_loop();
            } else {
               elog( "Error accepting connection: ${m}",( "m", ec.message() ) );
            }
         });
   }

   void net_plugin_impl::start_read_message( connection_ptr conn ) {

      try {
         if(!conn->socket) {
            return;
         }
         conn->socket->async_read_some
            (conn->pending_message_buffer.get_buffer_sequence_for_boost_async_read(),
             [this,conn]( boost::system::error_code ec, std::size_t bytes_transferred ) {
               try {
                  if( !ec ) {
                     if (bytes_transferred > conn->pending_message_buffer.bytes_to_write()) {
                        elog("async_read_some callback: bytes_transfered = ${bt}, buffer.bytes_to_write = ${btw}",
                             ("bt",bytes_transferred)("btw",conn->pending_message_buffer.bytes_to_write()));
                     }
                     FC_ASSERT(bytes_transferred <= conn->pending_message_buffer.bytes_to_write());
                     conn->pending_message_buffer.advance_write_ptr(bytes_transferred);
                     while (conn->pending_message_buffer.bytes_to_read() > 0) {
                        uint32_t bytes_in_buffer = conn->pending_message_buffer.bytes_to_read();

                        if (bytes_in_buffer < message_header_size) {
                           break;
                        } else {
                           uint32_t message_length;
                           auto index = conn->pending_message_buffer.read_index();
                           conn->pending_message_buffer.peek(&message_length, sizeof(message_length), index);
                           if(message_length > def_send_buffer_size*2) {
                              elog("incoming message length unexpected (${i})", ("i", message_length));
                              close(conn);
                              return;
                           }
                           if (bytes_in_buffer >= message_length + message_header_size) {
                              conn->pending_message_buffer.advance_read_ptr(message_header_size);
                              if (!conn->process_next_message(*this, message_length)) {
                                 return;
                              }
                           } else {
                              conn->pending_message_buffer.add_space(message_length + message_header_size - bytes_in_buffer);
                              break;
                           }
                        }
                     }
                     start_read_message(conn);
                  } else {
                     auto pname = conn->peer_name();
                     if (ec.value() != boost::asio::error::eof) {
                        elog( "Error reading message from ${p}: ${m}",("p",pname)( "m", ec.message() ) );
                     } else {
                        ilog( "Peer ${p} closed connection",("p",pname) );
                     }
                     close( conn );
                  }
               }
               catch(const std::exception &ex) {
                  string pname = conn ? conn->peer_name() : "no connection name";
                  elog("Exception in handling read data from ${p} ${s}",("p",pname)("s",ex.what()));
                  close( conn );
               }
               catch(const fc::exception &ex) {
                  string pname = conn ? conn->peer_name() : "no connection name";
                  elog("Exception in handling read data ${s}", ("p",pname)("s",ex.to_string()));
                  close( conn );
               }
               catch (...) {
                  string pname = conn ? conn->peer_name() : "no connection name";
                  elog( "Undefined exception hanlding the read data from connection ${p}",( "p",pname));
                  close( conn );
               }
            } );
      } catch (...) {
         string pname = conn ? conn->peer_name() : "no connection name";
         elog( "Undefined exception handling reading ${p}",("p",pname) );
         close( conn );
      }
   }

   size_t net_plugin_impl::count_open_sockets() const
   {
      size_t count = 0;
      for( auto &c : connections) {
         if(c->socket->is_open())
            ++count;
      }
      return count;
   }


   template<typename VerifierFunc>
   void net_plugin_impl::send_all( const net_message &msg, VerifierFunc verify) {
      for( auto &c : connections) {
         if( c->current() && verify( c)) {
            c->enqueue( msg );
         }
      }
   }

   bool net_plugin_impl::is_valid( const handshake_message &msg) {
      // Do some basic validation of an incoming handshake_message, so things
      // that really aren't handshake messages can be quickly discarded without
      // affecting state.
      bool valid = true;
      if (msg.last_irreversible_block_num > msg.head_num) {
         wlog("Handshake message validation: last irreversible block (${i}) is greater than head block (${h})",
              ("i", msg.last_irreversible_block_num)("h", msg.head_num));
         valid = false;
      }
      if (msg.p2p_address.empty()) {
         wlog("Handshake message validation: p2p_address is null string");
         valid = false;
      }
      if (msg.os.empty()) {
         wlog("Handshake message validation: os field is null string");
         valid = false;
      }
      if ((msg.sig != chain::signature_type() || msg.token != sha256()) && (msg.token != fc::sha256::hash(msg.time))) {
         wlog("Handshake message validation: token field invalid");
         valid = false;
      }
      return valid;
   }

   void net_plugin_impl::handle_message( connection_ptr c, const handshake_message &msg) {
      fc_dlog( logger, "got a handshake_message from ${p} ${h}", ("p",c->peer_addr)("h",msg.p2p_address));
      if (!is_valid(msg)) {
         elog( "Invalid handshake message received from ${p} ${h}", ("p",c->peer_addr)("h",msg.p2p_address));
         c->enqueue( go_away_message( fatal_other ));
         return;
      }
      chain_controller& cc = chain_plug->chain();
      uint32_t lib_num = cc.last_irreversible_block_num( );
      uint32_t peer_lib = msg.last_irreversible_block_num;
      if( c->connecting ) {
         c->connecting = false;
      }
      if (msg.generation == 1) {
         if( msg.node_id == node_id) {
            elog( "Self connection detected. Closing connection");
            c->enqueue( go_away_message( self ) );
            return;
         }

         if( c->peer_addr.empty() || c->last_handshake_recv.node_id == fc::sha256()) {
            fc_dlog(logger, "checking for duplicate" );
            for(const auto &check : connections) {
               if(check == c)
                  continue;
               if(check->connected() && check->peer_name() == msg.p2p_address) {
                  // It's possible that both peers could arrive here at relatively the same time, so
                  // we need to avoid the case where they would both tell a different connection to go away.
                  // Using the sum of the initial handshake times of the two connections, we will
                  // arbitrarily (but consistently between the two peers) keep one of them.
                  if (msg.time + c->last_handshake_sent.time <= check->last_handshake_sent.time + check->last_handshake_recv.time)
                     continue;

                  fc_dlog( logger, "sending go_away duplicate to ${ep}", ("ep",msg.p2p_address) );
                  go_away_message gam(duplicate);
                  gam.node_id = node_id;
                  c->enqueue(gam);
                  c->no_retry = duplicate;
                  return;
               }
            }
         }
         else {
            fc_dlog(logger, "skipping duplicate check, addr == ${pa}, id = ${ni}",("pa",c->peer_addr)("ni",c->last_handshake_recv.node_id));
         }

         if( msg.chain_id != chain_id) {
            elog( "Peer on a different chain. Closing connection");
            c->enqueue( go_away_message(go_away_reason::wrong_chain) );
            return;
         }
         if( msg.network_version != network_version) {
            if (network_version_match) {
               elog("Peer network version does not match expected ${nv} but got ${mnv}",
                    ("nv", network_version)("mnv", msg.network_version));
               c->enqueue(go_away_message(wrong_version));
               return;
            } else {
               wlog("Peer network version does not match expected ${nv} but got ${mnv}",
                    ("nv", network_version)("mnv", msg.network_version));
            }
         }

         if(  c->node_id != msg.node_id) {
            c->node_id = msg.node_id;
         }

         if(!authenticate_peer(msg)) {
            elog("Peer not authenticated.  Closing connection.");
            c->enqueue(go_away_message(authentication));
            return;
         }

         bool on_fork = false;
         fc_dlog(logger, "lib_num = ${ln} peer_lib = ${pl}",("ln",lib_num)("pl",peer_lib));

         if( peer_lib <= lib_num && peer_lib > 0) {
            try {
               block_id_type peer_lib_id =  cc.get_block_id_for_num( peer_lib);
               on_fork =( msg.last_irreversible_block_id != peer_lib_id);
            }
            catch( const unknown_block_exception &ex) {
               wlog( "peer last irreversible block ${pl} is unknown", ("pl", peer_lib));
               on_fork = true;
            }
            catch( ...) {
               wlog( "caught an exception getting block id for ${pl}",("pl",peer_lib));
               on_fork = true;
            }
            if( on_fork) {
               elog( "Peer chain is forked");
               c->enqueue( go_away_message( forked ));
               return;
            }
         }

         if (c->sent_handshake_count == 0) {
            c->send_handshake();
         }
      }

      c->last_handshake_recv = msg;
      sync_master->recv_handshake(c,msg);
   }

   void net_plugin_impl::handle_message( connection_ptr c, const go_away_message &msg ) {
      string rsn = reason_str( msg.reason );
      ilog( "received a go away message from ${p}, reason = ${r}",
            ("p", c->peer_name())("r",rsn));
      c->no_retry = msg.reason;
      if(msg.reason == duplicate ) {
         c->node_id = msg.node_id;
      }
      c->flush_queues();
      close (c);
   }

   void net_plugin_impl::handle_message(connection_ptr c, const time_message &msg) {
      /* We've already lost however many microseconds it took to dispatch
       * the message, but it can't be helped.
       */
      msg.dst = c->get_time();

      // If the transmit timestamp is zero, the peer is horribly broken.
      if(msg.xmt == 0)
         return;                 /* invalid timestamp */

      if(msg.xmt == c->xmt)
         return;                 /* duplicate packet */

      c->xmt = msg.xmt;
      c->rec = msg.rec;
      c->dst = msg.dst;

      if(msg.org == 0)
         {
            c->send_time(msg);
            return;  // We don't have enough data to perform the calculation yet.
         }

      c->offset = (double(c->rec - c->org) + double(msg.xmt - c->dst)) / 2;
      double NsecPerUsec{1000};

      if(logger.is_enabled(fc::log_level::all))
         logger.log(FC_LOG_MESSAGE(all, "Clock offset is ${o}ns (${us}us)", ("o", c->offset)("us", c->offset/NsecPerUsec)));
      c->org = 0;
      c->rec = 0;
   }

   void net_plugin_impl::handle_message( connection_ptr c, const notice_message &msg) {
      // peer tells us about one or more blocks or txns. When done syncing, forward on
      // notices of previously unknown blocks or txns,
      //
      fc_dlog(logger, "got a notice_message from ${p}", ("p",c->peer_name()));
      request_message req;
      bool send_req = false;
      switch (msg.known_trx.mode) {
      case none:
         break;
      case last_irr_catch_up: {
         c->last_handshake_recv.head_num = msg.known_trx.pending;
         req.req_trx.mode = none;
         break;
      }
      case catch_up : {
         if( msg.known_trx.pending > 0) {
            // plan to get all except what we already know about.
            req.req_trx.mode = catch_up;
            send_req = true;
            size_t known_sum = local_txns.size();
            if( known_sum ) {
               for( const auto& t : local_txns.get<by_id>( ) ) {
                  req.req_trx.ids.push_back( t.id );
               }
            }
         }
         break;
      }
      case normal: {
         big_msg_master->recv_notice (c, msg);
      }
      }

      fc_dlog(logger,"this is a ${m} notice with ${n} blocks", ("m",modes_str(msg.known_blocks.mode))("n",msg.known_blocks.pending));

      switch (msg.known_blocks.mode) {
      case none : {
         if (msg.known_trx.mode != normal) {
            return;
         }
         break;
      }
      case last_irr_catch_up:
      case catch_up: {
         sync_master->recv_notice(c,msg);
         break;
      }
      case normal : {
         big_msg_master->recv_notice (c, msg);
         break;
      }
      default: {
         fc_dlog(logger, "received a bogus known_blocks.mode ${m} from ${p}",("m",static_cast<uint32_t>(msg.known_blocks.mode))("p",c->peer_name()));
      }
      }
      fc_dlog(logger, "send req = ${sr}", ("sr",send_req));
      if( send_req) {
         c->enqueue(req);
      }
   }

   void net_plugin_impl::handle_message( connection_ptr c, const request_message &msg) {
      switch (msg.req_blocks.mode) {
      case catch_up :
         fc_dlog( logger,  "got a catch_up request_message from ${p}", ("p",c->peer_name()));
         c->blk_send_branch( );
         break;
      case normal :
         fc_dlog(logger, "got a normal block request_message from ${p}", ("p",c->peer_name()));
         c->blk_send(msg.req_blocks.ids);
         break;
      default:;
      }


      switch (msg.req_trx.mode) {
      case catch_up :
         c->txn_send_pending(msg.req_trx.ids);
         break;
      case normal :
         c->txn_send(msg.req_trx.ids);
         break;
      case none :
         if(msg.req_blocks.mode == none)
            c->stop_send();
         break;
      default:;
      }

   }

   void net_plugin_impl::handle_message( connection_ptr c, const sync_request_message &msg) {
      if( msg.end_block == 0) {
         c->peer_requested.reset();
         c->flush_queues();
      } else {
         c->peer_requested = sync_state( msg.start_block,msg.end_block,msg.start_block-1);
         c->enqueue_sync_block();
      }
   }

   void net_plugin_impl::handle_message( connection_ptr c, const packed_transaction &msg) {
      fc_dlog(logger, "got a packed transaction from ${p}, cancel wait", ("p",c->peer_name()));
      if( sync_master->is_active(c) ) {
         fc_dlog(logger, "got a txn during sync - dropping");
         return;
      }
      c->cancel_wait();
      big_msg_master->recv_transaction(c);
      uint64_t code = 0;
      try {
         chain_plug->accept_transaction( msg);
         fc_dlog(logger, "chain accepted transaction" );
         return;
      }
      catch( const fc::exception &ex) {
         code = ex.code();
         elog( "accept txn threw  ${m}",("m",ex.to_detail_string()));
      }
      catch( ...) {
         elog( " caught something attempting to accept transaction");
      }

      big_msg_master->bcast_rejected_transaction(msg, code);
   }

   void net_plugin_impl::handle_message( connection_ptr c, const signed_transaction &msg) {
      ilog("Got a signed transaction from ${p} cancel wait",("p",c->peer_name()));
      c->cancel_wait();
   }

   void net_plugin_impl::handle_message( connection_ptr c, const signed_block_summary &msg) {
      chain_controller &cc = chain_plug->chain();
      block_id_type blk_id = msg.id();
      uint32_t blk_num = msg.block_num();
      fc_dlog(logger, "canceling wait on ${p}", ("p",c->peer_name()));
      c->cancel_wait();

      try {
         if( cc.is_known_block(blk_id)) {
            sync_master->recv_block(c, blk_id, blk_num, true);
            return;
         }
      } catch( ...) {
         // should this even be caught?
         elog("Caught an unknown exception trying to recall blockID");
      }

      fc::microseconds age( fc::time_point::now() - msg.timestamp);
      fc_dlog(logger, "got signed_block_summary #${n} from ${p} block age in secs = ${age}",
              ("n",blk_num)("p",c->peer_name())("age",age.to_seconds()));

      signed_block sb(msg);
      update_block_num ubn(blk_num);
      for (const auto &region : sb.regions) {
         for (const auto &cycle_sum : region.cycles_summary) {
            for (const auto &shard : cycle_sum) {
               for (const auto &recpt : shard.transactions) {
                  auto ltx = local_txns.get<by_id>().find(recpt.id);
                  switch (recpt.status) {
                  case transaction_receipt::delayed:
                  case transaction_receipt::executed: {
                     if( ltx != local_txns.end()) {
                        sb.input_transactions.push_back(ltx->packed_txn);
                        local_txns.modify( ltx, ubn );
                     }
                     break;
                  }
                  case transaction_receipt::soft_fail:
                  case transaction_receipt::hard_fail: {
                     if( ltx != local_txns.end()) {
                        sb.input_transactions.push_back(ltx->packed_txn);
                        local_txns.modify( ltx, ubn );
                        auto ctx = c->trx_state.get<by_id>().find(recpt.id);
                        if( ctx != c->trx_state.end()) {
                           c->trx_state.modify( ctx, ubn );
                        }
                     }
                     break;
                  }
                  }
               }
            }
         }
      }

      go_away_reason reason = fatal_other;
      try {
         chain_plug->accept_block(sb, sync_master->is_active(c));
         reason = no_reason;
      } catch( const unlinkable_block_exception &ex) {
         elog( "unlinkable_block_exception accept block #${n} syncing from ${p}",("n",blk_num)("p",c->peer_name()));
         reason = unlinkable;
      } catch( const block_validate_exception &ex) {
         elog( "block_validate_exception accept block #${n} syncing from ${p}",("n",blk_num)("p",c->peer_name()));
         reason = validation;
      } catch( const assert_exception &ex) {
         elog( "unable to accept block on assert exception ${n} from ${p}",("n",ex.to_string())("p",c->peer_name()));
      } catch( const fc::exception &ex) {
         elog( "accept_block threw a non-assert exception ${x} from ${p}",( "x",ex.to_string())("p",c->peer_name()));
         reason = no_reason;
      } catch( ...) {
         elog( "handle sync block caught something else from ${p}",("num",blk_num)("p",c->peer_name()));
      }
      big_msg_master->recv_block(c, msg);
      sync_master->recv_block(c, blk_id, blk_num, reason == no_reason);
   }

   void net_plugin_impl::handle_message( connection_ptr c, const signed_block &msg) {
      // should only be during synch or rolling upgrade
      chain_controller &cc = chain_plug->chain();
      block_id_type blk_id = msg.id();
      uint32_t blk_num = msg.block_num();
      fc_dlog(logger, "canceling wait on ${p}", ("p",c->peer_name()));
      c->cancel_wait();

      try {
         if( cc.is_known_block(blk_id)) {
            sync_master->recv_block(c, blk_id, blk_num, true);
            return;
         }
      } catch( ...) {
         // should this even be caught?
         elog("Caught an unknown exception trying to recall blockID");
      }

      fc::microseconds age( fc::time_point::now() - msg.timestamp);
      fc_dlog(logger, "got signed_block #${n} from ${p} block age in secs = ${age}",
              ("n",blk_num)("p",c->peer_name())("age",age.to_seconds()));

      go_away_reason reason = fatal_other;
      try {
         chain_plug->accept_block(msg, sync_master->is_active(c));
         reason = no_reason;
      } catch( const unlinkable_block_exception &ex) {
         elog( "unlinkable_block_exception accept block #${n} syncing from ${p}",("n",blk_num)("p",c->peer_name()));
         reason = unlinkable;
      } catch( const block_validate_exception &ex) {
         elog( "block_validate_exception accept block #${n} syncing from ${p}",("n",blk_num)("p",c->peer_name()));
         reason = validation;
      } catch( const assert_exception &ex) {
         elog( "unable to accept block on assert exception ${n} from ${p}",("n",ex.to_string())("p",c->peer_name()));
      } catch( const fc::exception &ex) {
         elog( "accept_block threw a non-assert exception ${x} from ${p}",( "x",ex.to_string())("p",c->peer_name()));
         reason = no_reason;
      } catch( ...) {
         elog( "handle sync block caught something else from ${p}",("num",blk_num)("p",c->peer_name()));
      }

      update_block_num ubn(blk_num);
      if( reason == no_reason ) {
         for (const auto &region : msg.regions) {
            for (const auto &cycle_sum : region.cycles_summary) {
               for (const auto &shard : cycle_sum) {
                  for (const auto &recpt : shard.transactions) {
                     auto ltx = local_txns.get<by_id>().find(recpt.id);
                     if( ltx != local_txns.end()) {
                        local_txns.modify( ltx, ubn );
                     }
                     auto ctx = c->trx_state.get<by_id>().find(recpt.id);
                     if( ctx != c->trx_state.end()) {
                        c->trx_state.modify( ctx, ubn );
                     }
                  }
               }
            }
         }
      }
      sync_master->recv_block(c, blk_id, blk_num, reason == no_reason);
   }

   void net_plugin_impl::start_conn_timer( ) {
      connector_check->expires_from_now( connector_period);
      connector_check->async_wait( [&](boost::system::error_code ec) {
            if( !ec) {
               connection_monitor( );
            }
            else {
               elog( "Error from connection check monitor: ${m}",( "m", ec.message()));
               start_conn_timer( );
            }
         });
   }

   void net_plugin_impl::start_txn_timer() {
      transaction_check->expires_from_now( txn_exp_period);
      transaction_check->async_wait( [&](boost::system::error_code ec) {
            if( !ec) {
               expire_txns( );
            }
            else {
               elog( "Error from transaction check monitor: ${m}",( "m", ec.message()));
               start_txn_timer( );
            }
         });
   }

   void net_plugin_impl::ticker() {
      keepalive_timer->expires_from_now (keepalive_interval);
      keepalive_timer->async_wait ([&](boost::system::error_code ec) {
            ticker ();
            if (ec) {
               wlog ("Peer keepalive ticked sooner than expected: ${m}", ("m", ec.message()));
            }
            for (auto &c : connections ) {
               if (c->socket->is_open()) {
                  c->send_time();
               }
            }
         });
   }

   void net_plugin_impl::start_monitors() {
      connector_check.reset(new boost::asio::steady_timer( app().get_io_service()));
      transaction_check.reset(new boost::asio::steady_timer( app().get_io_service()));
      start_conn_timer();
      start_txn_timer();
   }

   void net_plugin_impl::expire_txns() {
      start_txn_timer( );
      auto &old = local_txns.get<by_expiry>();
      auto ex_up = old.upper_bound( time_point::now());
      auto ex_lo = old.lower_bound( fc::time_point_sec( 0));
      old.erase( ex_lo, ex_up);

      auto &stale = local_txns.get<by_block_num>();
      chain_controller &cc = chain_plug->chain();
      uint32_t bn = cc.last_irreversible_block_num();
      auto bn_up = stale.upper_bound(bn);
      auto bn_lo = stale.lower_bound(1);
      stale.erase( bn_lo, bn_up);
   }

   void net_plugin_impl::connection_monitor( ) {
      start_conn_timer();
      vector <connection_ptr> discards;
      num_clients = 0;
      for( auto &c : connections ) {
         if( !c->socket->is_open() && !c->connecting) {
            if( c->peer_addr.length() > 0) {
               connect(c);
            }
            else {
               discards.push_back( c);
            }
         } else {
            if( c->peer_addr.empty()) {
               num_clients++;
            }
         }
      }
      if( discards.size( ) ) {
         for( auto &c : discards) {
            connections.erase( c );
            c.reset();
         }
      }
   }

   void net_plugin_impl::close( connection_ptr c ) {
      if( c->peer_addr.empty( ) ) {
         --num_clients;
      }
      c->close();
 }

   /**
    * This one is necessary to hook into the boost notifier api
    **/
   void net_plugin_impl::transaction_ready(const transaction_metadata& md, const packed_transaction& txn) {
      fc_dlog(logger,"transaction ready called, id = ${id}",("id",md.id));
      time_point_sec expire;
      if (md.decompressed_trx) {
         expire = md.decompressed_trx->expiration;
      }
      my_impl->big_msg_master->bcast_transaction(md.id, expire, txn);
   }

   void net_plugin_impl::broadcast_block_impl( const chain::signed_block &sb) {
      big_msg_master->bcast_block(sb);
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
         producer_plugin* pp = app().find_plugin<producer_plugin>();
         if(pp != nullptr)
            found_producer_key = pp->is_producer_key(msg.key);
         if( allowed_it == allowed_peers.end() && private_it == private_keys.end() && !found_producer_key) {
            elog( "Peer ${peer} sent a handshake with an unauthorized key: ${key}.",
                  ("peer", msg.p2p_address)("key", msg.key));
            return false;
         }
      }

      namespace sc = std::chrono;
      sc::system_clock::duration msg_time(msg.time);
      auto time = sc::system_clock::now().time_since_epoch();
      if(time - msg_time > peer_authentication_interval) {
         elog( "Peer ${peer} sent a handshake with a timestamp skewed by more than ${time}.",
               ("peer", msg.p2p_address)("time", "1 second")); // TODO Add to_variant for std::chrono::system_clock::duration
         return false;
      }

      if(msg.sig != chain::signature_type() && msg.token != sha256()) {
         sha256 hash = fc::sha256::hash(msg.time);
         if(hash != msg.token) {
            elog( "Peer ${peer} sent a handshake with an invalid token.",
                  ("peer", msg.p2p_address));
            return false;
         }
         chain::public_key_type peer_key;
         try {
            peer_key = crypto::public_key(msg.sig, msg.token, true);
         }
         catch (fc::exception& /*e*/) {
            elog( "Peer ${peer} sent a handshake with an unrecoverable key.",
                  ("peer", msg.p2p_address));
            return false;
         }
         if((allowed_connections & (Producers | Specified)) && peer_key != msg.key) {
            elog( "Peer ${peer} sent a handshake with an unauthenticated key.",
                  ("peer", msg.p2p_address));
            return false;
         }
      }
      else if(allowed_connections & (Producers | Specified)) {
         dlog( "Peer sent a handshake with blank signature and token, but this node accepts only authenticated connections.");
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
      producer_plugin* pp = app().find_plugin<producer_plugin>();
      if(pp != nullptr && pp->get_state() == abstract_plugin::started)
         return pp->sign_compact(signer, digest);
      return chain::signature_type();
   }

   void
   handshake_initializer::populate( handshake_message &hello) {
      hello.network_version = my_impl->network_version;
      hello.chain_id = my_impl->chain_id;
      hello.node_id = my_impl->node_id;
      hello.key = my_impl->get_authentication_key();
      hello.time = std::chrono::system_clock::now().time_since_epoch().count();
      hello.token = fc::sha256::hash(hello.time);
      hello.sig = my_impl->sign_compact(hello.key, hello.token);
      // If we couldn't sign, don't send a token.
      if(hello.sig == chain::signature_type())
         hello.token = sha256();
      hello.p2p_address = my_impl->p2p_address + " - " + hello.node_id.str().substr(0,7);
#if defined( __APPLE__ )
      hello.os = "osx";
#elif defined( __linux__ )
      hello.os = "linux";
#elif defined( _MSC_VER )
      hello.os = "win32";
#else
      hello.os = "other";
#endif
      hello.agent = my_impl->user_agent_name;


      chain_controller& cc = my_impl->chain_plug->chain();
      hello.head_id = fc::sha256();
      hello.last_irreversible_block_id = fc::sha256();
      hello.head_num = cc.head_block_num();
      hello.last_irreversible_block_num = cc.last_irreversible_block_num();
      if( hello.last_irreversible_block_num ) {
         try {
            hello.last_irreversible_block_id = cc.get_block_id_for_num(hello.last_irreversible_block_num);
         }
         catch( const unknown_block_exception &ex) {
            ilog("caught unkown_block");
            hello.last_irreversible_block_num = 0;
         }
      }
      if( hello.head_num ) {
         try {
            hello.head_id = cc.get_block_id_for_num( hello.head_num );
         }
         catch( const unknown_block_exception &ex) {
            hello.head_num = 0;
         }
      }
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
         ( "p2p-peer-address", bpo::value< vector<string> >()->composing(), "The public endpoint of a peer node to connect to. Use multiple p2p-peer-address options as needed to compose a network.")
         ( "agent-name", bpo::value<string>()->default_value("\"EOS Test Agent\""), "The name supplied to identify this node amongst the peers.")
#if 0 //disabling block summary support
         ( "send-whole-blocks", bpo::value<bool>()->default_value(def_send_whole_blocks), "True to always send full blocks, false to send block summaries" )
#endif
         ( "allowed-connection", bpo::value<vector<string>>()->multitoken()->default_value({"any"}, "any"), "Can be 'any' or 'producers' or 'specified' or 'none'. If 'specified', peer-key must be specified at least once. If only 'producers', peer-key is not required. 'producers' and 'specified' may be combined.")
         ( "peer-key", bpo::value<vector<string>>()->composing()->multitoken(), "Optional public key of peer allowed to connect.  May be used multiple times.")
         ( "peer-private-key", boost::program_options::value<vector<string>>()->composing()->multitoken(),
           "Tuple of [PublicKey, WIF private key] (may specify multiple times)")
         ( "log-level-net-plugin", bpo::value<string>()->default_value("info"), "Log level: one of 'all', 'debug', 'info', 'warn', 'error', or 'off'")
         ( "max-clients", bpo::value<int>()->default_value(def_max_clients), "Maximum number of clients from which connections are accepted, use 0 for no limit")
         ( "connection-cleanup-period", bpo::value<int>()->default_value(def_conn_retry_wait), "number of seconds to wait before cleaning up dead connections")
         ( "network-version-match", bpo::value<bool>()->default_value(false),
           "True to require exact match of peer network version.")
         ( "sync-fetch-span", bpo::value<uint32_t>()->default_value(def_sync_fetch_span), "number of blocks to retrieve in a chunk from any individual peer during synchronization")
         ;
   }

   template<typename T>
   T dejsonify(const string& s) {
      return fc::json::from_string(s).as<T>();
   }

   void net_plugin::plugin_initialize( const variables_map& options ) {
      ilog("Initialize net plugin");

      // Housekeeping so fc::logger::get() will work as expected
      fc::get_logger_map()[logger_name] = logger;
      // fc::get_logger_map()[connection::logger_name] = connection::logger;
      // fc::get_logger_map()[net_plugin_impl::logger_name] = net_plugin_impl::logger;
      // fc::get_logger_map()[sync_manager::logger_name] = sync_manager::logger;
      // fc::get_logger_map()[big_msg_manager::logger_name] = big_msg_manager::logger;

      // Setting a parent would in theory get us the default appenders for free but
      // a) the parent's log level overrides our own in that case; and
      // b) fc library's logger was never finished - the _additivity flag tested is never true.
      for(fc::shared_ptr<fc::appender>& appender : fc::logger::get().get_appenders()) {
         logger.add_appender(appender);
         // connection::logger.add_appender(appender);
         // net_plugin_impl::logger.add_appender(appender);
         // sync_manager::logger.add_appender(appender);
         // big_msg_manager::logger.add_appender(appender);
      }

      if( options.count( "log-level-net-plugin" ) ) {
         fc::log_level logl;

         fc::from_variant(options.at("log-level-net-plugin").as<string>(), logl);
         ilog("Setting net_plugin logging level to ${level}", ("level", logl));
         logger.set_log_level(logl);
         // connection::logger.set_log_level(logl);
         // net_plugin_impl::logger.set_log_level(logl);
         // sync_manager::logger.set_log_level(logl);
         // big_msg_manager::logger.set_log_level(logl);
      }

      my->network_version = static_cast<uint16_t>(app().version());
      my->network_version_match = options.at("network-version-match").as<bool>();
      my->send_whole_blocks = def_send_whole_blocks;

      my->sync_master.reset( new sync_manager(options.at("sync-fetch-span").as<uint32_t>() ) );
      my->big_msg_master.reset( new big_msg_manager );

      my->connector_period = std::chrono::seconds(options.at("connection-cleanup-period").as<int>());
      my->txn_exp_period = def_txn_expire_wait;
      my->resp_expected_period = def_resp_expected_wait;
      my->big_msg_master->just_send_it_max = def_max_just_send;
      my->max_client_count = options.at("max-clients").as<int>();

      my->num_clients = 0;
      my->started_sessions = 0;

      my->resolver = std::make_shared<tcp::resolver>( std::ref( app().get_io_service() ) );
      if(options.count("p2p-listen-endpoint")) {
         my->p2p_address = options.at("p2p-listen-endpoint").as< string >();
         auto host = my->p2p_address.substr( 0, my->p2p_address.find(':') );
         auto port = my->p2p_address.substr( host.size()+1, my->p2p_address.size() );
         idump((host)(port));
         tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
         // Note: need to add support for IPv6 too?

         my->listen_endpoint = *my->resolver->resolve( query);

         my->acceptor.reset( new tcp::acceptor( app().get_io_service() ) );
      }
      if(options.count("p2p-server-address")) {
         my->p2p_address = options.at("p2p-server-address").as< string >();
      }
      else {
         if(my->listen_endpoint.address().to_v4() == address_v4::any()) {
            boost::system::error_code ec;
            auto host = host_name(ec);
            if( ec.value() != boost::system::errc::success) {

               FC_THROW_EXCEPTION( fc::invalid_arg_exception,
                                   "Unable to retrieve host_name. ${msg}",( "msg",ec.message()));

            }
            auto port = my->p2p_address.substr( my->p2p_address.find(':'), my->p2p_address.size());
            my->p2p_address = host + port;
         }
      }

      if(options.count("p2p-peer-address")) {
         my->supplied_peers = options.at("p2p-peer-address").as<vector<string> >();
      }
      if(options.count("agent-name")) {
         my->user_agent_name = options.at("agent-name").as<string>();
      }

      if(options.count("allowed-connection")) {
         const std::vector<std::string> allowed_remotes = options["allowed-connection"].as<std::vector<std::string>>();
         for(const std::string& allowed_remote : allowed_remotes)
            {
               if(allowed_remote == "any")
                  my->allowed_connections |= net_plugin_impl::Any;
               else if(allowed_remote == "producers")
                  my->allowed_connections |= net_plugin_impl::Producers;
               else if(allowed_remote == "specified")
                  my->allowed_connections |= net_plugin_impl::Specified;
               else if(allowed_remote == "none")
                  my->allowed_connections = net_plugin_impl::None;
            }
      }

      if(my->allowed_connections & net_plugin_impl::Specified)
         FC_ASSERT(options.count("peer-key"), "At least one peer-key must accompany 'allowed-connection=specified'");

      if(options.count("peer-key")) {
         const std::vector<std::string> key_strings = options["peer-key"].as<std::vector<std::string>>();
         for(const std::string& key_string : key_strings)
            {
               my->allowed_peers.push_back(dejsonify<chain::public_key_type>(key_string));
            }
      }

      if(options.count("peer-private-key"))
         {
            const std::vector<std::string> key_id_to_wif_pair_strings = options["peer-private-key"].as<std::vector<std::string>>();
            for(const std::string& key_id_to_wif_pair_string : key_id_to_wif_pair_strings)
               {
                  auto key_id_to_wif_pair = dejsonify<std::pair<chain::public_key_type, std::string>>(key_id_to_wif_pair_string);
                  my->private_keys[key_id_to_wif_pair.first] = fc::crypto::private_key(key_id_to_wif_pair.second);
               }
         }

      if( options.count( "send-whole-blocks")) {
         my->send_whole_blocks = options.at( "send-whole-blocks" ).as<bool>();
      }

      my->chain_plug = app().find_plugin<chain_plugin>();
      my->chain_plug->get_chain_id(my->chain_id);
      fc::rand_pseudo_bytes(my->node_id.data(), my->node_id.data_size());
      ilog("my node_id is ${id}",("id",my->node_id));

      my->keepalive_timer.reset(new boost::asio::steady_timer(app().get_io_service()));
      my->ticker();
   }

   void net_plugin::plugin_startup() {
      if( my->acceptor ) {
         my->acceptor->open(my->listen_endpoint.protocol());
         my->acceptor->set_option(tcp::acceptor::reuse_address(true));
         my->acceptor->bind(my->listen_endpoint);
         my->acceptor->listen();
         ilog("starting listener, max clients is ${mc}",("mc",my->max_client_count));
         my->start_listen_loop();
      }

      my->chain_plug->chain().on_pending_transaction.connect( &net_plugin_impl::transaction_ready);
      my->start_monitors();

      for( auto seed_node : my->supplied_peers ) {
         connect( seed_node );
      }
   }

   void net_plugin::plugin_shutdown() {
      try {
         ilog( "shutdown.." );
         my->done = true;
         if( my->acceptor ) {
            ilog( "close acceptor" );
            my->acceptor->close();

            ilog( "close ${s} connections",( "s",my->connections.size()) );
            auto cons = my->connections;
            for( auto con : cons ) {
               my->close( con);
            }

            my->acceptor.reset(nullptr);
         }
         ilog( "exit shutdown" );
      }
      FC_CAPTURE_AND_RETHROW()
   }

   void net_plugin::broadcast_block( const chain::signed_block &sb) {
      fc_dlog(logger, "broadcasting block #${num}",("num",sb.block_num()) );
      my->broadcast_block_impl( sb);
   }

   size_t net_plugin::num_peers() const {
      return my->count_open_sockets();
   }

   /**
    *  Used to trigger a new connection from RPC API
    */
   string net_plugin::connect( const string& host ) {
      if( my->find_connection( host ) )
         return "already connected";

      connection_ptr c = std::make_shared<connection>(host);
      fc_dlog(logger,"adding new connection to the list");
      my->connections.insert( c );
      fc_dlog(logger,"calling active connector");
      my->connect( c );
      return "added connection";
   }

   string net_plugin::disconnect( const string& host ) {
      for( auto itr = my->connections.begin(); itr != my->connections.end(); ++itr ) {
         if( (*itr)->peer_addr == host ) {
            (*itr)->reset();
            my->close(*itr);
            my->connections.erase(itr);
            return "connection removed";
         }
      }
      return "no known connection for host";
   }

   optional<connection_status> net_plugin::status( const string& host )const {
      auto con = my->find_connection( host );
      if( con )
         return con->get_status();
      return optional<connection_status>();
   }

   vector<connection_status> net_plugin::connections()const {
      vector<connection_status> result;
      result.reserve( my->connections.size() );
      for( const auto& c : my->connections ) {
         result.push_back( c->get_status() );
      }
      return result;
   }
   connection_ptr net_plugin_impl::find_connection( string host )const {
      for( const auto& c : connections )
         if( c->peer_addr == host ) return c;
      return connection_ptr();
   }
}
