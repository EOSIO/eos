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

#include <eos/net/core_messages.hpp>
#include <eos/net/message.hpp>
#include <eos/net/peer_database.hpp>

#include <eos/chain/protocol/types.hpp>

#include <list>

namespace eos { namespace net {

  using fc::variant_object;
  using eos::chain::chain_id_type;

  namespace detail
  {
    class node_impl;
    struct node_impl_deleter
    {
      void operator()(node_impl*);
    };
  }

  // during network development, we need to track message propagation across the network
  // using a structure like this:
  struct message_propagation_data
  {
    fc::time_point received_time;
    fc::time_point validated_time;
    node_id_t originating_peer;
  };

   /**
    *  @class node_delegate
    *  @brief used by node reports status to client or fetch data from client
    */
   class node_delegate
   {
      public:
         virtual ~node_delegate(){}

         /**
          *  If delegate has the item, the network has no need to fetch it.
          */
         virtual bool has_item( const net::item_id& id ) = 0;

         /**
          *  @brief Called when a new block comes in from the network
          *
          *  @param sync_mode true if the message was fetched through the sync process, false during normal operation
          *  @returns true if this message caused the blockchain to switch forks, false if it did not
          *
          *  @throws exception if error validating the item, otherwise the item is
          *          safe to broadcast on.
          */
         virtual bool handle_block( const eos::net::block_message& blk_msg, bool sync_mode, 
                                    std::vector<fc::uint160_t>& contained_transaction_message_ids ) = 0;
         
         /**
          *  @brief Called when a new transaction comes in from the network
          *
          *  @throws exception if error validating the item, otherwise the item is
          *          safe to broadcast on.
          */
         virtual void handle_transaction( const eos::net::trx_message& trx_msg ) = 0;

         /**
          *  @brief Called when a new message comes in from the network other than a
          *         block or a transaction.  Currently there are no other possible 
          *         messages, so this should never be called.
          *
          *  @throws exception if error validating the item, otherwise the item is
          *          safe to broadcast on.
          */
         virtual void handle_message( const message& message_to_process ) = 0;

         /**
          *  Assuming all data elements are ordered in some way, this method should
          *  return up to limit ids that occur *after* from_id.
          *  On return, remaining_item_count will be set to the number of items
          *  in our blockchain after the last item returned in the result,
          *  or 0 if the result contains the last item in the blockchain
          */
         virtual std::vector<item_hash_t> get_block_ids(const std::vector<item_hash_t>& blockchain_synopsis,
                                                        uint32_t& remaining_item_count,
                                                        uint32_t limit = 2000) = 0;

         /**
          *  Given the hash of the requested data, fetch the body.
          */
         virtual message get_item( const item_id& id ) = 0;

         virtual chain_id_type get_chain_id()const = 0;

         /**
          * Returns a synopsis of the blockchain used for syncing.
          * This consists of a list of selected item hashes from our current preferred
          * blockchain, exponentially falling off into the past.  Horrible explanation.
          *
          * If the blockchain is empty, it will return the empty list.
          * If the blockchain has one block, it will return a list containing just that block.
          * If it contains more than one block:
          *   the first element in the list will be the hash of the highest numbered block that
          *       we cannot undo
          *   the second element will be the hash of an item at the half way point in the undoable
          *       segment of the blockchain
          *   the third will be ~3/4 of the way through the undoable segment of the block chain
          *   the fourth will be at ~7/8...
          *     &c.
          *   the last item in the list will be the hash of the most recent block on our preferred chain
          */
         virtual std::vector<item_hash_t> get_blockchain_synopsis(const item_hash_t& reference_point, 
                                                                  uint32_t number_of_blocks_after_reference_point) = 0;

         /**
          *  Call this after the call to handle_message succeeds.
          *
          *  @param item_type the type of the item we're synchronizing, will be the same as item passed to the sync_from() call
          *  @param item_count the number of items known to the node that haven't been sent to handle_item() yet.
          *                    After `item_count` more calls to handle_item(), the node will be in sync
          */
         virtual void     sync_status( uint32_t item_type, uint32_t item_count ) = 0;

         /**
          *  Call any time the number of connected peers changes.
          */
         virtual void     connection_count_changed( uint32_t c ) = 0;

         virtual uint32_t get_block_number(const item_hash_t& block_id) = 0;

         /**
          * Returns the time a block was produced (if block_id = 0, returns genesis time).
          * If we don't know about the block, returns time_point_sec::min()
          */
         virtual fc::time_point_sec get_block_time(const item_hash_t& block_id) = 0;

         virtual item_hash_t get_head_block_id() const = 0;

         virtual uint32_t estimate_last_known_fork_from_git_revision_timestamp(uint32_t unix_timestamp) const = 0;

         virtual void error_encountered(const std::string& message, const fc::oexception& error) = 0;
         virtual uint8_t get_current_block_interval_in_seconds() const = 0;

   };

   /**
    *  Information about connected peers that the client may want to make
    *  available to the user.
    */
   struct peer_status
   {
      uint32_t         version;
      fc::ip::endpoint host;
      /** info contains the fields required by bitcoin-rpc's getpeerinfo call, we will likely
          extend it with our own fields. */
      fc::variant_object info;
   };

