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

  using connection_ptr = std::shared_ptr<connection>;
  using connection_wptr = std::weak_ptr<connection>;

  using socket_ptr = std::shared_ptr<tcp::socket>;

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
    uint32_t     start_block = 0;
    uint32_t     end_block = 0;
    uint32_t     last = 0; ///< last sent or received
    time_point   start_time; ///< time request made or received
  };

  struct by_start_block;
  typedef multi_index_container<
    sync_state,
    indexed_by<
      ordered_unique< tag<by_start_block>, member<sync_state, uint32_t, &sync_state::start_block > >
      >
    > sync_request_index;


  struct handshake_initializer {
    static void populate (handshake_message &hello);
  };

  class connection : public std::enable_shared_from_this<connection> {
  public:
    connection( string endpoint )
      : block_state(),
        trx_state(),
        in_sync_state(),
        out_sync_state(),
        socket( std::make_shared<tcp::socket>( std::ref( app().get_io_service() ))),
        pending_message_size(),
        pending_message_buffer(),
        remote_node_id(),
        last_handshake(),
        out_queue(),
        connecting (false),
        peer_addr (endpoint)
    {
      wlog( "created connection to ${n}", ("n", endpoint) );
      pending_message_buffer.resize( 1024*1024*4 );
      auto *rnd = remote_node_id.data();
      rnd[0] = 0;
    }

    connection( socket_ptr s )
      : block_state(),
        trx_state(),
        in_sync_state(),
        out_sync_state(),
        socket( s ),
        pending_message_size(),
        pending_message_buffer(),
        remote_node_id(),
        last_handshake(),
        out_queue(),
        connecting (false),
        peer_addr ()
    {
      wlog( "created connection from client" );
      pending_message_buffer.resize( 1024*1024*4 );
      auto *rnd = remote_node_id.data();
      rnd[0] = 0;
    }

    ~connection() {
      if (peer_addr.empty())
        wlog( "released connection from client" );
      else
        wlog( "released connection to server at ${addr}", ("addr", peer_addr) );
    }


    block_state_index              block_state;
    transaction_state_index        trx_state;
    sync_request_index             in_sync_state;  // we are requesting info from this peer
    sync_request_index             out_sync_state; // this peer is requesting info from us
    socket_ptr                     socket;

    uint32_t                       pending_message_size;
    vector<char>                   pending_message_buffer;

    fc::sha256                     remote_node_id;
    handshake_message              last_handshake;
    std::deque<net_message>        out_queue;
    uint32_t                       mtu;
    bool                           connecting;
    string                         peer_addr;

    void reset () {
      in_sync_state.clear();
      out_sync_state.clear();
      block_state.clear();
      trx_state.clear();
    }

    void close () {
      out_queue.clear();
      if (socket) {
        socket->close();
      }
    }

    void send_handshake ( ) {
      handshake_message hello;
      handshake_initializer::populate(hello);
      send (hello);
    }

    void send( const net_message& m ) {
      out_queue.push_back( m );
      if( out_queue.size() == 1 ) {
        send_next_message();
      }
    }

    void send_next_message() {
      if( !out_queue.size() ) {
        if (out_sync_state.size() > 0) {
          write_block_backlog();
        }
        return;
      }

      auto& m = out_queue.front();
      vector<char> buffer;
      uint32_t size = fc::raw::pack_size( m );
      buffer.resize(  size + sizeof(uint32_t) );
      fc::datastream<char*> ds( buffer.data(), buffer.size() );
      ds.write( (char*)&size, sizeof(size) );
      fc::raw::pack( ds, m );

      boost::asio::async_write( *socket, boost::asio::buffer( buffer.data(), buffer.size() ),
                   [this,buf=std::move(buffer)]( boost::system::error_code ec, std::size_t bytes_transferred ) {
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

   void write_block_backlog ( ) {
      chain_controller& cc = app().find_plugin<chain_plugin>()->chain();
      auto ss = out_sync_state.begin();
      uint32_t num = ++ss.get_node()->value().last;
      ilog ("num = ${num} end = ${end}",("num",num)("end",ss->end_block));
      if (num >= ss->end_block) {
        out_sync_state.erase(ss);
        ilog ("out sync size = ${s}",("s",out_sync_state.size()));
      }
      try {
        fc::optional<signed_block> sb = cc.fetch_block_by_number(num);
        if (sb) {
          // dlog("write backlog, block #${num}",("num",num));
          send( *sb );
        }
      } catch ( ... ) {
        wlog( "write loop exception" );
      }
      if (out_sync_state.size() == 0) {
        send_handshake ( );
      }
  }


  }; // class connection


  struct node_transaction_state {
    transaction_id_type id;
    fc::time_point      received;
    fc::time_point_sec  expires;
    // vector<char>        packed_transaction; //just for the moment
    SignedTransaction   transaction;
    uint32_t            block_num = -1; /// block transaction was included in
    bool                validated = false; /// whether or not our node has validated it
  };

  struct by_expiry;

  typedef multi_index_container<
    node_transaction_state,
    indexed_by<
      ordered_unique<
        tag<by_id>, member < node_transaction_state,
                             transaction_id_type,
                             &node_transaction_state::id > >,
      ordered_non_unique<
        tag<by_expiry>, member< node_transaction_state,
                                fc::time_point_sec,
                                &node_transaction_state::expires > >
      >
    >
  node_transaction_index;


  static net_plugin_impl *my_impl;

  class net_plugin_impl {
  public:
    unique_ptr<tcp::acceptor>     acceptor;
    tcp::endpoint                 listen_endpoint;
    string                        p2p_address;

    vector<string>                supplied_peers;
    std::set<fc::sha256>          resolved_nodes;
    std::set<fc::sha256>          learned_nodes;

    std::set< connection_ptr >    connections;
    bool                          done = false;

    unique_ptr<boost::asio::steady_timer>        connector_check;
    unique_ptr<boost::asio::steady_timer>        transaction_check;
    boost::asio::steady_timer::duration          connector_period;
    boost::asio::steady_timer::duration          txn_exp_period;

    int16_t                       network_version;
    chain_id_type                 chain_id;
    fc::sha256                    node_id;

    string                        user_agent_name;
    chain_plugin*                 chain_plug;
    int32_t                       just_send_it_max;
    bool                          send_whole_blocks;

    node_transaction_index        local_txns;
    vector<transaction_id_type>   pending_notify;

    shared_ptr<tcp::resolver>     resolver;


    void connect( connection_ptr c ) {
      c->connecting = true;
      auto host = c->peer_addr.substr( 0, c->peer_addr.find(':') );
      auto port = c->peer_addr.substr( host.size()+1, host.size() );
      idump((host)(port));
      tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
      // Note: need to add support for IPv6 too

      resolver->async_resolve( query,
                               [c, this]( const boost::system::error_code& err, tcp::resolver::iterator endpoint_itr ){
                                 if( !err ) {
                                   connect( c, endpoint_itr );
                                 } else {
                                   elog( "Unable to resolve ${peer_addr}: ${error}", ( "peer_addr", c->peer_addr )("error", err.message() ) );
                                 }
                               });
    }

    void connect( connection_ptr c, tcp::resolver::iterator endpoint_itr ) {
      auto current_endpoint = *endpoint_itr;
      ++endpoint_itr;
      c->socket->async_connect( current_endpoint,
                           [c,endpoint_itr, this]
                           ( const boost::system::error_code& err ) {
                             if( !err ) {
                               start_session( c );
                             } else {
                               if( endpoint_itr != tcp::resolver::iterator() ) {
                                 connect( c, endpoint_itr );
                               }
                               else {
                                 c->connecting = false;
                               }
                             }
                           } );
    }

    void start_session(connection_ptr con ) {
      con->connecting = false;
      uint32_t mtu = 1300; // need a way to query this
      if (mtu < just_send_it_max) {
        just_send_it_max = mtu;
      }
      start_read_message( con );

      con->send_handshake( );

      // for now, we can just use the application main loop.
      //     con->readloop_complete  = bf::async( [=](){ read_loop( con ); } );
      //     con->writeloop_complete = bf::async( [=](){ write_loop con ); } );
    }


    void start_listen_loop() {
      auto socket = std::make_shared<tcp::socket>( std::ref( app().get_io_service() ) );
      acceptor->async_accept( *socket, [socket,this]( boost::system::error_code ec ) {
          if( !ec ) {
            connection_ptr c = std::make_shared<connection>( socket );
            connections.insert( c );
            start_session( c );
            start_listen_loop();
          } else {
            elog( "Error accepting connection: ${m}", ("m", ec.message() ) );
          }
        });
    }

    void start_read_message( connection_ptr conn ) {
      conn->pending_message_size = 0;
      connection_wptr c (conn);
      uint32_t *buff = &(conn.get()->pending_message_size);

      boost::asio::async_read( *conn->socket,
                               boost::asio::buffer((char *)buff, sizeof(conn->pending_message_size)),
                               [this,c]( boost::system::error_code ec, std::size_t bytes_transferred ) {
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


    tcp::endpoint fc_to_asio (const fc::ip::endpoint &fcep) {
      address_v4 addr((uint32_t)fcep.get_address());
      return tcp::endpoint(addr, fcep.port());
    }

    fc::ip::endpoint asio_to_fc (const tcp::endpoint &ep) {
      uint32_t addr = ep.address().to_v4().to_ulong();
      return fc::ip::endpoint (addr,ep.port());
    }

    template<typename VerifierFunc>
    void send_all (const net_message &msg, VerifierFunc verify) {
      for (auto &c : connections) {
        if (c->out_sync_state.size() == 0 &&
            verify (c)) {
          c->send(msg);
        }
      }
    }

    void shared_fetch (uint32_t low, uint32_t high) {

      uint32_t delta = high - low;
      uint32_t count = connections.size();
      FC_ASSERT (count > 0);
      uint32_t span = delta / count;
      uint32_t lastSpan = delta - (span * (count-1));
      for (auto &cx: connections) {
        if (--count == 0) {
          span = lastSpan;
        }
        sync_state req = {low+1, low+span, low, time_point::now() };
        cx->in_sync_state.insert (req);
        sync_request_message srm = {req.start_block, req.end_block };
        cx->send (srm);
        low += span;
      }
    }

    void handle_message (connection_ptr c, const handshake_message &msg) {
      if (msg.node_id == node_id) {
        elog ("Self connection detected. Closing connection");
        close(c);
        return;
      }
      if (msg.chain_id != chain_id) {
        elog ("Peer on a different chain. Closing connection");
        close (c);
        return;
      }
      if (msg.network_version != network_version) {
        elog ("Peer network version does not match ");
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

      uint32_t head = cc.head_block_num ();
      if ( msg.head_num  >  head) {
        shared_fetch (head, msg.head_num);
      }

      if ( c->remote_node_id != msg.node_id) {
        c->reset();
        if (c->peer_addr.length() > 0) {
          auto old_id =  resolved_nodes.find (c->remote_node_id);
          if (old_id != resolved_nodes.end()) {
            resolved_nodes.erase(old_id);
          }
          resolved_nodes.insert (msg.node_id);
        }
        else {
          auto old_id =  learned_nodes.find (c->remote_node_id);
          if (old_id != learned_nodes.end()) {
            learned_nodes.erase(old_id);
          }
          learned_nodes.insert (msg.node_id);
        }

        c->remote_node_id = msg.node_id;
      }
      c->last_handshake = msg;
    }

    void handle_message (connection_ptr c, const notice_message &msg) {
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
        c->send(req);
      }
      }

    void handle_message (connection_ptr c, const request_message &msg) {
        // collect a list of transactions that were found.
        // collect a second list of transaction ids that were not found but are otherwise known by some peers
        // finally, what remains are future(?) transactions
      vector< SignedTransaction > send_now;
      map <connection_ptr, vector < transaction_id_type > > forward_to;
      auto conn_ndx = connections.begin();
      for (auto t: msg.req_trx) {
        auto txn = local_txns.get<by_id>().find(t);
        if (txn != local_txns.end()) {
          send_now.push_back(txn->transaction);
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
          c->send (t);
        }
      }
    }

    void handle_message (connection_ptr c, const sync_request_message &msg) {
      sync_state req = {msg.start_block,msg.end_block,msg.start_block-1,time_point::now()};
      c->out_sync_state.insert (req);
      c->write_block_backlog ();
    }

    void handle_message (connection_ptr c, const block_summary_message &msg) {
      const auto& itr = c->block_state.get<by_id>();
      auto bs = itr.find(msg.block);
      if (bs == c->block_state.end()) {
        c->block_state.insert (block_state({msg.block,true,true,fc::time_point()}));
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
      if (!cc.is_known_block(msg.block) ) {
        try {
          chain_plug->accept_block(sb, false);
        } catch (const unlinkable_block_exception &ex) {
          elog ("   caught unlinkable block exception #${n}",("n",sb.block_num()));
           close (c);
        } catch (const assert_exception &ex) {
          // received a block due to out of sequence
          elog ("  caught assertion #${n}",("n",sb.block_num()));
          close (c);
        }
      }
    }

    void handle_message (connection_ptr c, const SignedTransaction &msg) {
      auto txn = local_txns.get<by_id>().find(msg.id());
      if (txn != local_txns.end()) {
        return;
      }
      chain_controller &cc = chain_plug->chain();
      if (!cc.is_known_transaction(msg.id())) {
        chain_plug->accept_transaction (msg);
      }
    }

    void handle_message (connection_ptr c, const signed_block &msg) {
      chain_controller &cc = chain_plug->chain();

      if (cc.is_known_block(msg.id())) {
        return;
      }
      uint32_t num = 0;
      bool syncing = c->in_sync_state.size() > 0;
      if (syncing) {
        for( auto ss = c->in_sync_state.begin(); ss != c->in_sync_state.end(); ss++ ) {
          if (msg.block_num() == ss->last + 1 && msg.block_num() <= ss->end_block) {
            num = msg.block_num();
            ss.get_node()->value().last = num;
            break;
          }
        }
        if (num == 0) {
          elog ("Got out-of-order block ${n}",("n",msg.block_num()));
          close (c);
          return;
        }
      }
      try {
        chain_plug->accept_block(msg, syncing);
      } catch (const unlinkable_block_exception &ex) {
        elog ("unable to accpt block #${n}",("n",num));
        close (c);
      } catch (const assert_exception &ex) {
        elog ("unable to accept block cuz my asserts! #${n}",("n",num));
        close (c);
      }
    }


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


    void start_reading_pending_buffer( connection_ptr c ) {
      boost::asio::async_read( *c->socket,
        boost::asio::buffer(c->pending_message_buffer.data(), c->pending_message_size ),
          [this,c]( boost::system::error_code ec, std::size_t bytes_transferred ) {
            if( !ec ) {
              try {
                auto msg = fc::raw::unpack<net_message>( c->pending_message_buffer );
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

    void start_conn_timer () {
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

    void start_txn_timer () {
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

    void start_monitors () {
      connector_check.reset(new boost::asio::steady_timer (app().get_io_service()));
      transaction_check.reset(new boost::asio::steady_timer (app().get_io_service()));
      start_conn_timer();
      start_txn_timer();
    }

    void expire_txns () {
      start_txn_timer ();
#warning ("TODO: Add by-expiry purging code");
    }

    void connection_monitor () {
      start_conn_timer();
      vector <connection_ptr> discards;
      for (auto &c : connections ) {
        if (!c->socket->is_open() && !c->connecting) {
          if (c->peer_addr.length() > 0) {
            connect (c);
          }
          else {
            discards.push_back (c);
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

    void close( connection_ptr c ) {
      c->close();
    }

    void send_all_txn (const SignedTransaction& txn) {
      uint16_t bn = static_cast<uint16_t>(txn.refBlockNum);
      node_transaction_state nts = {txn.id(),time_point::now(),txn.expiration,
                                    txn,bn, true};
      local_txns.insert(nts);

      if (sizeof(txn) <= just_send_it_max) {
        send_all (txn, [txn](connection_ptr c) -> bool {
            const auto& bs = c->trx_state.find(txn.id());
            bool unknown = bs == c->trx_state.end();
            if (unknown)
              c->trx_state.insert(transaction_state({txn.id(),true,true,(uint32_t)-1,
                      fc::time_point(),fc::time_point() }));
            return unknown;
          });
      }
      else {
        pending_notify.push_back (txn.id());
        notice_message nm = {pending_notify};
        send_all (nm, [txn](connection_ptr c) -> bool {
            const auto& bs = c->trx_state.find(txn.id());
            bool unknown = bs == c->trx_state.end();
            if (unknown)
              c->trx_state.insert(transaction_state({txn.id(),false,true,(uint32_t)-1,
                      fc::time_point(),fc::time_point() }));
            return unknown;
          });
        pending_notify.clear();
      }
    }

    /**
     * This one is necessary to hook into the boost notifier api
     **/
    static void pending_txn (const SignedTransaction& txn) {
      my_impl->send_all_txn (txn);
    }


  }; // class net_plugin_impl

  void
  handshake_initializer::populate (handshake_message &hello) {
    hello.network_version = 0;
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

    my->send_whole_blocks = true;

    if( options.count( "remote-endpoint" ) ) {
      my->supplied_peers = options.at( "remote-endpoint" ).as< vector<string> >();
    }
    if (options.count("agent-name")) {
      my->user_agent_name = options.at ("agent-name").as< string > ();
    }
    my->chain_plug = app().find_plugin<chain_plugin>();
    my->chain_plug->get_chain_id(my->chain_id);
    fc::rand_pseudo_bytes(my->node_id.data(), my->node_id.data_size());

    my->connector_period = std::chrono::seconds (30);
    my->txn_exp_period = std::chrono::seconds (3);

    my->just_send_it_max = 1300;
  }

  void net_plugin::plugin_startup() {
    if( my->acceptor ) {
      my->acceptor->open(my->listen_endpoint.protocol());
      my->acceptor->set_option(tcp::acceptor::reuse_address(true));
      my->acceptor->bind(my->listen_endpoint);
      my->acceptor->listen();
      my->start_listen_loop();
    }

    my->chain_plug->chain().on_pending_transaction.connect (&net_plugin_impl::pending_txn);
    my->start_monitors();

    for( auto seed_node : my->supplied_peers ) {
      connection_ptr c = std::make_shared<connection>(seed_node);
      my->connections.insert (c);
      my->connect( c );
    }
    boost::asio::signal_set signals (app().get_io_service(), SIGINT, SIGTERM);
    signals.async_wait ([this](const boost::system::error_code &ec, int signum) {
        dlog ("caught signal ${sn}", ("sn", signum) ) ;
      });
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
      if (my->send_whole_blocks) {
        my->send_all (sb,[](connection_ptr c) -> bool { return true; });
        return;
      }
      vector<transaction_id_type> trxs;
      if (!sb.cycles.empty()) {
        for (const auto& cyc : sb.cycles) {
          for (const auto& thr : cyc) {
            for (auto ui : thr.user_input) {
              trxs.push_back (ui.id());
            }
          }
        }
      }

      block_summary_message bsm = {sb.id(), trxs};
      my->send_all (bsm,[sb](connection_ptr c) -> bool {
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
