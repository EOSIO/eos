/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/types.hpp>

#include <eos/net_plugin/net_plugin.hpp>
#include <eos/net_plugin/protocol.hpp>
#include <eos/net_plugin/message_buffer.hpp>
#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/block.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/producer_plugin/producer_plugin.hpp>
#include <eos/utilities/key_conversion.hpp>

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
  using std::vector;

  using boost::asio::ip::tcp;
  using boost::asio::ip::address_v4;
  using boost::asio::ip::host_name;
  using boost::intrusive::rbtree;

  using fc::time_point;
  using fc::time_point_sec;
  using eosio::chain::transaction_id_type;
  namespace bip = boost::interprocess;

  class connection;
  class sync_manager;


  using connection_ptr = std::shared_ptr<connection>;
  using connection_wptr = std::weak_ptr<connection>;

  using socket_ptr = std::shared_ptr<tcp::socket>;

  using net_message_ptr = shared_ptr<net_message>;

  struct node_transaction_state {
    transaction_id_type id;
    fc::time_point      received;
    fc::time_point_sec  expires;
    vector<char>        packed_transaction; /// the received raw bundle
    uint32_t            block_num = -1; /// block transaction was included in
    bool                validated = false; /// whether or not our node has validated it
  };

  struct update_block_num {
    uint16 new_bnum;
    update_block_num (uint16 bnum) : new_bnum(bnum) {}
    void operator() (node_transaction_state& nts) {
        nts.block_num = static_cast<uint32_t>(new_bnum);
    }
  };

  struct update_entry {
    const signed_transaction &txn;
    update_entry (const signed_transaction &msg) : txn(msg) {}

    void operator() (node_transaction_state& nts) {
      nts.received = fc::time_point::now();
      nts.validated = true;
      size_t bufsiz = fc::raw::pack_size(txn);
      nts.packed_transaction.resize(bufsiz);
      fc::datastream<char*> ds( nts.packed_transaction.data(), bufsiz );
      fc::raw::pack( ds, txn );
    }
  };

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
    uint32_t                         max_client_count;
    uint32_t                         num_clients;

    vector<string>                   supplied_peers;
    vector<chain::public_key_type>   allowed_peers; ///< peer keys allowed to connect
    std::map<chain::public_key_type,
             fc::ecc::private_key>   private_keys; ///< overlapping with producer keys, also authenticating non-producing nodes

    enum possible_connections : char {
      None = 0,
      Producers = 1 << 0,
      Specified = 1 << 1,
      Any = 1 << 2
    };
    possible_connections             allowed_connections{None};

    std::set< connection_ptr >       connections;
    bool                             done = false;
    unique_ptr< sync_manager >       sync_master;

    unique_ptr<boost::asio::steady_timer> connector_check;
    unique_ptr<boost::asio::steady_timer> transaction_check;
    unique_ptr<boost::asio::steady_timer> keepalive_timer;
    boost::asio::steady_timer::duration   connector_period;
    boost::asio::steady_timer::duration   txn_exp_period;
    boost::asio::steady_timer::duration   resp_expected_period;
    boost::asio::steady_timer::duration   keepalive_interval{std::chrono::seconds{32}};

    const std::chrono::system_clock::duration peer_authentication_interval{std::chrono::seconds{1}}; ///< Peer clock may be no more than 1 second skewed from our clock, including network latency.

    int16_t                       network_version;
    chain_id_type                 chain_id;
    fc::sha256                    node_id;

    string                        user_agent_name;
    chain_plugin*                 chain_plug;
    size_t                        just_send_it_max;
    bool                          send_whole_blocks;
    int                           started_sessions;

    node_transaction_index        local_txns;
    ordered_txn_ids               pending_notify;

    shared_ptr<tcp::resolver>     resolver;

    void connect( connection_ptr c );
    void connect( connection_ptr c, tcp::resolver::iterator endpoint_itr );
    void start_session( connection_ptr c );
    void start_listen_loop( );
    void start_read_message( connection_ptr c);

    void close( connection_ptr c );
    size_t count_open_sockets () const;

    template<typename VerifierFunc>
    void send_all (const net_message &msg, VerifierFunc verify);
    //    template<typename VerifierFunc>
    //    void send_all (net_message_ptr msg, VerifierFunc verify);
    void send_all_txn (const signed_transaction& txn);
    static void transaction_ready( const signed_transaction& txn);
    void broadcast_block_impl( const signed_block &sb);

    size_t cache_txn ( const transaction_id_type, const signed_transaction &txn);

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
    void handle_message( connection_ptr c, const block_summary_message &msg);
    void handle_message( connection_ptr c, const signed_transaction &msg);
    void handle_message( connection_ptr c, const signed_block &msg);

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
    fc::ecc::compact_signature sign_compact(const chain::public_key_type& signer, const fc::sha256& digest) const;
    static const fc::string logger_name;
    static fc::logger logger;
  };

  template<class enum_type, class=typename std::enable_if<std::is_enum<enum_type>::value>::type>
  inline enum_type& operator|=(enum_type& lhs, const enum_type& rhs)
  {
    using T = std::underlying_type_t <enum_type>;
    return lhs = static_cast<enum_type>(static_cast<T>(lhs) | static_cast<T>(rhs));
  }

  static net_plugin_impl *my_impl;
  const fc::string net_plugin_impl::logger_name("net_plugin_impl");
  fc::logger net_plugin_impl::logger(net_plugin_impl::logger_name);

  /**
   * default value initializers
   */
  constexpr auto     def_send_buffer_size_mb = 4;
  constexpr auto     def_send_buffer_size = 1024*1024*def_send_buffer_size_mb;
  constexpr auto     def_max_clients = 25; // 0 for unlimited clients
  constexpr auto     def_conn_retry_wait = 30;
  constexpr auto     def_txn_expire_wait = std::chrono::seconds (3);
  constexpr auto     def_resp_expected_wait = std::chrono::seconds (1);
  constexpr auto     def_sync_rec_span = 10;
  constexpr auto     def_max_just_send = 1300 * 3; // "mtu" * 3
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
    uint32_t            block_num = -1; ///< the block number the transaction was included in
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


  struct make_known {
    void operator() (block_state& bs) {
      bs.is_known = true;
    }
  };

  /**
   * Index by start_block_num
   */
  struct sync_state {
    sync_state(uint32_t start = 0, uint32_t end = 0, uint32_t last_acted = 0)
      :start_block ( start ), end_block( end ), last( last_acted ),
       start_time (time_point::now()), block_cache()
    {}
    uint32_t     start_block;
    uint32_t     end_block;
    uint32_t     last; ///< last sent or received
    time_point   start_time; ///< time request made or received
    deque<vector<char> > block_cache;
  };

  using sync_state_ptr = shared_ptr< sync_state >;

  struct handshake_initializer {
    static void populate (handshake_message &hello);
  };

  class connection : public std::enable_shared_from_this<connection> {
  public:
    connection( string endpoint,
                size_t send_buf_size = def_send_buffer_size );

    connection( socket_ptr s,
                size_t send_buf_size = def_send_buffer_size );
    ~connection();
    void initialize ();

    block_state_index       block_state;
    transaction_state_index trx_state;
    sync_state_ptr          sync_receiving;  // we are requesting info from this peer
    sync_state_ptr          sync_requested;  // this peer is requesting info from us
    socket_ptr              socket;

    message_buffer<4096>    pending_message_buffer;
    vector<char>            send_buffer;
    vector<char>            blk_buffer;

    deque< vector<char> >   txn_queue;
    size_t                  txn_in_flight;
    bool                    halt_txn_send;

    fc::sha256              node_id;
    handshake_message       last_handshake;
    int16_t                 sent_handshake_count;
    deque<net_message>      out_queue;
    bool                    connecting;
    bool                    syncing;
    string                  peer_addr;
    unique_ptr<boost::asio::steady_timer> response_expected;
    optional<request_message> pending_fetch;
    go_away_reason         no_retry;

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

    bool connected ();
    bool current ();
    void reset ();
    void close ();
    void send_handshake ();

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
    uint32_t send_branch (chain_controller &cc, block_id_type bid, uint32_t lib_num, block_id_type lib_id);

    void blk_send_branch( const vector<block_id_type> &ids);
    void blk_send(const vector<block_id_type> &txn_lis);
    void stop_send();

    void enqueue( const net_message &msg );
    bool enqueue_sync_block ();
    void send_next_message();
    void send_next_txn();

    void sync_wait ();
    void fetch_wait ();
    void sync_timeout (boost::system::error_code ec);
    void fetch_timeout (boost::system::error_code ec);

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

    static const fc::string logger_name;
    static fc::logger logger;
  };

  const fc::string connection::logger_name("connection");
  fc::logger connection::logger(connection::logger_name);

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
  public:
    uint32_t            sync_known_lib_num;
    uint32_t            sync_last_requested_num;
    uint32_t            sync_req_span;

    deque<sync_state_ptr> full_chunks;
    deque<sync_state_ptr> partial_chunks;
    deque<block_id_type> _blocks;
    chain_plugin * chain_plug;

  public:
    sync_manager (uint32_t span);
    bool syncing ();
    void request_next_chunk ();
    void assign_chunk (connection_ptr c);
    void apply_chunk (sync_state_ptr ss);
    void take_chunk (connection_ptr c);
    void start_sync (connection_ptr c, uint32_t target);

    void set_blocks_to_fetch (vector<block_id_type>);
    void assign_fectch (connection_ptr c);
    void reassign_fetch (connection_ptr c);

    static const fc::string logger_name;
    static fc::logger logger;
  };

  const fc::string sync_manager::logger_name("sync_manager");
  fc::logger sync_manager::logger(sync_manager::logger_name);

  //---------------------------------------------------------------------------

  connection::connection( string endpoint,
                          size_t send_buf_size )
      : block_state(),
        trx_state(),
        sync_receiving(),
        sync_requested(),
        socket( std::make_shared<tcp::socket>( std::ref( app().get_io_service() ))),
        send_buffer(send_buf_size),
        node_id(),
        last_handshake(),
        sent_handshake_count(0),
        out_queue(),
        connecting (false),
        syncing (false),
        peer_addr (endpoint),
        response_expected (),
        pending_fetch (),
        no_retry (go_away_reason::no_reason)
    {
      wlog( "created connection to ${n}", ("n", endpoint) );
      initialize();
    }

  connection::connection( socket_ptr s,
                          size_t send_buf_size )
      : block_state(),
        trx_state(),
        sync_receiving(),
        sync_requested(),
        socket( s ),
        send_buffer(send_buf_size),
        node_id(),
        last_handshake(),
        sent_handshake_count(0),
        out_queue(),
        connecting (false),
        syncing (false),
        peer_addr (),
        response_expected (),
        pending_fetch(),
        no_retry (go_away_reason::no_reason)
    {
      wlog( "accepted network connection" );
      initialize ();
    }

  connection::~connection() {
      if (peer_addr.empty())
        wlog( "released connection from client" );
      else
        wlog( "released connection to server at ${addr}", ("addr", peer_addr) );
    }

  void connection::initialize () {
      auto *rnd = node_id.data();
      rnd[0] = 0;
      response_expected.reset(new boost::asio::steady_timer (app().get_io_service()));
    }

  bool connection::connected () {
    return (socket->is_open() && !connecting);
  }

  bool connection::current () {
    if( syncing ) {
      fc_dlog(logger, "skipping connection ${n} due to syncing", ("n",peer_name()));
    } else if (!connected()) {
      fc_dlog(logger, "skipping connection ${n} due to not connected", ("n",peer_name()));
    } else {
      fc_dlog(logger, "connection ${n} is current", ("n",peer_name()));
    }
    return (connected() && !syncing);
  }

  void connection::reset () {
      sync_requested.reset();
      block_state.clear();
      trx_state.clear();
    }

    void connection::close () {
      if (socket) {
        socket->close();
      }
      connecting = false;
      syncing = false;
      out_queue.clear();
      if (response_expected) {
        response_expected->cancel();
      }
    }

  void connection::txn_send_pending (const vector<transaction_id_type> &ids) {
    for (auto t : my_impl->local_txns){
      if (t.packed_transaction.size()) {
        bool found = false;
        for (auto l : ids) {
          if ( l == t.id) {
            found = true;
            break;
          }
        }
        if (!found) {
          txn_queue.push_back( t.packed_transaction );
        }
      }
    }
  }

  void connection::txn_send(const vector<transaction_id_type> &ids) {
    for (auto t : ids) {
      auto n = my_impl->local_txns.get<by_id>().find(t);
      if (n != my_impl->local_txns.end() &&
          n->packed_transaction.size()) {
        txn_queue.push_back( n->packed_transaction );
      }
    }
  }

  uint32_t connection::send_branch (chain_controller &cc, block_id_type bid, uint32_t lib_num, block_id_type lib_id) {
    static uint32_t dbg_depth = 0;
    uint32_t count = 0;
    try {
      optional<signed_block> b = cc.fetch_block_by_id (bid);
      if( b ) {
        block_id_type prev = b->previous;
        if( prev == lib_id) {
          enqueue( *b );
          count = 1;
        }
        else {
          uint32_t pnum = block_header::num_from_id( prev );
          if( pnum < lib_num ) {
            uint32_t irr = cc.last_irreversible_block_num();
            elog( "Whoa! branch regression passed last irreversible block! depth = ${d}", ("d",dbg_depth ));
            optional<signed_block> pb = cc.fetch_block_by_id (prev);
            block_id_type pprev;
            if (!pb ) {
              fc_dlog(logger, "no block for prev");
            } else {
              pprev = pb->previous;
            }
            fc_dlog(logger, "irr = ${irr} lib_nim = ${ln} pnum = ${pn}", ("irr",irr)("ln",lib_num)("pn",pnum));
            fc_dlog(logger, "prev = ${p}", ("p",prev));
            fc_dlog(logger, "lib  = ${p}", ("p",lib_id));
            fc_dlog(logger, "bid  = ${p}", ("p",bid));
            fc_dlog(logger, "ppre = ${p}", ("p",pprev));
          }
          else {
            ++dbg_depth;
            count = send_branch (cc, prev, lib_num, lib_id );
            --dbg_depth;
            if (count > 0) {
              enqueue( *b );
              ++count;
            }
          }
        }
      }
    } catch (...) {
      elog( "Send Branch failed, caught unidentified exception");
    }
    return count;
  }


  void connection::blk_send_branch(const vector<block_id_type> &ids) {
    chain_controller &cc = my_impl->chain_plug->chain();
    uint32_t head_num = cc.head_block_num ();
    notice_message note;
    note.known_blocks.mode = normal;
    note.known_blocks.pending = 0;
    fc_dlog(logger, "head_num = ${h}",("h",head_num));
    if (head_num == 0) {
      enqueue (note);
      return;
    }
    block_id_type head_id;
    block_id_type lib_id;
    uint32_t lib_num;
    try {
      lib_num = cc.last_irreversible_block_num();
      if( lib_num != 0 )
        lib_id = cc.get_block_id_for_num(lib_num);
      head_id = cc.get_block_id_for_num (head_num);
    }
    catch (const assert_exception &ex) {
      fc_dlog(logger, "caught assert ${x}",("x",ex.what()));
      enqueue (note);
      return;
    }
#if 0
    rbtree<block_id_type> sorted_ids ;
    if(!ids.empty()) {
      for (const auto & exclude : ids) {
        sorted_ids.insert (exclude);
      }
    }
#endif
    uint32_t count = send_branch (cc, head_id, lib_num, lib_id);
    fc_dlog(logger, "Sent ${n} blocks on my fork",("n",count));
    syncing = false;
  }

  void connection::blk_send(const vector<block_id_type> &ids) {
    chain_controller &cc = my_impl->chain_plug->chain();

    for (auto blkid : ids) {
      try {
        optional<signed_block> b = cc.fetch_block_by_id (blkid);
        if (b) {
          enqueue (*b);
        }
      }
      catch (const assert_exception &ex) {
        elog( "caught assert on fetch_block_by_id, ${ex}",("ex",ex.what()));
        // keep going, client can ask another peer
      }
      catch (...) {
        elog( "failed to retrieve block for id");
      }
    }
  }

  void connection::stop_send() {
    if (txn_in_flight > 0) {
      halt_txn_send = true;
    }
    else {
      txn_queue.clear();
    }
  }

    void connection::send_handshake ( ) {
      handshake_message hello;
      handshake_initializer::populate(hello);
      hello.generation = ++sent_handshake_count;
      fc_dlog(logger, "Sending handshake to ${ep}", ("ep", peer_addr));
      enqueue (hello);
    }

  char* connection::convert_tstamp(const tstamp& t)
  {
    const long long NsecPerSec{1000000000};
    time_t seconds = t / NsecPerSec;
    strftime(ts, ts_buffer_size, "%F %T", localtime(&seconds));
    snprintf(ts+19, ts_buffer_size-19, ".%lld", t % NsecPerSec);
    return ts;
  }

  void connection::send_time () {
    time_message xpkt;
    xpkt.org = rec;
    xpkt.rec = dst;
    xpkt.xmt = get_time();
    org = xpkt.xmt;
    enqueue(xpkt);
  }

  void connection::send_time (const time_message& msg) {
    time_message xpkt;
    xpkt.org = msg.xmt;
    xpkt.rec = msg.dst;
    xpkt.xmt = get_time();
    enqueue(xpkt);
  }

  void connection::enqueue( const net_message &m ) {
    out_queue.push_back( m );
    if( out_queue.size() == 1 ) {
      send_next_message();
    }
  }

  bool connection::enqueue_sync_block ( ) {
    chain_controller& cc = app().find_plugin<chain_plugin>()->chain();
    uint32_t num = ++sync_requested->last;

    if (num == sync_requested->end_block) {
      sync_requested.reset();
    }
    try {
      fc::optional<signed_block> sb = cc.fetch_block_by_number(num);
      if (sb) {
        enqueue( *sb );
        return true;
      }
    } catch ( ... ) {
      wlog( "write loop exception" );
    }
    return false;
  }

  void connection::send_next_message() {
    if( !out_queue.size() ) {
      if( !sync_requested || !enqueue_sync_block( ) ) {
        send_next_txn ();
      }
      return;
    }

    auto& m = out_queue.front();
    if (m.contains<sync_request_message>()) {
      sync_wait( );
    } else if (m.contains<request_message>()) {
      pending_fetch = m.get<request_message>();
      fetch_wait( );
    }

    uint32_t payload_size = fc::raw::pack_size( m );
    char * header = reinterpret_cast<char*>(&payload_size);
    size_t header_size = sizeof(payload_size);

    size_t buffer_size = header_size + payload_size;

    fc::datastream<char*> ds( send_buffer.data(), buffer_size);
    ds.write( header, header_size );
    fc::raw::pack( ds, m );
    boost::asio::async_write( *socket, boost::asio::buffer( send_buffer, buffer_size ),
                              [this]( boost::system::error_code ec, std::size_t /*bytes_transferred*/ ) {
                                if( ec ) {
                                  elog( "Error sending message: ${msg}", ("msg",ec.message() ) );
                                } else  {
                                  if (out_queue.size()) {
                                    if (out_queue.front().contains<go_away_message>()) {
                                      close();
                                      return;
                                    }
                                    out_queue.pop_front();
                                  }
                                  send_next_message();
                                }
                              });
  }

  void connection::send_next_txn() {
    if( !txn_queue.size() || halt_txn_send) {
      return;
    }

    ssize_t limit = 65535;
    while (txn_in_flight < txn_queue.size() && limit > 0 ) {
      limit -= txn_queue[txn_in_flight].size();
      if (limit >= 0 || txn_in_flight == 0) {
        txn_in_flight++;
      }
    }
    for (size_t i = 0; i < txn_in_flight; i++) {
      boost::asio::async_write( *socket, boost::asio::buffer(txn_queue[i], txn_queue[i].size()),
                                [this, i]( boost::system::error_code ec, std::size_t /*bytes_transferred*/ ) {
                                  if( ec ) {
                                    elog( "Error sending txn block: ${msg}", ("msg",ec.message() ) );
                                  } else  {
                                    if (i == txn_in_flight - 1) {
                                      if (halt_txn_send) {
                                        txn_queue.clear();
                                        txn_in_flight = 0;
                                        halt_txn_send = false;
                                      }
                                      else {
                                        while (txn_in_flight-- > 0) {
                                          txn_queue.pop_front();
                                        }
                                      }
                                      send_next_message();
                                    }
                                  }
                                });
    }
  }

  void connection::sync_wait( ) {
    response_expected->expires_from_now( my_impl->resp_expected_period);
    response_expected->async_wait( boost::bind(&connection::sync_timeout,
                                               this, boost::asio::placeholders::error));
  }

  void connection::fetch_wait( ) {
    response_expected->expires_from_now( my_impl->resp_expected_period);
    response_expected->async_wait( boost::bind(&connection::fetch_timeout,
                                               this, boost::asio::placeholders::error));
  }

  void connection::sync_timeout( boost::system::error_code ec ) {
    if( !ec ) {
      if( sync_receiving && sync_receiving->last < sync_receiving->end_block) {
        enqueue( (sync_request_message) {0,0});
        my_impl->sync_master->take_chunk (shared_from_this());
      }
    }
    else if( ec == boost::asio::error::operation_aborted) {
      if( !connected()) {
        my_impl->sync_master->take_chunk (shared_from_this());
      }
    }
    else {
      elog ("setting timer for sync request got error ${ec}",("ec", ec.message()));
    }
  }

  const string connection::peer_name () {
    if( !peer_addr.empty() ) {
      return peer_addr;
    }
    if( !last_handshake.p2p_address.empty() ) {
      return last_handshake.p2p_address;
    }
    return "connecting client";
  }

  void connection::fetch_timeout( boost::system::error_code ec ) {
    if( !ec ) {
      if( !( pending_fetch->req_trx.empty( ) || pending_fetch->req_blocks.empty( ) ) ) {
        enqueue( ( request_message ) {ordered_txn_ids( ), ordered_blk_ids( )} );
        my_impl->sync_master->reassign_fetch( shared_from_this( ) );
      }
    }
    else if( ec == boost::asio::error::operation_aborted ) {
      if( !connected( ) ) {
        fc_dlog(logger, "fetch timeout was cancelled due to dead connection");
        my_impl->sync_master->reassign_fetch( shared_from_this( ) );
      }
    }
    else {
      elog( "setting timer for fetch request got error ${ec}", ("ec", ec.message( ) ) );
    }
  }

  bool connection::process_next_message(net_plugin_impl& impl, uint32_t message_length) {
    try {
      // If it is a signed_block, then save the raw message for the cache
      // This must be done before we unpack the message.
      unsigned_int which;
      auto index = pending_message_buffer.read_index();
      pending_message_buffer.peek(&which, sizeof(unsigned_int), index);
      if (which == uint32_t(net_message::tag<signed_block>::value)) {
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

  sync_manager::sync_manager( uint32_t span )
    :sync_known_lib_num( 0 )
    ,sync_last_requested_num( 0 )
    ,sync_req_span( span )
  {
    chain_plug = app( ).find_plugin<chain_plugin>( );
  }

  bool sync_manager::syncing( ) {
    fc_dlog(logger, "ours = ${ours} known = ${known} head = ${head}",("ours",sync_last_requested_num)("known",sync_known_lib_num)("head",chain_plug->chain( ).head_block_num( )));
    return( sync_last_requested_num != sync_known_lib_num ||
            chain_plug->chain( ).head_block_num( ) < sync_last_requested_num );
  }

  void sync_manager::request_next_chunk () {
    uint32_t head_block = chain_plug->chain().head_block_num();
    for (auto &c : my_impl->connections ) {
      if (c->sync_receiving && c->sync_receiving->start_block == head_block + 1) {
        c->enqueue( (sync_request_message){c->sync_receiving->start_block,
              c->sync_receiving->end_block});
        sync_last_requested_num = c->sync_receiving->end_block;
      }
    }
  }

  void sync_manager::assign_chunk( connection_ptr c ) {
    uint32_t start = 0;
    uint32_t end = 0;

    if( !partial_chunks.empty( ) ) {
      c->sync_receiving = partial_chunks.front( );
      partial_chunks.pop_front( );
      start = c->sync_receiving->last + 1;
      end = c->sync_receiving->end_block;
    }
    else if( sync_last_requested_num != sync_known_lib_num ) {
      start = sync_last_requested_num + 1;
      end = ( start + sync_req_span - 1 );
      if( end > sync_known_lib_num )
        end = sync_known_lib_num;
      if( end > 0 && end >= start ) {
        fc_dlog(logger, "conn ${n} recv blks ${s} to ${e}",("n",c->peer_name() )("s",start)("e",end));
        c->sync_receiving.reset(new sync_state( start, end, sync_last_requested_num ) );
      }
    }
    else {
      fc_dlog(logger, "conn ${n} resetting sync recv",("n",c->peer_name() ));
      c->sync_receiving.reset ( );
    }
#if 0
    if( end > 0 && end >= start ) {
      c->enqueue( (sync_request_message){start, end} );
      sync_last_requested_num = end;
    }
#endif
  }

    struct postcache : public fc::visitor<void> {
      chain_plugin * chain_plug;
      postcache (chain_plugin *cp) : chain_plug (cp) {}

      void operator( )(const signed_block &block) const
      {
        try {
          chain_plug->accept_block( block,true );
        } catch( const unlinkable_block_exception &ex ) {
          elog( "post cache: unlinkable_block_exception accept block #${n}",("n",block.block_num()));
        } catch (const assert_exception &ex) {
          elog ("post cache: unable to accept block on assert exception ${n}",("n",ex.to_string()));
        } catch (const fc::exception &ex) {
          elog ("post cache: accept_block threw a non-assert exception ${x}", ("x",ex.what()));
        } catch (...) {
          elog ("post cache: unknown error accepting cached block");
        }
      }

      template <typename T> void operator()(const T &msg) const { /* no-op */ }
    };

    void sync_manager::apply_chunk( sync_state_ptr ss) {
      postcache pc(chain_plug);
      for( auto & blk : ss->block_cache) {
        auto block = fc::raw::unpack<net_message>( blk );
        block.visit( pc);
      }
    }

    void sync_manager::take_chunk( connection_ptr c) {
      if( !c->sync_receiving) {
        elog( "take_chunk called, but sync_receiving is empty");
        return;
      }
      sync_state_ptr ss;
      c->sync_receiving.swap(ss);
      fc_dlog(logger, "conn ${n} losing recv blks ${s} to ${e}",("n",c->peer_name() )("s",ss->start_block)("e",ss->end_block));

      if( !ss->block_cache.empty()) {
        if( ss->last < ss->end_block) {
          partial_chunks.push_back( ss );
          return;
        }

        if( ss->start_block != chain_plug->chain().last_irreversible_block_num() + 1) {
          bool found = false;
          for( auto pos = full_chunks.begin(); !found && pos != full_chunks.end(); ++pos) {
            if( ss->end_block <( *pos)->start_block) {
              full_chunks.insert( pos,ss);
              found = true;
            }
          }
          if( !found) {
            full_chunks.push_back( ss); //either full chunks is empty or pos ran off the end
          }
        }
        else {
          apply_chunk( ss);
        }
      }
      while( !full_chunks.empty()) {
        auto chunk = full_chunks.front();
        if( chunk->start_block == chain_plug->chain().head_block_num() + 1) {
          apply_chunk( chunk);
          if( chunk->last == chunk->end_block ) {
            full_chunks.pop_front();
          }
          else {
            chunk->start_block = chunk->last+1;
            break;
          }
        }
        else
          break;
      }

      if( chain_plug->chain().head_block_num() == sync_known_lib_num ) {
        handshake_message hello;
        handshake_initializer::populate(hello);
        fc_dlog(logger, "All caught up with last known last irreversible block resending handshake");
        for( auto &ci : my_impl->connections) {
          if( ci->current()) {
            hello.generation = ++ci->sent_handshake_count;
            fc_dlog(logger, "send to ${p}", ("p",ci->peer_name()));
            ci->enqueue( hello );
          }
        }
      }
      if( c->connected()) {
        assign_chunk( c);
      }
      request_next_chunk();
    }

    void sync_manager::start_sync( connection_ptr c, uint32_t target) {
      if( !syncing()) {
        sync_last_requested_num = chain_plug->chain().head_block_num();
      }
      ilog( "Catching up with chain, our last req is ${cc}, theirs is ${t}",
           ( "cc",sync_last_requested_num)("t",target));
      if( target > sync_known_lib_num) {
        sync_known_lib_num = target;
      }
      if( c->sync_receiving && c->sync_receiving->end_block > 0) {
        return;
      }
      assign_chunk( c);
      request_next_chunk();
    }

  void sync_manager::reassign_fetch( connection_ptr c) {
#warning( "TODO: migrate remaining fetch requests to other peers");
  }

  //------------------------------------------------------------------------

  void net_plugin_impl::connect( connection_ptr c ) {
    if( c->no_retry != go_away_reason::no_reason) {
      fc_dlog( logger, "Skipping connect due to go_away reason ${r}",("r", reason_str( c->no_retry )));
      return;
    }
      auto host = c->peer_addr.substr( 0, c->peer_addr.find(':') );
      auto port = c->peer_addr.substr( host.size()+1, host.size() );
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
      c->socket->async_connect( current_endpoint,
                           [c, endpoint_itr, this]
                          (  const boost::system::error_code& err ) {
                             if( !err ) {
                               start_session( c );
                             } else {
                               if( endpoint_itr != tcp::resolver::iterator() ) {
                                 c->close();
                                 connect( c, endpoint_itr );
                               }
                               else {
                                 elog( "connection failed to ${peer}: ${error}",
                                       ( "peer", c->peer_name())("error",err.message()));
                                 c->connecting = false;
                                 c->close();
                                 if (started_sessions == 0) {
                                   connect (c);
                                 }
                               }
                             }
                           } );
    }

    void net_plugin_impl::start_session( connection_ptr con ) {
      boost::asio::ip::tcp::no_delay option( true );
      con->socket->set_option( option );
      start_read_message( con );
      ++started_sessions;
      con->send_handshake( );

      // for now, we can just use the application main loop.
      //     con->readloop_complete  = bf::async( [=](){ read_loop( con ); } );
      //     con->writeloop_complete = bf::async( [=](){ write_loop con ); } );
    }


    void net_plugin_impl::start_listen_loop( ) {
      auto socket = std::make_shared<tcp::socket>( std::ref( app().get_io_service() ) );
      acceptor->async_accept( *socket, [socket,this]( boost::system::error_code ec ) {
          if( !ec ) {
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
      connection_wptr c( conn);
      conn->socket->async_read_some(
        conn->pending_message_buffer.get_buffer_sequence_for_boost_async_read(),
        [this,c]( boost::system::error_code ec, std::size_t bytes_transferred ) {
          if( !ec ) {
            connection_ptr conn = c.lock();
            if (!conn) {
              return;
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
                if (bytes_in_buffer >= message_length + message_header_size) {
                  conn->pending_message_buffer.advance_read_ptr(message_header_size);
                  if (!conn->process_next_message(*this, message_length)) {
                    return;
                  }
                } else {
                  conn->pending_message_buffer.add_space(message_length - bytes_in_buffer);
                  break;
                }
              }
            }
            start_read_message(conn);
          } else {
            elog( "Error reading message from connection: ${m}",( "m", ec.message() ) );
            close( c.lock() );
          }
        }
      );
    }

  size_t net_plugin_impl::count_open_sockets () const
  {
    size_t count = 0;
    for( auto &c : connections) {
      if (c->socket->is_open())
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

    void net_plugin_impl::handle_message( connection_ptr c, const handshake_message &msg) {
      fc_dlog(logger, "got a handshake_message from ${p}", ("p",c->peer_name()));
      if( c->connecting ) {
        c->connecting = false;

        if( msg.node_id == node_id) {
          elog( "Self connection detected. Closing connection");
          c->enqueue( go_away_message( go_away_reason::self ) );
          return;
        }

        if ( c->peer_addr.empty() || c->last_handshake.node_id == fc::sha256()) {
          fc_dlog(logger, "checking for duplicate" );
          for (const auto &check : connections) {
            if (check == c)
              continue;
            if (check->connected() &&
                check->peer_name() == msg.p2p_address) {
              fc_dlog(logger, "sending go_away duplicate to ${ep}", ("ep",msg.p2p_address) );
              go_away_message gam (go_away_reason::duplicate);
              gam.node_id = node_id;
              c->enqueue (gam);
              c->no_retry = go_away_reason::duplicate;
              return;
            }
          }
        }
        else {
          fc_dlog(logger, "skipping duplicate check, addr == ${pa}, id = ${ni}",("pa",c->peer_addr)("ni",c->last_handshake.node_id));
        }

        if( msg.chain_id != chain_id) {
          elog( "Peer on a different chain. Closing connection");
          c->enqueue( go_away_message(go_away_reason::wrong_chain) );
          return;
        }
        if( msg.network_version != network_version) {
          elog( "Peer network version does not match expected ${nv} but got ${mnv}",
                ( "nv", network_version)("mnv",msg.network_version));
          c->enqueue( go_away_message( go_away_reason::wrong_version ));
          return;
        }

        if(  c->node_id != msg.node_id) {
          c->node_id = msg.node_id;
        }

        if(!authenticate_peer(msg)) {
          dlog("Peer not authenticated.  Closing connection.");
          close(c);
          return;
        }
      }

      chain_controller& cc = chain_plug->chain();
      uint32_t lib_num = cc.last_irreversible_block_num( );
      uint32_t peer_lib = msg.last_irreversible_block_num;
      bool on_fork = false;
      fc_dlog(logger, "lib_num = ${ln} peer_lib = ${pl}",("ln",lib_num)("pl",peer_lib));

      if( peer_lib <= lib_num && peer_lib > 0) {
        try {
          block_id_type peer_lib_id =  cc.get_block_id_for_num( peer_lib);
          on_fork =( msg.last_irreversible_block_id != peer_lib_id);
        }
        catch( ...) {
          wlog( "caught an exception getting block id for ${pl}",("pl",peer_lib));
          on_fork = true;
        }
        if( on_fork) {
          elog( "Peer chain is forked");
          c->enqueue( go_away_message( go_away_reason::forked ));
          return;
        }

      }

      c->syncing = false;

      uint32_t head = cc.head_block_num( );
      block_id_type head_id = cc.head_block_id();

      if( peer_lib > head || sync_master->syncing() ) {
        sync_master->start_sync( c, peer_lib );
      }
      else if( msg.head_id != head_id ) {
        fc_dlog(logger, "msg.head_id = ${m} our head = ${h}",("m",msg.head_id)("h",head_id));

        notice_message note;
        note.known_blocks.mode = id_list_modes::none;
        fc_dlog(logger, "msg head = ${mh} msg lib = ${ml} my head = ${h} my lib = ${l}",("mh",msg.head_num)("ml",msg.last_irreversible_block_num)("h",head)("l",lib_num));
        if( msg.head_num >= lib_num ) {
          note.known_blocks.mode = id_list_modes::catch_up;
          note.known_blocks.pending = head - lib_num;
        } else {
          note.known_blocks.mode = id_list_modes::last_irr_catch_up;
          note.known_blocks.pending = lib_num;
        }
        note.known_trx.mode = id_list_modes::catch_up;
        note.known_trx.pending = local_txns.size(); // cc.pending().size();
        if( note.known_trx.pending > 0 || note.known_blocks.pending > 0) {
          fc_dlog(logger, "sending ${m} notice to ${n} about ${t} txns and ${b} blocks",("m",modes_str(note.known_blocks.mode))("n",c->peer_name())("t",note.known_trx.pending)("b",note.known_blocks.pending));
          c->enqueue( note );
          c->syncing = true;
        }
      }
      c->last_handshake = msg;
    }

  void net_plugin_impl::handle_message( connection_ptr c, const go_away_message &msg ) {
    string rsn = reason_str( msg.reason );
    c->no_retry = msg.reason;
    if (msg.reason == go_away_reason::duplicate ) {
      c->node_id = msg.node_id;
    }
  }

    void net_plugin_impl::handle_message (connection_ptr c, const time_message &msg) {
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
      notice_message fwd;
      request_message req;
      bool send_req = false;
      chain_controller &cc = chain_plug->chain();
      if ( msg.known_trx.pending > 0) {
        // plan to get all except what we already know about.
        req.req_trx.mode = id_list_modes::catch_up;
        send_req = true;
        size_t known_sum = local_txns.size();
        if ( known_sum ) {
          for( const auto& t : local_txns.get<by_id>( ) ) {
            req.req_trx.ids.push_back( t.id );
          }
        }
      }
      else {
        fwd.known_trx.mode = id_list_modes::normal;
        fwd.known_trx.pending = 0;
        req.req_trx.mode = id_list_modes::normal;
        req.req_trx.pending = 0;
        for( const auto& t : msg.known_trx.ids ) {
          const auto &tx = my_impl->local_txns.get<by_id>( ).find( t );
          if( tx == my_impl->local_txns.end( ) ) {
            c->trx_state.insert( ( transaction_state ){ t,true,true,( uint32_t ) - 1,
                fc::time_point( ),fc::time_point( ) } );
            if( !sync_master->syncing( ) ) {
              fwd.known_trx.ids.push_back( t );
            }
            req.req_trx.ids.push_back( t );
          }
        }
      }

      fc_dlog(logger,"this is a ${m} notice with ${n} blocks", ("m",modes_str(msg.known_blocks.mode))("n",msg.known_blocks.pending));


      if( msg.known_blocks.mode == id_list_modes::last_irr_catch_up ) {
        sync_master->start_sync (c, msg.known_blocks.pending);
      }
      else if( msg.known_blocks.mode == id_list_modes::catch_up ) {
          req.req_blocks.mode = id_list_modes::catch_up;
          req.req_blocks.pending = msg.known_blocks.pending;
          send_req = true;
        }
      else {
        req.req_blocks.mode = normal;
        for( const auto& blkid : msg.known_blocks.ids) {
          optional<signed_block> b;
          try {
            b = cc.fetch_block_by_id(blkid);
          } catch (const assert_exception &ex) {
            elog( "caught assert on fetch_block_by_id, ${ex}",("ex",ex.what()));
            // keep going, client can ask another peer
          } catch (...) {
            elog( "failed to retrieve block for id");
          }
          if (!b) {
            c->block_state.insert((block_state){blkid,false,true,fc::time_point::now()});
            fwd.known_blocks.ids.push_back(blkid);
            send_req = true;
            req.req_blocks.ids.push_back(blkid);
          }
        }
      }
      if( msg.known_trx.pending == 0 && (fwd.known_trx.ids.size() > 0 ||
                                         fwd.known_blocks.ids.size() > 0) ) {
        send_all( fwd, [c,fwd](connection_ptr cptr) -> bool {
            return cptr != c;
          });
      }
      fc_dlog(logger, "send req = ${sr}", ("sr",send_req));
      if ( send_req) {
        c->enqueue(req);
      }
    }

    void net_plugin_impl::handle_message( connection_ptr c, const request_message &msg) {
      switch (msg.req_blocks.mode) {
      case id_list_modes::catch_up :
        fc_dlog(logger, "got a catch_up request_message from ${p}", ("p",c->peer_name()));
        c->blk_send_branch( msg.req_trx.ids );
        break;
      case id_list_modes::normal :
        fc_dlog(logger, "got a normal request_message from ${p}", ("p",c->peer_name()));
        c->blk_send(msg.req_blocks.ids);
        break;
      default:;
      }


      switch (msg.req_trx.mode) {
      case id_list_modes::catch_up :
        c->txn_send_pending(msg.req_trx.ids);
        break;
      case id_list_modes::normal :
        c->txn_send(msg.req_trx.ids);
        break;
      case id_list_modes::none :
        if (msg.req_blocks.mode == id_list_modes::none)
          c->stop_send();
        break;
      default:;
      }

    }

    void net_plugin_impl::handle_message( connection_ptr c, const sync_request_message &msg) {
      fc_dlog(logger, "got a sync_request_message from ${p}", ("p",c->peer_name()));
      if( msg.end_block == 0) {
        c->sync_requested.reset();
      } else {
        c->sync_requested.reset(new sync_state( msg.start_block,msg.end_block,msg.start_block-1));
        c->enqueue_sync_block();
      }
    }

    void net_plugin_impl::handle_message( connection_ptr c, const block_summary_message &msg) {
      fc_dlog(logger, "got a block_summary_message from ${p}", ("p",c->peer_name()));
      fc_dlog(logger, "bsm header = ${h}",("h",msg.block_header));
      fc_dlog(logger, "txn count = ${c}", ("c",msg.trx_ids.size()));

      const auto& itr = c->block_state.get<by_id>();
      auto bs = itr.find(msg.block_header.id());
      if( bs == c->block_state.end()) {
        c->block_state.insert( (block_state){msg.block_header.id(),true,true,fc::time_point()});
        send_all( msg, [c](connection_ptr cptr) -> bool {
            return cptr != c;
          });
      } else {
        if( !bs->is_known) {
          c->block_state.modify (bs, make_known());
          send_all( msg, [c](connection_ptr cptr) -> bool {
              return cptr != c;
            });
        }
      }

      signed_block sb;
      bool fetch_error = false;
      chain_controller &cc = chain_plug->chain();
      if( !cc.is_known_block(msg.block_header.id()) ) {
        sb.previous = msg.block_header.previous;
        sb.timestamp = msg.block_header.timestamp;
        sb.transaction_merkle_root = msg.block_header.transaction_merkle_root;
        sb.producer_changes = msg.block_header.producer_changes;
        sb.producer = msg.block_header.producer;
        sb.producer_signature = msg.block_header.producer_signature;

        for( auto &cyc : msg.trx_ids ) {
          fc_dlog(logger, "cycle count = ${c}", ("c",cyc.size()));
          if( cyc.size() == 0 ) {
            continue;
          }
          sb.cycles.emplace_back( eosio::chain::cycle( ) );
          eosio::chain::cycle &sbcycle = sb.cycles.back( );
          for( auto &cyc_thr_id : cyc ) {
            fc_dlog(logger, "cycle user theads count = ${c}", ("c",cyc_thr_id.user_trx.size()));
            if (cyc_thr_id.user_trx.size() == 0) {
              continue;
            }
            sbcycle.emplace_back( eosio::chain::thread( ) );
            eosio::chain::thread &cyc_thr = sbcycle.back();
            /*
            for( auto &gt : cyc_thr_id.gen_trx ) {
              try {
                auto gen = cc.get_generated_transaction( gt );
                cyc_thr.generated_input.push_back( processed_generated_transaction( gen ) );
              } catch ( const exception &ex) {
                fetch_error = true;
                elog( "unable to retieve generated transaction, caught {ex}", ("ex",ex) );
                break;
              } catch ( ... ) {
                fetch_error = true;
                elog( "unable to retieve generated transaction" );
                break;
              }
            }
            */
            for( auto &ut : cyc_thr_id.user_trx ) {
              // auto ltxn = local_txns.get<by_id>().find(ut);

              try {
                processed_transaction pt(cc.get_recent_transaction(ut.id));
                pt.output = ut.outmsgs;
                cyc_thr.user_input.emplace_back(pt);

                fc_dlog(logger, "Found the transaction");
              } catch ( const exception &ex) {
                fetch_error = true;
                elog( "unable to retieve user transaction, caught {ex}", ("ex",ex) );
                break;
              } catch ( ... ) {
                fetch_error = true;
                elog( "unable to retieve user transaction" );
                break;
              }
            }
            if( fetch_error ){
              break;
            }
          }
          if( fetch_error ){
            break;
          }
        }

        try {
          fc_dlog(logger, "calling accept block, fetcherror = ${fe}",("fe",fetch_error));
          if( !fetch_error )
            chain_plug->accept_block( sb, false );
        } catch( const unlinkable_block_exception &ex) {
          elog( "caught unlinkable block exception #${n}",("n",sb.block_num()));
          c->enqueue( go_away_message( go_away_reason::unlinkable ));
        } catch( const assert_exception &ex) {
            // received a block due to out of sequence
          elog( "caught assertion on block #${n} ${ex}",
                ("n",sb.block_num())("ex",ex.what()));
        } catch( ... ) {
          elog( "unable to accept block, reason unknown" );
        }
      }
    }


    void net_plugin_impl::handle_message( connection_ptr c, const signed_transaction &msg) {
      fc_dlog(logger, "got a signed transaction from ${p}", ("p",c->peer_name()));
      transaction_id_type txnid = msg.id();
      auto entry = local_txns.get<by_id>().find( txnid );
      if (entry != local_txns.end( ) ) {
        if( entry->validated ) {
          fc_dlog(logger, "the txnid is known and validated, so short circuit" );
          return;
        }
      }

      try {
        //        chain_plug->chain().validate_transaction(msg);
      }
      catch ( const transaction_exception &ex ) {
        elog ("got a bad txn ${ex}", ("ex", ex.get_log()));
        c->enqueue( go_away_message( go_away_reason::bad_transaction) );
        return;
      }

      if (entry != local_txns.end( ) ) {
        local_txns.modify( entry, update_entry( msg ) );
      }
      else {
        cache_txn (txnid, msg);
      }

      auto tx = c->trx_state.find(txnid);
      if( tx == c->trx_state.end()) {
        c->trx_state.insert((transaction_state){txnid,true,true,(uint32_t)msg.ref_block_num,
              fc::time_point(),fc::time_point()});
      } else {
        struct trx_mod {
          uint16 block;
          trx_mod( uint16 bn) : block(bn) {}
          void operator( )( transaction_state &t) {
            t.is_known_by_peer = true;
            t.block_num = static_cast<uint32_t>(block);
          }
        };
        c->trx_state.modify(tx,trx_mod(msg.ref_block_num));
      }

      try {
        chain_plug->accept_transaction( msg );
        fc_dlog(logger, "chain accepted transaction" );
      } catch( const fc::exception &ex) {
        // received a block due to out of sequence
        elog( "accept txn threw  ${m}",("m",ex.what()));
      }
      catch( ...) {
        elog( " caught something attempting to accept transaction");
      }

    }

  void net_plugin_impl::handle_message( connection_ptr c, const signed_block &msg) {
    fc_dlog(logger, "got signed_block #${n} from ${p}", ("n",msg.block_num())("p",c->peer_name()));
    chain_controller &cc = chain_plug->chain();
    block_id_type blk_id = msg.id();
    try {
      if( cc.is_known_block(blk_id)) {
        return;
      }
    } catch( ...) {
    }
    if( cc.head_block_num() >= msg.block_num()) {
      elog( "received forking block #${n}",( "n",msg.block_num()));
    }
    fc::microseconds age( fc::time_point::now() - msg.timestamp);
    fc_dlog(logger, "got signed_block #${n} from ${p} block age in secs = ${age}",("n",msg.block_num())("p",c->peer_name())("age",age.to_seconds()));

    bool has_chunk = false;
    uint32_t num = msg.block_num();
    bool syncing = sync_master->syncing();
    if( syncing ) {
      has_chunk =( c->sync_receiving && c->sync_receiving->end_block > 0);

      if( !has_chunk) {
        if (c->sync_receiving)
          elog("got a block while syncing but sync_receiving end block == 0 #${n}",
             ( "n",num));
        else
          elog("got a block while syncing but no sync_receiving set #${n}",
             ( "n",num));
      }
      else {
        if( c->sync_receiving->last + 1 != num) {
          wlog( "expected block ${next} but got ${num}",("next",c->sync_receiving->last+1)("num",num));
          return;
        }
        c->sync_receiving->last = num;
      }
    }
    bool accepted = false;
    fc_dlog(logger, "last irreversible block = ${lib}", ("lib", cc.last_irreversible_block_num()));
    if( !syncing || num == cc.head_block_num()+1 ) {
      try {
        chain_plug->accept_block(msg, syncing);
        accepted = true;
      } catch( const unlinkable_block_exception &ex) {
        elog( "handle signed block: unlinkable_block_exception accept block #${n} syncing",("n",num));
        c->enqueue( go_away_message( go_away_reason::unlinkable ));
      } catch( const assert_exception &ex) {
        elog( "unable to accept block on assert exception ${n}",("n",ex.what()));
      } catch( const fc::exception &ex) {
        elog( "accept_block threw a non-assert exception ${x}",( "x",ex.what()));
      } catch( ...) {
        elog( "handle sync block caught something else");
      }
    }

    if( has_chunk) {
      if( !accepted) {
        c->sync_receiving->block_cache.emplace_back(std::move(c->blk_buffer));
      }

      if( num == c->sync_receiving->end_block) {
        sync_master->take_chunk( c);
      } else {
        c->sync_wait( );
      }
    }
    else {
      if (age < fc::seconds(3) && fc::raw::pack_size(msg) < just_send_it_max && !c->syncing ) {
        fc_dlog(logger, "forwarding the signed block");
        send_all( msg, [c, blk_id, num](connection_ptr conn) -> bool {
            bool sendit = false;
            if ( c != conn && !conn->syncing ) {
              auto b = conn->block_state.get<by_id>().find(blk_id);
              if (b == conn->block_state.end()) {
                conn->block_state.insert( (block_state){blk_id,true,true,fc::time_point()});
                sendit = true;
              } else if (!b->is_known) {
                conn->block_state.modify(b,make_known());
                sendit = true;
              }
            }
            fc_dlog(logger, "${action} block ${num} to ${c}",("action", sendit ? "sending " : "skipping ")("num",num)("c", conn->peer_name() ));
            return sendit;
          });
      }
    }
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

    void net_plugin_impl::start_txn_timer( ) {
      transaction_check->expires_from_now( txn_exp_period);
      transaction_check->async_wait( [&](boost::system::error_code ec) {
          if( !ec) {
            expire_txns( );
          }
          else {
            elog( "Error from connection check monitor: ${m}",( "m", ec.message()));
            start_txn_timer( );
          }
        });
    }

    void net_plugin_impl::ticker () {
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

    void net_plugin_impl::start_monitors( ) {
      connector_check.reset(new boost::asio::steady_timer( app().get_io_service()));
      transaction_check.reset(new boost::asio::steady_timer( app().get_io_service()));
      start_conn_timer();
      start_txn_timer();
    }

    void net_plugin_impl::expire_txns( ) {
      start_txn_timer( );
      auto &old = local_txns.get<by_expiry>();
      auto ex_up = old.upper_bound( time_point::now());
      auto ex_lo = old.lower_bound( fc::time_point_sec( 0));
      old.erase( ex_lo, ex_up);
      auto &stale = local_txns.get<by_block_num>();
      chain_controller &cc = chain_plug->chain();
      uint32_t bn = cc.last_irreversible_block_num();
      auto bn_up = stale.upper_bound(bn);
      auto bn_lo = stale.lower_bound(0);
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
          c.reset( );
        }
      }
    }

    void net_plugin_impl::close( connection_ptr c ) {
      if( c->peer_addr.empty( ) ) {
        --num_clients;
      }
      if( c->sync_receiving)
        sync_master->take_chunk( c);
      c->close();
    }


  size_t net_plugin_impl::cache_txn (const transaction_id_type txnid,
                                     const signed_transaction& txn ) {
      size_t packsiz = fc::raw::pack_size(txn);
      size_t bufsiz = packsiz + sizeof(packsiz);
      vector<char> buff(bufsiz);
      fc::datastream<char*> ds( buff.data(), bufsiz);
      ds.write( reinterpret_cast<char*>(&packsiz), sizeof(packsiz) );

      fc::raw::pack( ds, txn );

      uint16_t bn = static_cast<uint16_t>(txn.ref_block_num);
      node_transaction_state nts = {txnid,time_point::now(),
                                    txn.expiration,
                                    buff,
                                    bn, true};
      local_txns.insert(nts);
      return bufsiz;
    }

    void net_plugin_impl::send_all_txn( const signed_transaction& txn) {
      transaction_id_type txnid = txn.id();
      if( local_txns.get<by_id>().find( txnid ) != local_txns.end( ) ) { //found
        fc_dlog(logger, "found txnid in local_txns" );
        return;
      }

      size_t bufsiz = cache_txn (txnid, txn);
      fc_dlog(logger, "bufsiz = ${bs} max = ${max}",("bs", (uint32_t)bufsiz)("max", just_send_it_max));

      if( bufsiz <= just_send_it_max) {
        send_all( txn, [txn, txnid](connection_ptr c) -> bool {
            const auto& bs = c->trx_state.find(txnid);
            bool unknown = bs == c->trx_state.end();
            if( unknown) {
              c->trx_state.insert(transaction_state({txnid,true,true,(uint32_t)-1,
                      fc::time_point(),fc::time_point() }));
              fc_dlog(logger, "sending whole txn to ${n}", ("n",c->peer_name() ) );
            }
            return unknown;
          });
      }
      else {
        fc_dlog(logger, "pending_notify, mode = ${m}, pending count = ${p}",("m",modes_str(pending_notify.mode))("p",pending_notify.pending));
        pending_notify.ids.push_back( txnid );
        notice_message nm = { pending_notify, ordered_blk_ids( ) };
        send_all( nm, [txn, txnid](connection_ptr c) -> bool {
            const auto& bs = c->trx_state.find(txnid);
            bool unknown = bs == c->trx_state.end();
            if( unknown) {
              fc_dlog(logger, "sending notice to ${n}", ("n",c->peer_name() ) );
              c->trx_state.insert(transaction_state({txnid,false,true,(uint32_t)-1,
                      fc::time_point(),fc::time_point() }));
            }
            return unknown;
          });
        pending_notify.ids.clear();
      }
    }

    /**
     * This one is necessary to hook into the boost notifier api
     **/
    void net_plugin_impl::transaction_ready( const signed_transaction& txn) {
      my_impl->send_all_txn( txn );
    }

    void net_plugin_impl::broadcast_block_impl( const chain::signed_block &sb) {
      if( send_whole_blocks) {
        send_all( sb,[](connection_ptr c) -> bool { return true; });
        return;
      }

      block_summary_message bsm = {sb, vector<cycle_ids>()};
      vector<cycle_ids> &trxs = bsm.trx_ids;
      if( !sb.cycles.empty()) {
        for( const auto& cyc : sb.cycles) {
          fc_dlog(logger, "cyc.size = ${cs}",( "cs", cyc.size()));
          if( cyc.empty() ) {
            continue;
          }
          trxs.emplace_back (cycle_ids());
          cycle_ids &cycs = trxs.back();
          fc_dlog(logger, "trxs.size = ${ts} cycles.size = ${cs}",("ts", trxs.size())("cs", cycs.size()));
          for( const auto& thr : cyc) {
            fc_dlog(logger, "user txns = ${ui} generated = ${gi}",("ui",thr.user_input.size( ))("gi",thr.generated_input.size( )));
            if( thr.user_input.size( ) == 0 ) {
              continue;
            }
            cycs.emplace_back (thread_ids());
            thread_ids &thd_ids = cycs.back();

            for( auto gi : thr.generated_input ) {
              thd_ids.gen_trx.emplace_back( gi.id );
            }

            for( auto &ui : thr.user_input) {
              processed_trans_summary pts ({ui.id( ),ui.output });
              thd_ids.user_trx.emplace_back( pts );
              fc_dlog(logger, "user txn has ${m} messages, summary has ${ms}", ("m",ui.output.size())("ms",pts.outmsgs.size()));
            }
          }
        }
      }

      fc_dlog(logger, "sending bsm with ${c} transactions",("c",trxs.size()));
      fc_dlog(logger, "bsm header = ${h} txns = ${t}",("h",bsm.block_header)("t",bsm.trx_ids.size()));
      if (bsm.trx_ids.size() > 0) {
        fc_dlog(logger, "cycles.size = ${cs}",("cs", bsm.trx_ids[0].size()));
        if (bsm.trx_ids[0].size()) {
        }
      }
      send_all( bsm,[sb](connection_ptr c) -> bool {
          const auto& bs = c->block_state.find(sb.id());
          if( bs == c->block_state.end()) {
            c->block_state.insert( (block_state){sb.id(),true,true,fc::time_point()});
            return true;
          }
          return false;
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

      if(msg.sig != ecc::compact_signature() && msg.token != sha256()) {
        sha256 hash = fc::sha256::hash(msg.time);
        if(hash != msg.token) {
          elog( "Peer ${peer} sent a handshake with an invalid token.",
                ("peer", msg.p2p_address));
          return false;
        }
        types::public_key peer_key;
        try {
          peer_key = ecc::public_key(msg.sig, msg.token, true);
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
      producer_plugin* pp = app().find_plugin<producer_plugin>();
      if(pp != nullptr)
        return pp->first_producer_public_key();
      return chain::public_key_type();
    }

    fc::ecc::compact_signature net_plugin_impl::sign_compact(const chain::public_key_type& signer, const fc::sha256& digest) const
    {
      auto private_key_itr = private_keys.find(signer);
      if(private_key_itr != private_keys.end())
        return private_key_itr->second.sign_compact(digest);
      producer_plugin* pp = app().find_plugin<producer_plugin>();
      if(pp != nullptr)
        return pp->sign_compact(signer, digest);
      return ecc::compact_signature();
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
    if(hello.sig == ecc::compact_signature())
      hello.token = sha256();
    hello.p2p_address = my_impl->p2p_address;
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
    if ( hello.last_irreversible_block_num ) {
      try {
        hello.last_irreversible_block_id = cc.get_block_id_for_num
          ( hello.last_irreversible_block_num);
      }
      catch( const unknown_block_exception &ex) {
        hello.last_irreversible_block_num = 0;
      }
    }
    if ( hello.head_num ) {
      try {
        hello.head_id = cc.get_block_id_for_num ( hello.head_num );
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
     ( "listen-endpoint", bpo::value<string>()->default_value( "0.0.0.0:9876" ), "The actual host:port used to listen for incoming p2p connections.")
     ( "public-endpoint", bpo::value<string>(), "An externally accessible host:port for identifying this node. Defaults to listen-endpoint.")
     ( "remote-endpoint", bpo::value< vector<string> >()->composing(), "The public endpoint of a peer node to connect to. Use multiple remote-endpoint options as needed to compose a network.")
     ( "agent-name", bpo::value<string>()->default_value("\"EOS Test Agent\""), "The name supplied to identify this node amongst the peers.")
     ( "send-whole-blocks", bpo::value<bool>()->default_value(def_send_whole_blocks), "True to always send full blocks, false to send block summaries" )
     ( "allowed-connection", bpo::value<vector<string>>()->multitoken()->default_value({"none"}, "none"), "Can be 'any' or 'producers' or 'specified' or 'none'. If 'specified', peer-key must be specified at least once. If only 'producers', peer-key is not required. 'producers' and 'specified' may be combined.")
     ( "peer-key", bpo::value<vector<string>>()->composing()->multitoken(), "Optional public key of peer allowed to connect.  May be used multiple times.")
     ( "peer-private-key", boost::program_options::value<vector<string>>()->composing()->multitoken(),
       "Tuple of [PublicKey, WIF private key] (may specify multiple times)")
     ( "log-level-net-plugin", bpo::value<string>()->default_value("info"), "Log level: one of 'all', 'debug', 'info', 'warn', 'error', or 'off'")
      ( "max-clients", bpo::value<int>()->default_value(def_max_clients), "Maximum number of clients from which connections are accepted, use 0 for no limit")
      ( "connection-cleanup-period", bpo::value<int>()->default_value(def_conn_retry_wait), "number of seconds to wait before cleaning up dead connections")
     ;
  }

  template<typename T>
  T dejsonify(const string& s) {
     return fc::json::from_string(s).as<T>();
  }

  void net_plugin::plugin_initialize( const variables_map& options ) {
    ilog("Initialize net plugin");

    // Housekeeping so fc::logger::get() will work as expected
    fc::get_logger_map()[connection::logger_name] = connection::logger;
    fc::get_logger_map()[net_plugin_impl::logger_name] = net_plugin_impl::logger;
    fc::get_logger_map()[sync_manager::logger_name] = sync_manager::logger;

    // Setting a parent would in theory get us the default appenders for free but
    // a) the parent's log level overrides our own in that case; and
    // b) fc library's logger was never finished - the _additivity flag tested is never true.
    for(fc::shared_ptr<fc::appender>& appender : fc::logger::get().get_appenders()) {
      connection::logger.add_appender(appender);
      net_plugin_impl::logger.add_appender(appender);
      sync_manager::logger.add_appender(appender);
    }

    if( options.count( "log-level-net-plugin" ) ) {
      fc::log_level logl;

      fc::from_variant(options.at("log-level-net-plugin").as<string>(), logl);
      ilog("Setting net_plugin logging level to ${level}", ("level", logl));
      connection::logger.set_log_level(logl);
      net_plugin_impl::logger.set_log_level(logl);
      sync_manager::logger.set_log_level(logl);
    }

    my->network_version = (uint16_t)app().version_int();
    my->send_whole_blocks = def_send_whole_blocks;

    my->sync_master.reset( new sync_manager( def_sync_rec_span ) );

    my->connector_period = std::chrono::seconds(options.at("connection-cleanup-period").as<int>());
    my->txn_exp_period = def_txn_expire_wait;
    my->resp_expected_period = def_resp_expected_wait;
    my->just_send_it_max = def_max_just_send;
    my->max_client_count = options.at("max-clients").as<int>();

    my->max_client_count = def_max_clients;
    my->num_clients = 0;
    my->started_sessions = 0;

    my->resolver = std::make_shared<tcp::resolver>( std::ref( app().get_io_service() ) );
    if(options.count("listen-endpoint")) {
      my->p2p_address = options.at("listen-endpoint").as< string >();
      auto host = my->p2p_address.substr( 0, my->p2p_address.find(':') );
      auto port = my->p2p_address.substr( host.size()+1, my->p2p_address.size() );
      idump((host)(port));
      tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
      // Note: need to add support for IPv6 too?

      my->listen_endpoint = *my->resolver->resolve( query);

      my->acceptor.reset( new tcp::acceptor( app().get_io_service() ) );
    }
    if(options.count("public-endpoint")) {
      my->p2p_address = options.at("public-endpoint").as< string >();
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

    if(options.count("remote-endpoint")) {
      my->supplied_peers = options.at("remote-endpoint").as<vector<string> >();
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
         fc::optional<fc::ecc::private_key> private_key = eosio::utilities::wif_to_key(key_id_to_wif_pair.second);
         FC_ASSERT(private_key, "Invalid WIF-format private key ${key_string}",
                   ("key_string", key_id_to_wif_pair.second));
         my->private_keys[key_id_to_wif_pair.first] = *private_key;
      }
    }

    if( options.count( "send-whole-blocks")) {
      my->send_whole_blocks = options.at( "send-whole-blocks" ).as<bool>();
    }

    my->chain_plug = app().find_plugin<chain_plugin>();
    my->chain_plug->get_chain_id(my->chain_id);
    fc::rand_pseudo_bytes(my->node_id.data(), my->node_id.data_size());
    ilog ("my node_id is ${id}",("id",my->node_id));

    my->keepalive_timer.reset(new boost::asio::steady_timer (app().get_io_service()));
    my->ticker();
    my->pending_notify.mode = id_list_modes::normal;
    my->pending_notify.pending = 0;

  }

  void net_plugin::plugin_startup() {
    if( my->acceptor ) {
      my->acceptor->open(my->listen_endpoint.protocol());
      my->acceptor->set_option(tcp::acceptor::reuse_address(true));
      my->acceptor->bind(my->listen_endpoint);
      my->acceptor->listen();
      my->start_listen_loop();
    }

    my->chain_plug->chain().on_pending_transaction.connect( &net_plugin_impl::transaction_ready);
    my->start_monitors();

    for( auto seed_node : my->supplied_peers ) {
      connection_ptr c = std::make_shared<connection>(seed_node);
      my->connections.insert( c);
      my->connect( c );
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
          con->socket->close();
          my->close( con);
        }

        my->acceptor.reset(nullptr);
      }
      ilog( "exit shutdown" );
    } FC_CAPTURE_AND_RETHROW() }

    void net_plugin::broadcast_block( const chain::signed_block &sb) {
      fc_dlog(my->logger, "broadcasting block #${num}",("num",sb.block_num()) );
      my->broadcast_block_impl( sb);
    }

  size_t net_plugin::num_peers( ) const {
    return my->count_open_sockets();
  }

}
