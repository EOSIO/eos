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
#include <boost/thread.hpp>

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
    connection( socket_ptr s, bool try_recon )
      : block_state(),
        trx_state(),
        in_sync_state(),
        out_sync_state(),
        socket(s),
        shared_peers(),
        pending_message_size(),
        pending_message_buffer(),
        remote_node_id(),
        last_handshake(),
        out_queue(),
        try_reconnect (try_recon)
    {
      wlog( "created connection" );
      pending_message_buffer.resize( 1024*1024*4 );
      auto *rnd = remote_node_id.data();
      rnd[0] = 0;



    }

    ~connection() {
       wlog( "released connection" );
    }

    block_state_index              block_state;
    transaction_state_index        trx_state;
    sync_request_index             in_sync_state;
    sync_request_index             out_sync_state;
    socket_ptr                     socket;
    set<fc::sha256>                shared_peers;
    set<connection_ptr>            mutual_peers;

    uint32_t                       pending_message_size;
    vector<char>                   pending_message_buffer;
    vector<char>                   raw_recv;
    vector<char>                   raw_send;

    fc::sha256                     remote_node_id;
    handshake_message              last_handshake;
    std::deque<net_message>        out_queue;
    uint32_t                       mtu;
    bool                           try_reconnect;

    void send_handshake ( ) {
      dlog ("sending new handshake message" );
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

  static boost::thread_specific_ptr<transaction_id_type> last_recd_txn;
  static net_plugin_impl *my_impl;

  class last_recd_txn_guard {
  public:
    last_recd_txn_guard(transaction_id_type tid ) {
      last_recd_txn.reset (new transaction_id_type (tid));
    }

    ~last_recd_txn_guard () {
      dlog ("TSS Guard dtor 1");
      last_recd_txn.reset (0);
      dlog ("TSS Guard dtor 2");
    }
  };

  class net_plugin_impl {
  public:
    unique_ptr<tcp::acceptor>     acceptor;
    tcp::endpoint                 listen_endpoint;
    string                        p2p_address;

    vector<string>                supplied_peers;
    std::set<fc::sha256>          resolved_nodes;
    std::set<fc::sha256>          learned_nodes;

    std::set<socket_ptr>          pending_sockets;
    std::set< connection_ptr >    connections;
    bool                          done = false;


    int16_t                       network_version = 0;
    chain_id_type                 chain_id; ///< used to identify chain
    fc::sha256                    node_id; ///< used to identify peers and prevent self-connect

    string                        user_agent_name;
    chain_plugin*                 chain_plug;
    int32_t                       just_send_it_max;

    node_transaction_index        local_txns;
    vector<transaction_id_type>   pending_notify;

    void connect( const string& peer_addr ) {
      auto host = peer_addr.substr( 0, peer_addr.find(':') );
      auto port = peer_addr.substr( host.size()+1, host.size() );
      idump((host)(port));
      auto resolver = std::make_shared<tcp::resolver>( std::ref( app().get_io_service() ) );
      tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
      // Note: need to add support for IPv6 too

      resolver->async_resolve( query,
                               [resolver,peer_addr,this]( const boost::system::error_code& err, tcp::resolver::iterator endpoint_itr ){
                                 if( !err ) {
                                   connect( resolver, endpoint_itr );
                                 } else {
                                   elog( "Unable to resolve ${peer_addr}: ${error}", ( "peer_addr", peer_addr )("error", err.message() ) );
                                 }
                               });
    }

    void connect( std::shared_ptr<tcp::resolver> resolver, tcp::resolver::iterator endpoint_itr ) {
      auto sock = std::make_shared<tcp::socket>( std::ref( app().get_io_service() ) );
      pending_sockets.insert( sock );

      auto current_endpoint = *endpoint_itr;
      ++endpoint_itr;
      sock->async_connect( current_endpoint,
                           [sock,resolver,endpoint_itr, this]
                           ( const boost::system::error_code& err ) {
                             pending_sockets.erase( sock );
                             if( !err ) {
                               start_session( std::make_shared<connection>(sock, true));
                             } else {
                               if( endpoint_itr != tcp::resolver::iterator() ) {
                                 connect( resolver, endpoint_itr );
                               }
                             }
                           } );
    }

#if 0
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
#endif


    void start_session(connection_ptr con ) {
      connections.insert (con);
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
            start_session( std::make_shared<connection>( socket, false ) );
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

    void send_peer_message () {
      peer_message pm;
      for (auto &c : connections) {
        if ( (c->remote_node_id._hash[0] | c->remote_node_id._hash[1] | c->remote_node_id._hash[2] | c->remote_node_id._hash[0]) == 0 ) {
          return;
        }
        pm.peers.push_back(c->remote_node_id);
      }
      if (!pm.peers.empty()) {
        for (auto &c : connections) {
          if (c->out_sync_state.size() == 0) {
            c->send(pm);
          }
        }
      }
    }

    //    template<typename T>
    void send_all (const SignedTransaction &msg) {
      for (auto &c : connections) {
        if (c->out_sync_state.size() == 0) {
          const auto& bs = c->trx_state.find(msg.id());
          if (bs == c->trx_state.end()) {
            c->trx_state.insert(transaction_state({msg.id(),true,true,(uint32_t)-1,
                    fc::time_point(),fc::time_point()}));
          }
          c->send(msg);
        }
      }
    }

    void send_all (const block_summary_message &msg) {
      for (auto &c : connections) {
        const auto& bs = c->block_state.find(msg.block.id());
        if (bs == c->block_state.end()) {
          c->block_state.insert ((block_state){msg.block.id(),true,true,fc::time_point()});
          if (c->out_sync_state.size() == 0)
            c->send(msg);
        }
      }
    }

    void send_all (const notice_message &msg) {
      for (auto &c : connections) {
        if (c->out_sync_state.size() == 0) {
          for (const auto& b : msg.known_blocks) {
            const auto& bs = c->block_state.find(b);
            if (bs == c->block_state.end()) {
              c->block_state.insert ((block_state){b,false,true,fc::time_point()});
            }
          }
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

    void forward (connection_ptr source, const net_message &msg) {
      for (auto c : connections ) {
        if (c != source) {
          c->send (msg);
        }
      }
    }

    void handle_message (connection_ptr c, const handshake_message &msg) {
      dlog ("got a handshake message from ${p}", ("p", msg.p2p_address));
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
        elog ("Peer network id does not match ");
        close (c);
        return;
      }

      chain_controller& cc = chain_plug->chain();
      uint32_t head = cc.head_block_num ();
      if ( msg.head_num  >  head) {
        shared_fetch (head, msg.head_num);
      }

      dlog ("setting remote node id = ${n}",("n", msg.node_id));

      if ( c->remote_node_id != msg.node_id) {
        if (c->try_reconnect) {
          dlog ("adding ${pn} to resolved node list", ("pn", msg.node_id));
          auto old_id =  resolved_nodes.find (c->remote_node_id);
          if (old_id != resolved_nodes.end()) {
            dlog ("first purging old id");
            resolved_nodes.erase(old_id);
          }
          resolved_nodes.insert (msg.node_id);
        }
        else {
          dlog ("adding ${pn} to learned node list", ("pn", msg.node_id));
          auto old_id =  learned_nodes.find (c->remote_node_id);
          if (old_id != learned_nodes.end()) {
            dlog ("first purging old id");
            learned_nodes.erase(old_id);
          }
          learned_nodes.insert (msg.node_id);
        }

        c->remote_node_id = msg.node_id;
        send_peer_message();
      }
      c->last_handshake = msg;
    }

    void handle_message (connection_ptr c, const peer_message &msg) {
      dlog ("got a peer message with ${pc} from ${r}", ("pc", msg.peers.size())("r",c->last_handshake.p2p_address));
      c->shared_peers.clear();
      for (auto pnode : msg.peers) {
        if (pnode == node_id) {
          dlog ("skipping self peernode");
          continue;
        }
        if (pnode == c->remote_node_id) {
          dlog ("skipping received connection's node");
          continue;
        }
        for (auto &conn : connections) {
          if (conn->remote_node_id == pnode) {
            c->mutual_peers.insert (conn);
            break;
          }
        }
        c->shared_peers.insert (pnode);
      }
    }

    void handle_message (connection_ptr c, const notice_message &msg) {
      dlog ("got a notice message");
      notice_message fwd;
      request_message req;
      for (const auto& b : msg.known_blocks) {
        const auto &bs = c->block_state.find(b);
        if (bs == c->block_state.end()) {
          c->block_state.insert((block_state){b,true,true,fc::time_point()});
          fwd.known_blocks.push_back(b);
          req.req_blocks.push_back(b);
        }
      }

      for (const auto& t : msg.known_trx) {
        const auto &tx = c->trx_state.find(t);
        if (tx == c->trx_state.end()) {
          c->trx_state.insert((transaction_state){t,true,true,(uint32_t)-1,
                fc::time_point(),fc::time_point()});
          fwd.known_trx.push_back(t);
          req.req_trx.push_back(t);
        }
      }
      if (fwd.known_blocks.size() > 0 || fwd.known_trx.size() > 0) {
        forward (c, fwd);
        c->send(req);
      }
    }

    void handle_message (connection_ptr c, const request_message &msg) {
      dlog ("got a request message");
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
          dlog ("request message looping through peers");
          int cycle_count = 4;
          auto loop_start = conn_ndx++;
          while (conn_ndx != loop_start) {
            if (conn_ndx == connections.end()) {
              if (--cycle_count == 0) {
                dlog ("breaking out of stupid loop");
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
                // add to forward_to list
              break;
            }
            ++conn_ndx;
          }
        }
      }

      if (!send_now.empty()) {
      }
    }

    void handle_message (connection_ptr c, const sync_request_message &msg) {
      // dlog ("got a sync request message for blocks ${s} to ${e}",
      //      ("s",msg.start_block)("e", msg.end_block));
      sync_state req = {msg.start_block,msg.end_block,msg.start_block-1,time_point::now()};
      c->out_sync_state.insert (req);
      c->write_block_backlog ();
    }

    void handle_message (connection_ptr c, const block_summary_message &msg) {
      dlog ("got a block summary message blkid = ${b} from peer ${p}", ("b",msg.block.id())("p", c->remote_node_id));
#warning ("TODO: reconstruct actual block from cached transactions")
      const auto& itr = c->block_state.get<by_id>();
      auto bs = itr.find(msg.block.id());
      if (bs == c->block_state.end()) {
        dlog ("not found, forwarding on");
        c->block_state.insert (block_state({msg.block.id(),true,true,fc::time_point()}));
        forward (c, msg);
      } else {
        if (!bs->is_known) {
          dlog ("found, but !is_known, forwarding on");
          block_state value = *bs;
          value.is_known= true;
          c->block_state.insert (std::move(value));
          forward (c, msg);
        }
      }
      chain_controller &cc = chain_plug->chain();
      if (!cc.is_known_block(msg.block.id()) ) {
        try {
          chain_plug->accept_block(msg.block, false);
          dlog ("successfully accepted block");
        } catch (const unlinkable_block_exception &ex) {
          elog ("   caught unlinkable block exception #${n}",("n",msg.block.block_num()));
          // close (c);
        } catch (const assert_exception &ex) {
          // received a block due to out of sequence
          elog ("  caught assertion #${n}",("n",msg.block.block_num()));
          // close (c);
        }
      }
    }

    void handle_message (connection_ptr c, const SignedTransaction &msg) {
      chain_controller &cc = chain_plug->chain();
      if (!cc.is_known_transaction(msg.id())) {
        last_recd_txn_guard tls_guard(msg.id());
        chain_plug->accept_transaction (msg);
        uint16_t bn = static_cast<uint16_t>(msg.refBlockNum);
        node_transaction_state nts = {msg.id(),time_point::now(),msg.expiration,
                                      msg,bn, true};
        local_txns.insert(nts);
        forward (c, msg);
      }
    }

    void handle_message (connection_ptr c, const signed_block &msg) {
      uint32_t bn = msg.block_num();
      dlog ("got a signed_block, num = ${n}", ("n", bn));
      chain_controller &cc = chain_plug->chain();

      if (cc.is_known_block(msg.id())) {
        // dlog ("block id ${id} is known", ("id", msg.id()) );
        return;
      }
      uint32_t num = 0;

      for( auto ss = c->in_sync_state.begin(); ss != c->in_sync_state.end(); ) {
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
      try {
        chain_plug->accept_block(msg, true);
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
            if ( c->try_reconnect ) {
#warning ("TODO: Add reconnect logic after a read failure");
            }
            else {
              close( c );
            }
          });
    }


    void close( connection_ptr c ) {
      if( c->socket )
        c->socket->close();
      connections.erase( c );
      c.reset ();
    }

    void send_all_txn (const SignedTransaction& txn) {
      if (last_recd_txn.get() && *last_recd_txn.get() == txn.id()) {
        dlog ("skipping our received transacton");
        return;
      }

      if (true) { //txn.get_size() <= just_send_it_max) {
        send_all (txn);
        return;
      }

      uint32_t psize = (pending_notify.size()+1) * sizeof (txn.id());
      if (psize >= my_impl->just_send_it_max) {
        notice_message nm = {vector<block_id_type>(), pending_notify};
        send_all (nm);
        pending_notify.clear();
      }
      pending_notify.push_back(txn.id());
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
    auto resolver = std::make_shared<tcp::resolver>( std::ref( app().get_io_service() ) );
    if( options.count( "listen-endpoint" ) ) {
      my->p2p_address = options.at("listen-endpoint").as< string >();
      auto host = my->p2p_address.substr( 0, my->p2p_address.find(':') );
      auto port = my->p2p_address.substr( host.size()+1, my->p2p_address.size() );
      idump((host)(port));
      tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
      // Note: need to add support for IPv6 too?

      my->listen_endpoint = *resolver->resolve( query);

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
    dlog("my node_id = ${n}", ("n",my->node_id));

    my->just_send_it_max = 1300;
  }

  void net_plugin::plugin_startup() {
    if( my->acceptor ) {

      my->acceptor->open(my->listen_endpoint.protocol());
      my->acceptor->set_option(tcp::acceptor::reuse_address(true));
      my->acceptor->bind(my->listen_endpoint);
      my->acceptor->listen();
      my->chain_plug->chain().on_pending_transaction.connect (&net_plugin_impl::pending_txn);

      my->start_listen_loop();
    }

    for( auto seed_node : my->supplied_peers ) {
      my->connect( seed_node );
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

      vector<block_id_type> blks;
      blks.push_back (sb.id());
      notice_message nm = {blks, my->pending_notify};
      my->send_all (nm);

      block_summary_message bsm = {sb, trxs};
      my->send_all (bsm);
      my->pending_notify.clear();
    }

}