   /**
    *  @class node
    *  @brief provides application independent P2P broadcast and data synchronization
    *
    *  Unanswered questions:
    *    when does the node start establishing network connections and accepting peers?
    *    we don't have enough info to start synchronizing until sync_from() is called,
    *    would we have any reason to connect before that?
    */
   class node : public std::enable_shared_from_this<node>
   {
      public:
        node(const std::string& user_agent);
        ~node();

        void close();

        void      set_node_delegate( node_delegate* del );

        void      load_configuration( const fc::path& configuration_directory );

        virtual void      listen_to_p2p_network();
        virtual void      connect_to_p2p_network();

        /**
         *  Add endpoint to internal level_map database of potential nodes
         *  to attempt to connect to.  This database is consulted any time
         *  the number connected peers falls below the target.
         */
        void      add_node( const fc::ip::endpoint& ep );

        /**
         *  Attempt to connect to the specified endpoint immediately.
         */
        virtual void connect_to_endpoint( const fc::ip::endpoint& ep );

        /**
         *  Specifies the network interface and port upon which incoming
         *  connections should be accepted.
         */
        void      listen_on_endpoint( const fc::ip::endpoint& ep, bool wait_if_not_available );

        /**
         *  Call with true to enable listening for incoming connections
         */
        void accept_incoming_connections(bool accept);

        /**
         *  Specifies the port upon which incoming connections should be accepted.
         *  @param port the port to listen on
         *  @param wait_if_not_available if true and the port is not available, enter a
         *                               sleep and retry loop to wait for it to become
         *                               available.  If false and the port is not available,
         *                               just choose a random available port
         */
        void      listen_on_port(uint16_t port, bool wait_if_not_available);

        /**
         * Returns the endpoint the node is listening on.  This is usually the same
         * as the value previously passed in to listen_on_endpoint, unless we
         * were unable to bind to that port.
         */
        virtual fc::ip::endpoint get_actual_listening_endpoint() const;

        /**
         *  @return a list of peers that are currently connected.
         */
        std::vector<peer_status> get_connected_peers() const;

        /** return the number of peers we're actively connected to */
        virtual uint32_t get_connection_count() const;

        /**
         *  Add message to outgoing inventory list, notify peers that
         *  I have a message ready.
         */
        virtual void  broadcast( const message& item_to_broadcast );
        virtual void  broadcast_transaction( const signed_transaction& trx )
        {
           broadcast( trx_message(trx) );
        }

        /**
         *  Node starts the process of fetching all items after item_id of the
         *  given item_type.   During this process messages are not broadcast.
         */
        virtual void      sync_from(const item_id& current_head_block, const std::vector<uint32_t>& hard_fork_block_numbers);

        bool      is_connected() const;

        void set_advanced_node_parameters(const fc::variant_object& params);
        fc::variant_object get_advanced_node_parameters();
        message_propagation_data get_transaction_propagation_data(const eos::chain::transaction_id_type& transaction_id);
        message_propagation_data get_block_propagation_data(const eos::chain::block_id_type& block_id);
        node_id_t get_node_id() const;
        void set_allowed_peers(const std::vector<node_id_t>& allowed_peers);

        /**
         * Instructs the node to forget everything in its peer database, mostly for debugging
         * problems where nodes are failing to connect to the network
         */
        void clear_peer_database();

        void set_total_bandwidth_limit(uint32_t upload_bytes_per_second, uint32_t download_bytes_per_second);

        fc::variant_object network_get_info() const;
        fc::variant_object network_get_usage_stats() const;

        std::vector<potential_peer_record> get_potential_peers() const;

        void disable_peer_advertising();
        fc::variant_object get_call_statistics() const;
      private:
        std::unique_ptr<detail::node_impl, detail::node_impl_deleter> my;
   };

    class simulated_network : public node
    {
    public:
      ~simulated_network();
      simulated_network(const std::string& user_agent) : node(user_agent) {}
      void      listen_to_p2p_network() override {}
      void      connect_to_p2p_network() override {}
      void      connect_to_endpoint(const fc::ip::endpoint& ep) override {}

      fc::ip::endpoint get_actual_listening_endpoint() const override { return fc::ip::endpoint(); }

      void      sync_from(const item_id& current_head_block, const std::vector<uint32_t>& hard_fork_block_numbers) override {}
      void      broadcast(const message& item_to_broadcast) override;
      void      add_node_delegate(node_delegate* node_delegate_to_add);

      virtual uint32_t get_connection_count() const override { return 8; }
    private:
      struct node_info;
      void message_sender(node_info* destination_node);
      std::list<node_info*> network_nodes;
    };


   typedef std::shared_ptr<node> node_ptr;
   typedef std::shared_ptr<simulated_network> simulated_network_ptr;

} } // eos::net

FC_REFLECT(eos::net::message_propagation_data, (received_time)(validated_time)(originating_peer));
FC_REFLECT( eos::net::peer_status, (version)(host)(info) );
