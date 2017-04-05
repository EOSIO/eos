/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <eos/net/node.hpp>
#include <eos/net/peer_database.hpp>
#include <eos/net/message_oriented_connection.hpp>
#include <eos/net/stcp_socket.hpp>
#include <eos/net/config.hpp>

#include <boost/tuple/tuple.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <queue>
#include <boost/container/deque.hpp>
#include <fc/thread/future.hpp>

namespace eos { namespace net
  {
    struct firewall_check_state_data
    {
      node_id_t        expected_node_id;
      fc::ip::endpoint endpoint_to_test;

      // if we're coordinating a firewall check for another node, these are the helper
      // nodes we've already had do the test (if this structure is still relevant, that
      // that means they have all had indeterminate results
      std::set<node_id_t> nodes_already_tested;

      // If we're a just a helper node, this is the node we report back to
      // when we have a result
      node_id_t        requesting_peer;
    };

    class peer_connection;
    class peer_connection_delegate
    {
    public:
      virtual void on_message(peer_connection* originating_peer,
                              const message& received_message) = 0;
      virtual void on_connection_closed(peer_connection* originating_peer) = 0;
      virtual message get_message_for_item(const item_id& item) = 0;
    };

    class peer_connection;
    typedef std::shared_ptr<peer_connection> peer_connection_ptr;
    class peer_connection : public message_oriented_connection_delegate,
                            public std::enable_shared_from_this<peer_connection>
    {
    public:
      enum class our_connection_state
      {
        disconnected,
        just_connected, // if in this state, we have sent a hello_message
        connection_accepted, // remote side has sent us a connection_accepted, we're operating normally with them
        connection_rejected // remote side has sent us a connection_rejected, we may be exchanging address with them or may just be waiting for them to close
      };
      enum class their_connection_state
      {
        disconnected,
        just_connected, // we have not yet received a hello_message
        connection_accepted, // we have sent them a connection_accepted
        connection_rejected // we have sent them a connection_rejected
      };
      enum class connection_negotiation_status
      {
        disconnected,
        connecting,
        connected,
        accepting,
        accepted,
        hello_sent,
        peer_connection_accepted,
        peer_connection_rejected,
        negotiation_complete,
        closing,
        closed
      };
    private:
      peer_connection_delegate*      _node;
      fc::optional<fc::ip::endpoint> _remote_endpoint;
      message_oriented_connection    _message_connection;

      /* a base class for messages on the queue, to hide the fact that some
       * messages are complete messages and some are only hashes of messages.
       */
      struct queued_message
      {
        fc::time_point enqueue_time;
        fc::time_point transmission_start_time;
        fc::time_point transmission_finish_time;

        queued_message(fc::time_point enqueue_time = fc::time_point::now()) :
          enqueue_time(enqueue_time)
        {}

        virtual message get_message(peer_connection_delegate* node) = 0;
        /** returns roughly the number of bytes of memory the message is consuming while
         * it is sitting on the queue
         */
        virtual size_t get_size_in_queue() = 0;
        virtual ~queued_message() {}
      };

      /* when you queue up a 'real_queued_message', a full copy of the message is
       * stored on the heap until it is sent
       */
      struct real_queued_message : queued_message
      {
        message        message_to_send;
        size_t         message_send_time_field_offset;

        real_queued_message(message message_to_send,
                            size_t message_send_time_field_offset = (size_t)-1) :
          message_to_send(std::move(message_to_send)),
          message_send_time_field_offset(message_send_time_field_offset)
        {}

        message get_message(peer_connection_delegate* node) override;
        size_t get_size_in_queue() override;
      };

      /* when you queue up a 'virtual_queued_message', we just queue up the hash of the
       * item we want to send.  When it reaches the top of the queue, we make a callback
       * to the node to generate the message.
       */
      struct virtual_queued_message : queued_message
      {
        item_id item_to_send;

        virtual_queued_message(item_id item_to_send) :
          item_to_send(std::move(item_to_send))
        {}

        message get_message(peer_connection_delegate* node) override;
        size_t get_size_in_queue() override;
      };


      size_t _total_queued_messages_size;
      std::queue<std::unique_ptr<queued_message>, std::list<std::unique_ptr<queued_message> > > _queued_messages;
      fc::future<void> _send_queued_messages_done;
    public:
      fc::time_point connection_initiation_time;
      fc::time_point connection_closed_time;
      fc::time_point connection_terminated_time;
      peer_connection_direction direction;
      //connection_state state;
      firewalled_state is_firewalled;
      fc::microseconds clock_offset;
      fc::microseconds round_trip_delay;

      our_connection_state our_state;
      bool they_have_requested_close;
      their_connection_state their_state;
      bool we_have_requested_close;

      connection_negotiation_status negotiation_status;
      fc::oexception connection_closed_error;

      fc::time_point get_connection_time()const { return _message_connection.get_connection_time(); }
      fc::time_point get_connection_terminated_time()const { return connection_terminated_time; }

      /// data about the peer node
      /// @{
      /** node_public_key from the hello message, zero-initialized before we get the hello */
      node_id_t        node_public_key;
      /** the unique identifier we'll use to refer to the node with.  zero-initialized before
       * we receive the hello message, at which time it will be filled with either the "node_id"
       * from the user_data field of the hello, or if none is present it will be filled with a
       * copy of node_public_key */
      node_id_t        node_id;
      uint32_t         core_protocol_version;
      std::string      user_agent;
      fc::optional<std::string> eos_git_revision_sha;
      fc::optional<fc::time_point_sec> eos_git_revision_unix_timestamp;
      fc::optional<std::string> fc_git_revision_sha;
      fc::optional<fc::time_point_sec> fc_git_revision_unix_timestamp;
      fc::optional<std::string> platform;
      fc::optional<uint32_t> bitness;

      // for inbound connections, these fields record what the peer sent us in
      // its hello message.  For outbound, they record what we sent the peer
      // in our hello message
      fc::ip::address inbound_address;
      uint16_t inbound_port;
      uint16_t outbound_port;
      /// @}

      typedef std::unordered_map<item_id, fc::time_point> item_to_time_map_type;

      /// blockchain synchronization state data
      /// @{
      boost::container::deque<item_hash_t> ids_of_items_to_get; /// id of items in the blockchain that this peer has told us about
      std::set<item_hash_t> ids_of_items_being_processed; /// list of all items this peer has offered use that we've already handed to the client but the client hasn't finished processing
      uint32_t number_of_unfetched_item_ids; /// number of items in the blockchain that follow ids_of_items_to_get but the peer hasn't yet told us their ids
      bool peer_needs_sync_items_from_us;
      bool we_need_sync_items_from_peer;
      fc::optional<boost::tuple<std::vector<item_hash_t>, fc::time_point> > item_ids_requested_from_peer; /// we check this to detect a timed-out request and in busy()
      item_to_time_map_type sync_items_requested_from_peer; /// ids of blocks we've requested from this peer during sync.  fetch from another peer if this peer disconnects
      item_hash_t last_block_delegate_has_seen; /// the hash of the last block  this peer has told us about that the peer knows
      fc::time_point_sec last_block_time_delegate_has_seen;
      bool inhibit_fetching_sync_blocks;
      /// @}

      /// non-synchronization state data
      /// @{
      struct timestamped_item_id
      {
        item_id            item;
        fc::time_point_sec timestamp;
        timestamped_item_id(const item_id& item, const fc::time_point_sec timestamp) :
          item(item),
          timestamp(timestamp)
        {}
      };
      struct timestamp_index{};
      typedef boost::multi_index_container<timestamped_item_id,
                                           boost::multi_index::indexed_by<boost::multi_index::hashed_unique<boost::multi_index::member<timestamped_item_id, item_id, &timestamped_item_id::item>,
                                                                                                            std::hash<item_id> >,
                                                                          boost::multi_index::ordered_non_unique<boost::multi_index::tag<timestamp_index>,
                                                                                                                 boost::multi_index::member<timestamped_item_id, fc::time_point_sec, &timestamped_item_id::timestamp> > > > timestamped_items_set_type;
      timestamped_items_set_type inventory_peer_advertised_to_us;
      timestamped_items_set_type inventory_advertised_to_peer;

      item_to_time_map_type items_requested_from_peer;  /// items we've requested from this peer during normal operation.  fetch from another peer if this peer disconnects
      /// @}

      // if they're flooding us with transactions, we set this to avoid fetching for a few seconds to let the
      // blockchain catch up
      fc::time_point transaction_fetching_inhibited_until;

      uint32_t last_known_fork_block_number;

      fc::future<void> accept_or_connect_task_done;

      firewall_check_state_data *firewall_check_state;
#ifndef NDEBUG
    private:
      fc::thread* _thread;
      unsigned _send_message_queue_tasks_running; // temporary debugging
#endif
    private:
      peer_connection(peer_connection_delegate* delegate);
      void destroy();
    public:
      static peer_connection_ptr make_shared(peer_connection_delegate* delegate); // use this instead of the constructor
      virtual ~peer_connection();

      fc::tcp_socket& get_socket();
      void accept_connection();
      void connect_to(const fc::ip::endpoint& remote_endpoint, fc::optional<fc::ip::endpoint> local_endpoint = fc::optional<fc::ip::endpoint>());

      void on_message(message_oriented_connection* originating_connection, const message& received_message) override;
      void on_connection_closed(message_oriented_connection* originating_connection) override;

      void send_queueable_message(std::unique_ptr<queued_message>&& message_to_send);
      void send_message(const message& message_to_send, size_t message_send_time_field_offset = (size_t)-1);
      void send_item(const item_id& item_to_send);
      void close_connection();
      void destroy_connection();

      uint64_t get_total_bytes_sent() const;
      uint64_t get_total_bytes_received() const;

      fc::time_point get_last_message_sent_time() const;
      fc::time_point get_last_message_received_time() const;

      fc::optional<fc::ip::endpoint> get_remote_endpoint();
      fc::ip::endpoint get_local_endpoint();
      void set_remote_endpoint(fc::optional<fc::ip::endpoint> new_remote_endpoint);

      bool busy();
      bool idle();

      bool is_transaction_fetching_inhibited() const;
      fc::sha512 get_shared_secret() const;
      void clear_old_inventory();
      bool is_inventory_advertised_to_us_list_full_for_transactions() const;
      bool is_inventory_advertised_to_us_list_full() const;
      bool performing_firewall_check() const;
      fc::optional<fc::ip::endpoint> get_endpoint_for_connecting() const;
    private:
      void send_queued_messages_task();
      void accept_connection_task();
      void connect_to_task(const fc::ip::endpoint& remote_endpoint);
    };
    typedef std::shared_ptr<peer_connection> peer_connection_ptr;

 } } // end namespace eos::net

// not sent over the wire, just reflected for logging
FC_REFLECT_ENUM(eos::net::peer_connection::our_connection_state, (disconnected)
                                                                 (just_connected)
                                                                 (connection_accepted)
                                                                 (connection_rejected))
FC_REFLECT_ENUM(eos::net::peer_connection::their_connection_state, (disconnected)
                                                                   (just_connected)
                                                                   (connection_accepted)
                                                                   (connection_rejected))
FC_REFLECT_ENUM(eos::net::peer_connection::connection_negotiation_status, (disconnected)
                                                                          (connecting)
                                                                          (connected)
                                                                          (accepting)
                                                                          (accepted)
                                                                          (hello_sent)
                                                                          (peer_connection_accepted)
                                                                          (peer_connection_rejected)
                                                                          (negotiation_complete)
                                                                          (closing)
                                                                          (closed) )

FC_REFLECT( eos::net::peer_connection::timestamped_item_id, (item)(timestamp));
