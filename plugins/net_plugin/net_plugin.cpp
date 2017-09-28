
#include <eos/chain/types.hpp>

#include <eos/net_plugin/net_plugin.hpp>
#include <eos/net_plugin/protocol.hpp>
#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/block.hpp>

#include <fc/network/ip.hpp>
#include <fc/io/raw.hpp>
#include <fc/container/flat.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/exception/exception.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/steady_timer.hpp>

namespace eos {
  using std::vector;

  using boost::asio::ip::tcp;
  using boost::asio::ip::address_v4;
  using boost::asio::ip::host_name;

  using fc::time_point;
  using fc::time_point_sec;
  using eos::chain::transaction_id_type;
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
    // vector<char>        packed_transaction; //just for the moment
    uint32_t            block_num = -1; /// block transaction was included in
    bool                validated = false; /// whether or not our node has validated it
  };

  struct update_block_num {
    UInt16 new_bnum;
    update_block_num (UInt16 bnum) : new_bnum(bnum) {}
    void operator() (node_transaction_state& nts) {
      nts.block_num = static_cast<uint32_t>(new_bnum);
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
    unique_ptr<tcp::acceptor>     acceptor;
    tcp::endpoint                 listen_endpoint;
    string                        p2p_address;
    uint32_t                      max_client_count;
    uint32_t                      num_clients;

    vector<string>                supplied_peers;

    std::set< connection_ptr >    connections;
    bool                          done = false;
    unique_ptr< sync_manager >    sync_master;

    unique_ptr<boost::asio::steady_timer> connector_check;
    unique_ptr<boost::asio::steady_timer> transaction_check;
    unique_ptr<boost::asio::steady_timer> keepalive_timer;
    boost::asio::steady_timer::duration   connector_period;
    boost::asio::steady_timer::duration   txn_exp_period;
    boost::asio::steady_timer::duration   resp_expected_period;
    boost::asio::steady_timer::duration   keepalive_interval{std::chrono::seconds{32}};

    int16_t                       network_version;
    chain_id_type                 chain_id;
    fc::sha256                    node_id;

    string                        user_agent_name;
    chain_plugin*                 chain_plug;
    size_t                        just_send_it_max;
    bool                          send_whole_blocks;

    node_transaction_index        local_txns;
    vector<transaction_id_type>   pending_notify;

    shared_ptr<tcp::resolver>     resolver;

    void connect( connection_ptr c );
    void connect( connection_ptr c, tcp::resolver::iterator endpoint_itr );
    void start_session( connection_ptr c );
    void start_listen_loop( );
    void start_read_message( connection_ptr c);
    void start_reading_pending_buffer( connection_ptr c);

    void close( connection_ptr c );

    template<typename VerifierFunc>
    void send_all (const net_message &msg, VerifierFunc verify);
    //    template<typename VerifierFunc>
    //    void send_all (net_message_ptr msg, VerifierFunc verify);
    void send_all_txn (const SignedTransaction& txn);
    static void transaction_ready( const SignedTransaction& txn);
    void broadcast_block_impl( const signed_block &sb);

    void handle_message( connection_ptr c, const handshake_message &msg);
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
    void handle_message( connection_ptr c, const SignedTransaction &msg);
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
  };

  static net_plugin_impl *my_impl;

  /**
   * default value initializers
   */
  constexpr auto     def_buffer_size_mb = 4;
  constexpr auto     def_buffer_size = 1024*1024*def_buffer_size_mb;
  constexpr auto     def_max_clients = 20; // 0 for unlimited clients
  constexpr auto     def_conn_retry_wait = std::chrono::seconds (30);
  constexpr auto     def_txn_expire_wait = std::chrono::seconds (3);
  constexpr auto     def_resp_expected_wait = std::chrono::seconds (1);
  constexpr auto     def_network_version = 0;
  constexpr auto     def_sync_rec_span = 10;
  constexpr auto     def_max_just_send = 1300 * 3; // "mtu" * 3
  constexpr auto     def_send_whole_blocks = true;

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

  /**
   * Index by start_block
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
    deque< vector<char> > block_cache;
  };

  using sync_state_ptr = shared_ptr< sync_state >;

  struct handshake_initializer {
    static void populate (handshake_message &hello);
  };

  class connection : public std::enable_shared_from_this<connection> {
  public:
    connection( string endpoint,
                size_t send_buf_size = def_buffer_size,
                size_t recv_buf_size = def_buffer_size );

    connection( socket_ptr s,
                size_t send_buf_size = def_buffer_size,
                size_t recv_buf_size = def_buffer_size );
    ~connection();
    void initialize ();

    block_state_index       block_state;
    transaction_state_index trx_state;
    sync_state_ptr          sync_receiving;  // we are requesting info from this peer
    sync_state_ptr          sync_requested;  // this peer is requesting info from us
    socket_ptr              socket;

    uint32_t                pending_message_size;
    vector<char>            pending_message_buffer;
    uint32_t                send_message_size;
    vector<char>            send_buffer;
    vector<char>            blk_buffer;
    size_t                  message_size;

    fc::sha256              remote_node_id;
    handshake_message       last_handshake;
    deque<net_message>      out_queue;
    bool                    connecting;
    bool                    syncing;
    string                  peer_addr;
    unique_ptr<boost::asio::steady_timer> response_expected;

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

    void enqueue( const net_message &msg );
    void enqueue_sync_block ();

    void send_next_message();

    void sync_wait ();
    void sync_timeout (boost::system::error_code ec);
    void fetch_timeout (boost::system::error_code ec);
  };


  class sync_manager {
    uint32_t            sync_head;
    uint32_t            sync_req_head;
    uint32_t            sync_req_span;

    deque< sync_state_ptr > full_chunks;
    deque< sync_state_ptr > partial_chunks;
    chain_plugin * chain_plug;

  public:
    sync_manager (uint32_t span);
    bool syncing ();
    void assign_chunk (connection_ptr c);
    void apply_chunk (sync_state_ptr ss);
    void take_chunk (connection_ptr c);
    void start_sync (connection_ptr c, uint32_t target);

  };

  //---------------------------------------------------------------------------

  connection::connection( string endpoint,
                          size_t send_buf_size,
                          size_t recv_buf_size )
      : block_state(),
        trx_state(),
        sync_receiving(),
        sync_requested(),
        socket( std::make_shared<tcp::socket>( std::ref( app().get_io_service() ))),
        pending_message_size(0),
        pending_message_buffer(recv_buf_size),
        send_buffer(send_buf_size),
        remote_node_id(),
        last_handshake(),
        out_queue(),
        connecting (false),
        syncing (false),
        peer_addr (endpoint),
        response_expected ()
    {
      wlog( "created connection to ${n}", ("n", endpoint) );
      initialize();
    }

  connection::connection( socket_ptr s,
                          size_t send_buf_size,
                          size_t recv_buf_size )
      : block_state(),
        trx_state(),
        sync_receiving(),
        sync_requested(),
        socket( s ),
        pending_message_size(0),
        pending_message_buffer(recv_buf_size),
        send_buffer(send_buf_size),
        remote_node_id(),
        last_handshake(),
        out_queue(),
        connecting (false),
        syncing (false),
        peer_addr (),
        response_expected ()
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
      auto *rnd = remote_node_id.data();
      rnd[0] = 0;
      response_expected.reset(new boost::asio::steady_timer (app().get_io_service()));
    }

  bool connection::connected () {
    return (socket->is_open() && !connecting);
  }

  bool connection::current () {
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

    void connection::send_handshake ( ) {
      handshake_message hello;
      handshake_initializer::populate(hello);
      enqueue (hello);
    }

  char* connection::convert_tstamp(const tstamp& t)
  {
    const long NsecPerSec{1000000000};
    time_t seconds = t / NsecPerSec;
    strftime(ts, ts_buffer_size, "%F %T", localtime(&seconds));
    snprintf(ts+19, ts_buffer_size-19, ".%ld", t % NsecPerSec);
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

  void connection::send_next_message() {
      if( !out_queue.size() ) {
        if (sync_requested ) {
          enqueue_sync_block();
        }
        return;
      }

      auto& m = out_queue.front();
      if (m.contains<sync_request_message>()) {
        sync_wait( );
      }
      send_message_size = fc::raw::pack_size( m );
      fc::datastream<char*> ds( send_buffer.data(), send_message_size + sizeof(send_message_size) );
      ds.write( (char*)&send_message_size, sizeof(send_message_size) );
      fc::raw::pack( ds, m );

      boost::asio::async_write( *socket, boost::asio::buffer( send_buffer, send_message_size + sizeof(send_message_size) ),
                   [this]( boost::system::error_code ec, std::size_t /*bytes_transferred*/ ) {
                     if( ec ) {
                       elog( "Error sending message: ${msg}", ("msg",ec.message() ) );
                     } else  {
                       if (!out_queue.size()) {
                         elog ("out_queue underflow!");
                       } else {
                         out_queue.pop_front();
                       }
                       send_next_message();
                     }
                   });
    }

  void connection::enqueue_sync_block ( ) {
    chain_controller& cc = app().find_plugin<chain_plugin>()->chain();
    uint32_t num = ++sync_requested->last;

    ilog ("num = ${num} end = ${end}",("num",num)("end",sync_requested->end_block));
    if (num == sync_requested->end_block) {
      sync_requested.reset();
    }
    try {
      fc::optional<signed_block> sb = cc.fetch_block_by_number(num);
      if (sb) {
        enqueue( *sb );
      }
    } catch ( ... ) {
      wlog( "write loop exception" );
    }
  }

  void connection::sync_wait( ) {
    response_expected->expires_from_now( my_impl->resp_expected_period);
    response_expected->async_wait( boost::bind(&connection::sync_timeout,
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

  void connection::fetch_timeout( boost::system::error_code ec ) {
    if( !ec ) {
      dlog ("fetch timeout occurred");
    }
    else if( ec == boost::asio::error::operation_aborted) {
      if( !connected()) {
        dlog ("fetch timeout was cancelled due to dead connection");
      }
    }
  }
  //-----------------------------------------------------------

  sync_manager::sync_manager (uint32_t span)
    :sync_head (0)
    ,sync_req_head (0)
    ,sync_req_span (span)
  {
    chain_plug = app().find_plugin<chain_plugin>();
  }

  bool sync_manager::syncing () {
    return (sync_req_head != sync_head ||
            chain_plug->chain().head_block_num() < sync_req_head);
  }

  void sync_manager::assign_chunk (connection_ptr c) {
    uint32_t start = 0;
    uint32_t end = 0;

    if( !partial_chunks.empty() ) {
      c->sync_receiving = partial_chunks.front();
      partial_chunks.pop_front();
      start = c->sync_receiving->last + 1;
      end = c->sync_receiving->end_block;
    }
    else if( sync_req_head != sync_head ) {
      start = sync_req_head + 1;
      end = (start + sync_req_span -1);
      if (end > sync_head)
        end = sync_head;
      if( end > 0 && end >= start) {
        c->sync_receiving.reset(new sync_state (start, end, sync_req_head));
      }
    }
    else {
      c->sync_receiving.reset();
    }
    if (end > 0 && end >= start) {
      c->enqueue((sync_request_message){start, end});
      sync_req_head = end;
    }
  }

    struct postcache : public fc::visitor<void> {
      chain_plugin * chain_plug;
      postcache (chain_plugin *cp) : chain_plug (cp) {}

      void operator()(const signed_block &block) const
      {
        try {
          chain_plug->accept_block (block,true);
        } catch (const unlinkable_block_exception &ex) {
          elog ("unlinkable_block_exception accept block #${n}",("n",block.block_num()));
          //close (c);
        } catch (const assert_exception &ex) {
          elog ("unable to accept block on assert exception ${n}",("n",ex.what()));
        } catch (const fc::exception &ex) {
          elog ("accept_block threw a non-assert exception ${x}", ("x",ex.what()));
        } catch (...) {
          elog ("unkown error acceping cached block");
        }
      }

      template <typename T> void operator()(const T &msg) const { /* no-op */ }
    };

    void sync_manager::apply_chunk (sync_state_ptr ss) {
      postcache pc(chain_plug);
      for (auto & blk : ss->block_cache) {
        auto block = fc::raw::unpack<net_message>( blk );
        block.visit (pc);
      }
    }

    void sync_manager::take_chunk (connection_ptr c) {
      if (!c->sync_receiving) {
        elog ("take_chunk called, but sync_receiving is empty");
        return;
      }
      sync_state_ptr ss;
      c->sync_receiving.swap(ss);
      if (!ss->block_cache.empty()) {
        if (ss->last < ss->end_block) {
          partial_chunks.push_back( ss );
          return;
        }

        if (ss->start_block != chain_plug->chain().last_irreversible_block_num() + 1) {
          bool found = false;
          for (auto pos = full_chunks.begin(); !found && pos != full_chunks.end(); ++pos) {
            if (ss->end_block < (*pos)->start_block) {
              full_chunks.insert (pos,ss);
              found = true;
            }
          }
          if (!found) {
            full_chunks.push_back (ss); //either full chunks is empty or pos ran off the end
          }
        }
        else {
          apply_chunk (ss);
        }
      }
      while (!full_chunks.empty()) {
        auto chunk = full_chunks.front();
        if (chunk->start_block == chain_plug->chain().head_block_num() + 1) {
          apply_chunk (chunk);
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

      if ( chain_plug->chain().head_block_num() == sync_head ) {
        handshake_message hello;
        handshake_initializer::populate(hello);
        my_impl->send_all (hello, [](connection_ptr /* no arg */) -> bool  {
          return true;
        });
    }
      if (c->connected()) {
        assign_chunk (c);
      }
    }

    void sync_manager::start_sync (connection_ptr c, uint32_t target) {
      ilog ("Catching up with chain, our head is ${cc}, theirs is ${t}",
            ("cc",sync_req_head)("t",target));
      if (!syncing()) {
        sync_req_head = chain_plug->chain().head_block_num();
      }
      if (target > sync_head) {
        sync_head = target;
      }
      if (c->sync_receiving && c->sync_receiving->end_block > 0) {
        return;
      }
      assign_chunk (c);
    }

  //------------------------------------------------------------------------

  void net_plugin_impl::connect( connection_ptr c ) {
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
                                         ( "peer_addr", c->peer_addr )("error", err.message() ) );
                                 }
                               });
    }

    void net_plugin_impl::connect( connection_ptr c, tcp::resolver::iterator endpoint_itr ) {
      auto current_endpoint = *endpoint_itr;
      ++endpoint_itr;
      c->connecting = true;
      c->socket->async_connect( current_endpoint,
                           [c, endpoint_itr, this]
                           ( const boost::system::error_code& err ) {
                             if( !err ) {
                               start_session( c );
                             } else {
                               if( endpoint_itr != tcp::resolver::iterator() ) {
                                 c->close();
                                 connect( c, endpoint_itr );
                               }
                               else {
                                 elog ("connection failed to ${peer}: ${error}",
                                       ("peer", c->peer_addr)("error",err.message()));
                                 c->connecting = false;
                                 c->close();
                               }
                             }
                           } );
    }

    void net_plugin_impl::start_session( connection_ptr con ) {
      boost::asio::ip::tcp::no_delay option( true );
      con->socket->set_option( option );
      start_read_message( con );
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
                    ("m", max_client_count) );
              socket->close( );
            }
            start_listen_loop();
          } else {
            elog( "Error accepting connection: ${m}", ("m", ec.message() ) );
          }
        });
    }

    void net_plugin_impl::start_read_message( connection_ptr conn ) {
      conn->pending_message_size = 0;
      connection_wptr c (conn);
      uint32_t *buff = &(conn.get()->pending_message_size);

      boost::asio::async_read( *conn->socket,
                               boost::asio::buffer((char *)buff, sizeof(conn->pending_message_size)),
                               [this,c]( boost::system::error_code ec, std::size_t /*bytes_transferred*/ ) {
                                 //ilog( "read size handler..." );
                                 if( !ec ) {
                                   connection_ptr conn = c.lock();
                                   if( conn->pending_message_size <= conn->pending_message_buffer.size() ) {
                                     start_reading_pending_buffer( conn );
                                     return;
                                   } else {
                                     elog( "Received a message that was too big" );
                                   }
                                 } else {
                                   elog( "Error reading message from connection: ${m}", ("m", ec.message() ) );
                                 }
                                 close( c.lock() );
                               }
                               );
    }

    template<typename VerifierFunc>
    void net_plugin_impl::send_all( const net_message &msg, VerifierFunc verify) {
      for (auto &c : connections) {
        if (c->current() && verify (c)) {
          c->enqueue( msg );
        }
      }
    }

    void net_plugin_impl::handle_message (connection_ptr c, const handshake_message &msg) {
      c->connecting = false;

      if (msg.node_id == node_id) {
        elog ("Self connection detected. Closing connection");
        close( c );
        return;
      }
      if (msg.chain_id != chain_id) {
        elog ("Peer on a different chain. Closing connection");
        close (c);
        return;
      }
      if (msg.network_version != network_version) {
        elog ("Peer network version does not match expected ${nv} but got ${mnv}",
              ("nv", network_version)("mnv",msg.network_version));
        close (c);
        return;
      }

      chain_controller& cc = chain_plug->chain();
      uint32_t lib_num = cc.last_irreversible_block_num ();
      uint32_t peer_lib = msg.last_irreversible_block_num;
      bool on_fork = false;
      if (peer_lib <= lib_num && peer_lib > 0) {
        try {
          block_id_type peer_lib_id =  cc.get_block_id_for_num (peer_lib);
          on_fork = (msg.last_irreversible_block_id != peer_lib_id);
        }
        catch (...) {
          wlog ("caught an exception getting block id for ${pl}",("pl",peer_lib));
          on_fork = true;
        }
        if (on_fork) {
          elog ("Peer chain is forked");
          close (c);
          return;
        }
      }

      if ( c->remote_node_id != msg.node_id) {
        //        c->reset();
        c->remote_node_id = msg.node_id;
      }

      c->syncing = false;

      uint32_t head = cc.head_block_num ();
      if ( msg.last_irreversible_block_num  >  head || sync_master->syncing() ) {
        dlog("calling start_sync, myhead = ${h} their last_irreversable = ${mhn}",
             ("h",head)("mhn",peer_lib));
        sync_master->start_sync (c, peer_lib);
      }
      else if( msg.head_num < cc.head_block_num()) {
        dlog("peer doesn't need to sync but does need to fetch blocks/trans");
        if( msg.head_num >= lib_num ) {
#warning ("TODO: construct notice message for blocks and transactions");
        }
        c->syncing = true;
      }


      c->last_handshake = msg;
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
      dlog("Clock offset is ${o}ns (${us}us)", ("o", c->offset)("us", c->offset/NsecPerUsec));
      c->org = 0;
      c->rec = 0;
    }

    void net_plugin_impl::handle_message (connection_ptr c, const notice_message &msg) {
      //peer tells us about one or more blocks or txns. We need to forward only those
      //we don't already know about. and for each peer note that it knows
      notice_message fwd;
      request_message req;

      for (const auto& t : msg.known_trx) {
        const auto &tx = c->trx_state.find(t);
        if (tx == c->trx_state.end()) {
          c->trx_state.insert((transaction_state){t,true,true,(uint32_t)-1,
                fc::time_point(),fc::time_point()});
          fwd.known_trx.push_back(t);
          req.req_trx.push_back(t);
        }
      }
      if (fwd.known_trx.size() > 0) {
        send_all (fwd, [c,fwd](connection_ptr cptr) -> bool {
            return cptr != c;
          });
        c->enqueue(req);
      }
    }

    void net_plugin_impl::handle_message (connection_ptr c, const request_message &msg) {
        // collect a list of transactions that were found.
        // collect a second list of transaction ids that were not found but are otherwise known by some peers
        // finally, what remains are future(?) transactions

      vector< SignedTransaction > send_now;
      map <connection_ptr, vector < transaction_id_type > > forward_to;
      auto conn_ndx = connections.begin();
      for (auto t: msg.req_trx) {
        auto txn = local_txns.get<by_id>().find(t);
        if (txn != local_txns.end()) {
          chain_controller &cc = chain_plug->chain();
          try {
            send_now.push_back(cc.get_recent_transaction(t));
          } catch (...) {
            elog( "failed to retrieve transaction");
          }
        }
        else {
          int cycle_count = 2;
          auto loop_start = conn_ndx++;
          while (conn_ndx != loop_start) {
            if (conn_ndx == connections.end()) {
              if (--cycle_count == 0) {
                elog("loop cycled twice, something is wrong");
                break;
              }
              conn_ndx = connections.begin();
              continue;
            }
            if (conn_ndx->get() == c.get()) {
              ++conn_ndx;
              continue;
            }
            auto txn = conn_ndx->get()->trx_state.get<by_id>().find(t);
            if (txn != conn_ndx->get()->trx_state.end()) {

              //forward_to[conn_ndx]->push_back(t);
              break;
            }
            ++conn_ndx;
          }
        }
      }

      if (!send_now.empty()) {
        for (auto &t : send_now) {
          c->enqueue (t);
        }
      }
    }

    void net_plugin_impl::handle_message (connection_ptr c, const sync_request_message &msg) {
      if (msg.end_block == 0) {
        c->sync_requested.reset();
      } else {
        c->sync_requested.reset(new sync_state (msg.start_block,msg.end_block,msg.start_block-1));
        c->enqueue_sync_block();
      }
    }

    void net_plugin_impl::handle_message (connection_ptr c, const block_summary_message &msg) {
      const auto& itr = c->block_state.get<by_id>();
      auto bs = itr.find(msg.block.id());
      if (bs == c->block_state.end()) {
        c->block_state.insert ((block_state){msg.block.id(),true,true,fc::time_point()});
        send_all (msg, [c](connection_ptr cptr) -> bool {
            return cptr != c;
          });
      } else {
        if (!bs->is_known) {
          block_state value = *bs;
          value.is_known= true;
          c->block_state.insert (std::move(value));
          send_all (msg, [c](connection_ptr cptr) -> bool {
              return cptr != c;
            });
        }
      }
#warning ("TODO: reconstruct actual block from cached transactions")
      signed_block sb;
      chain_controller &cc = chain_plug->chain();
      if (!cc.is_known_block(msg.block.id()) ) {
        try {
          chain_plug->accept_block(sb, false);
        } catch (const unlinkable_block_exception &ex) {
          elog ("   caught unlinkable block exception #${n}",("n",sb.block_num()));
           close (c);
        } catch (const assert_exception &ex) {
          // received a block due to out of sequence
          elog ("  caught assertion #${n}",("n",sb.block_num()));
        }
      }
    }

    void net_plugin_impl::handle_message (connection_ptr c, const SignedTransaction &msg) {
      transaction_id_type txnid = msg.id();
      if( local_txns.get<by_id>().find( txnid ) != local_txns.end () ) { //found
        return;
      }

      auto tx = c->trx_state.find(txnid);
      if (tx == c->trx_state.end()) {
        c->trx_state.insert((transaction_state){txnid,true,true,(uint32_t)msg.refBlockNum,
              fc::time_point(),fc::time_point()});
      } else {
        struct trx_mod {
          UInt16 block;
          trx_mod( UInt16 bn) : block(bn) {}
          void operator () (transaction_state &t) {
            t.is_known_by_peer = true;
            t.block_num = static_cast<uint32_t>(block);
          }
        };
        c->trx_state.modify(tx,trx_mod(msg.refBlockNum));
      }

      try {
        chain_plug->accept_transaction (msg);
      } catch (const fc::exception &ex) {
        // received a block due to out of sequence
        elog ("accept txn threw  ${m}",("m",ex.what()));
      }
      catch (...) {
        elog (" caught something attempting to accept transaction");
      }

    }

    void net_plugin_impl::handle_message (connection_ptr c, const signed_block &msg) {
      chain_controller &cc = chain_plug->chain();
      try {
        if (cc.is_known_block(msg.id())) {
          return;
        }
      } catch (...) {
      }
      if (cc.head_block_num() >= msg.block_num()) {
        elog ("received a full block we know about #${n}", ("n",msg.block_num()));
      }

      bool has_chunk = false;
      uint32_t num = msg.block_num();
      bool syncing = sync_master->syncing();
      if( syncing ) {
        has_chunk = (c->sync_receiving && c->sync_receiving->end_block > 0);

        if (!has_chunk) {
          elog("got a block while syncing but no sync_receiving set #${n}",
               ("n",num));
        }
        else {
          if( c->sync_receiving->last + 1 != num) {
            elog ("expected block ${next} but got ${num}",("next",c->sync_receiving->last+1)("num",num));
          }
          c->sync_receiving->last = num;
        }
      }
      bool accepted = false;
      if (!syncing || num == cc.head_block_num()+1) {
        try {
          chain_plug->accept_block(msg, syncing);
          accepted = true;
        } catch (const unlinkable_block_exception &ex) {
          elog ("unlinkable_block_exception accept block #${n} syncing",("n",num));
          close (c);
        } catch (const assert_exception &ex) {
          elog ("unable to accept block on assert exception ${n}",("n",ex.what()));
        } catch (const fc::exception &ex) {
          elog ("accept_block threw a non-assert exception ${x}", ("x",ex.what()));
        } catch (...) {
          elog ("handle sync block caught something else");
        }
      }

      if (has_chunk) {
        if (!accepted) {
          c->sync_receiving->block_cache.emplace_back(std::move(c->blk_buffer));
        }

        if (num == c->sync_receiving->end_block) {
          sync_master->take_chunk (c);
        } else {
          c->sync_wait( );
        }
      }
      else {
#warning ("TODO: only send if not requested, and if the size is less than the just send it size otherwise send a notice");
        send_all (msg, [c](connection_ptr conn) -> bool {
            return (c != conn);
          });
       }
    }

    struct precache : public fc::visitor<void> {
      connection_ptr c;
      precache (connection_ptr conn) : c(conn) {}

      void operator()(const signed_block &msg) const
      {
        c->blk_buffer.resize(c->message_size);
        memcpy(c->blk_buffer.data(),c->pending_message_buffer.data(), c->message_size);
      }

      template <typename T>
      void operator()(const T &msg) const
      {
        //no-op
      }

    };

    struct msgHandler : public fc::visitor<void> {
      net_plugin_impl &impl;
      connection_ptr c;
      msgHandler (net_plugin_impl &imp, connection_ptr conn) : impl(imp), c(conn) {}

      template <typename T>
      void operator()(const T &msg) const
      {
        impl.handle_message (c, msg);
      }
    };


    void net_plugin_impl::start_reading_pending_buffer( connection_ptr c ) {
      boost::asio::async_read( *c->socket,
        boost::asio::buffer(c->pending_message_buffer.data(), c->pending_message_size ),
          [this,c]( boost::system::error_code ec, std::size_t bytes_transferred ) {
            if( !ec ) {
              try {
                c->message_size = bytes_transferred;
                auto msg = fc::raw::unpack<net_message>( c->pending_message_buffer );
                precache pc( c );
                msg.visit (pc);
                start_read_message( c );

                msgHandler m(*this, c);
                msg.visit(m);
                return;
              } catch ( const fc::exception& e ) {
                edump((e.to_detail_string() ));
              }
            } else {
              elog( "Error reading message from connection: ${m}", ("m", ec.message() ) );
            }
            close( c );
          });
    }

    void net_plugin_impl::start_conn_timer () {
      connector_check->expires_from_now (connector_period);
      connector_check->async_wait ([&](boost::system::error_code ec) {
          if (!ec) {
            connection_monitor ();
          }
          else {
            elog ("Error from connection check monitor: ${m}", ("m", ec.message()));
            start_conn_timer ();
          }
        });
    }

    void net_plugin_impl::start_txn_timer () {
      transaction_check->expires_from_now (txn_exp_period);
      transaction_check->async_wait ([&](boost::system::error_code ec) {
          if (!ec) {
            expire_txns ();
          }
          else {
            elog ("Error from connection check monitor: ${m}", ("m", ec.message()));
            start_txn_timer ();
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

    void net_plugin_impl::start_monitors () {
      connector_check.reset(new boost::asio::steady_timer (app().get_io_service()));
      transaction_check.reset(new boost::asio::steady_timer (app().get_io_service()));
      start_conn_timer();
      start_txn_timer();
    }

    void net_plugin_impl::expire_txns () {
      start_txn_timer ();
      auto &old = local_txns.get<by_expiry>();
      auto ex_up = old.upper_bound (time_point::now());
      auto ex_lo = old.lower_bound (fc::time_point_sec (0));
      old.erase (ex_lo, ex_up);
      auto &stale = local_txns.get<by_block_num>();
      chain_controller &cc = chain_plug->chain();
      uint32_t bn = cc.last_irreversible_block_num();
      auto bn_up = stale.upper_bound(bn);
      auto bn_lo = stale.lower_bound(0);
      stale.erase (bn_lo, bn_up);
    }

    void net_plugin_impl::connection_monitor () {
      start_conn_timer();
      vector <connection_ptr> discards;
      num_clients = 0;
      for (auto &c : connections ) {
        if (!c->socket->is_open() && !c->connecting) {
          if (c->peer_addr.length() > 0) {
            connect (c);
          }
          else {
            discards.push_back (c);
          }
        } else {
          if (c->peer_addr.empty()) {
            num_clients++;
          }
        }
      }
      if (discards.size () ) {
        for (auto &c : discards) {
          connections.erase( c );
          c.reset ();
        }
      }
    }

    void net_plugin_impl::close( connection_ptr c ) {
      if( c->peer_addr.empty( ) ) {
        --num_clients;
      }
      if (c->sync_receiving)
        sync_master->take_chunk (c);
      c->close();
    }

    void net_plugin_impl::send_all_txn (const SignedTransaction& txn) {
      transaction_id_type txnid = txn.id();
      if( local_txns.get<by_id>().find( txnid ) != local_txns.end () ) { //found
        return;
      }

      uint16_t bn = static_cast<uint16_t>(txn.refBlockNum);
      node_transaction_state nts = {txnid,time_point::now(),
                                    txn.expiration,
                                    bn, true};
      local_txns.insert(nts);

      if (fc::raw::pack_size(txn) <= just_send_it_max) {
        send_all (txn, [txn, txnid](connection_ptr c) -> bool {
            const auto& bs = c->trx_state.find(txnid);
            bool unknown = bs == c->trx_state.end();
            if (unknown)
              c->trx_state.insert(transaction_state({txnid,true,true,(uint32_t)-1,
                      fc::time_point(),fc::time_point() }));
            return unknown;
          });
      }
      else {
        pending_notify.push_back (txnid);
        notice_message nm = {pending_notify};
        send_all (nm, [txn, txnid](connection_ptr c) -> bool {
            const auto& bs = c->trx_state.find(txnid);
            bool unknown = bs == c->trx_state.end();
            if (unknown)
              c->trx_state.insert(transaction_state({txnid,false,true,(uint32_t)-1,
                      fc::time_point(),fc::time_point() }));
            return unknown;
          });
        pending_notify.clear();
      }
    }

    /**
     * This one is necessary to hook into the boost notifier api
     **/
    void net_plugin_impl::transaction_ready (const SignedTransaction& txn) {
      my_impl->send_all_txn (txn);
    }

    void net_plugin_impl::broadcast_block_impl (const chain::signed_block &sb) {
      if (send_whole_blocks) {
        send_all (sb,[](connection_ptr c) -> bool { return true; });
      }
      chain_controller &cc = chain_plug->chain();

      vector<transaction_id_type> trxs;
      if (!sb.cycles.empty()) {
        for (const auto& cyc : sb.cycles) {
          for (const auto& thr : cyc) {
            for (auto ui : thr.user_input) {
              auto &txn = cc.get_recent_transaction (ui.id());
              auto &id_iter = local_txns.get<by_id>();
              auto lt = id_iter.find(ui.id());
              if (lt != local_txns.end()) {
                id_iter.modify (lt, update_block_num(txn.refBlockNum));
              } else {
                transaction_id_type txnid = txn.id();
                if( local_txns.get<by_id>().find( txnid ) == local_txns.end () ) { //not found
                  uint16_t bn = static_cast<uint16_t>(txn.refBlockNum);
                  node_transaction_state nts = {txnid,time_point::now(),
                                                txn.expiration, bn, true};
                  local_txns.insert(nts);
                }
              }
              trxs.push_back (ui.id());
            }
          }
        }
      }

      if (!send_whole_blocks) {
        block_summary_message bsm = {sb, trxs};
        send_all (bsm,[sb](connection_ptr c) -> bool {
            return true;
            const auto& bs = c->block_state.find(sb.id());
            if (bs == c->block_state.end()) {
              c->block_state.insert ((block_state){sb.id(),true,true,fc::time_point()});
              return true;
            }
            return false;
          });
      }
    }

  void
  handshake_initializer::populate (handshake_message &hello) {
    hello.network_version = my_impl->network_version;
    hello.chain_id = my_impl->chain_id;
    hello.node_id = my_impl->node_id;
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
    try {
      hello.last_irreversible_block_id = cc.get_block_id_for_num
        (hello.last_irreversible_block_num = cc.last_irreversible_block_num());
    }
    catch (const unknown_block_exception &ex) {
      hello.last_irreversible_block_id = fc::sha256::hash(0);
      hello.last_irreversible_block_num = 0;
    }
    try {
      hello.head_id = cc.get_block_id_for_num
        (hello.head_num = cc.head_block_num());
    }
    catch (const unknown_block_exception &ex) {
      hello.head_id = fc::sha256::hash(0);
      hello.head_num = 0;
    }

  }

  net_plugin::net_plugin()
    :my( new net_plugin_impl ) {
    my_impl = my.get();
  }

  net_plugin::~net_plugin() {
  }

  void net_plugin::set_program_options( options_description& cli, options_description& cfg )
  {
    cfg.add_options()
      ("listen-endpoint", bpo::value<string>()->default_value( "0.0.0.0:9876" ), "The local IP address and port to listen for incoming connections.")
      ("remote-endpoint", bpo::value< vector<string> >()->composing(), "The IP address and port of a remote peer to sync with.")
      ("public-endpoint", bpo::value<string>(), "Overrides the advertised listen endpointlisten ip address.")
      ("agent-name", bpo::value<string>()->default_value("EOS Test Agent"), "The name supplied to identify this node amongst the peers.")
      ;
  }

  void net_plugin::plugin_initialize( const variables_map& options ) {
    ilog("Initialize net plugin");

    my->network_version = def_network_version;
    my->send_whole_blocks = def_send_whole_blocks;

    my->sync_master.reset( new sync_manager( def_sync_rec_span ) );

    my->connector_period = def_conn_retry_wait;
    my->txn_exp_period = def_txn_expire_wait;
    my->resp_expected_period = def_resp_expected_wait;
    my->just_send_it_max = def_max_just_send;
    my->max_client_count = def_max_clients;
    my->num_clients = 0;

    my->resolver = std::make_shared<tcp::resolver>( std::ref( app().get_io_service() ) );
    if( options.count( "listen-endpoint" ) ) {
      my->p2p_address = options.at("listen-endpoint").as< string >();
      auto host = my->p2p_address.substr( 0, my->p2p_address.find(':') );
      auto port = my->p2p_address.substr( host.size()+1, my->p2p_address.size() );
      idump((host)(port));
      tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
      // Note: need to add support for IPv6 too?

      my->listen_endpoint = *my->resolver->resolve( query);

      my->acceptor.reset( new tcp::acceptor( app().get_io_service() ) );
    }
    if (options.count ("public-endpoint") ) {
      my->p2p_address = options.at("public-endpoint").as< string >();
    }
    else {
      if (my->listen_endpoint.address().to_v4() == address_v4::any()) {
        boost::system::error_code ec;
        auto host = host_name(ec);
        if (ec.value() != boost::system::errc::success) {

          FC_THROW_EXCEPTION (fc::invalid_arg_exception,
                              "Unable to retrieve host_name. ${msg}", ("msg",ec.message()));

        }
        auto port = my->p2p_address.substr (my->p2p_address.find(':'), my->p2p_address.size());
        my->p2p_address = host + port;
      }
    }

    if( options.count( "remote-endpoint" ) ) {
      my->supplied_peers = options.at( "remote-endpoint" ).as< vector<string> >();
    }
    if (options.count("agent-name")) {
      my->user_agent_name = options.at ("agent-name").as< string > ();
    }
    my->chain_plug = app().find_plugin<chain_plugin>();
    my->chain_plug->get_chain_id(my->chain_id);
    fc::rand_pseudo_bytes(my->node_id.data(), my->node_id.data_size());

    my->keepalive_timer.reset(new boost::asio::steady_timer (app().get_io_service()));
    my->ticker();
  }

  void net_plugin::plugin_startup() {
    if( my->acceptor ) {
      my->acceptor->open(my->listen_endpoint.protocol());
      my->acceptor->set_option(tcp::acceptor::reuse_address(true));
      my->acceptor->bind(my->listen_endpoint);
      my->acceptor->listen();
      my->start_listen_loop();
    }

    my->chain_plug->chain().on_pending_transaction.connect (&net_plugin_impl::transaction_ready);
    my->start_monitors();

    for( auto seed_node : my->supplied_peers ) {
      connection_ptr c = std::make_shared<connection>(seed_node);
      my->connections.insert (c);
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

        ilog( "close ${s} connections", ("s",my->connections.size()) );
        auto cons = my->connections;
        for( auto con : cons ) {
          con->socket->close();
          my->close (con);
        }

        my->acceptor.reset(nullptr);
      }
      ilog( "exit shutdown" );
    } FC_CAPTURE_AND_RETHROW() }

    void net_plugin::broadcast_block (const chain::signed_block &sb) {
      my->broadcast_block_impl (sb);
    }
}
