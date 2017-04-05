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

#include <eos/net/config.hpp>
#include <eos/chain/protocol/block.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/network/ip.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/time.hpp>
#include <fc/variant_object.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/enum_type.hpp>


#include <vector>

namespace eos { namespace net {
  using eos::chain::signed_transaction;
  using eos::chain::block_id_type;
  using eos::chain::transaction_id_type;
  using eos::chain::signed_block;

  typedef fc::ecc::public_key_data node_id_t;
  typedef fc::ripemd160 item_hash_t;
  struct item_id
  {
      uint32_t      item_type;
      item_hash_t   item_hash;

      item_id() {}
      item_id(uint32_t type, const item_hash_t& hash) :
        item_type(type),
        item_hash(hash)
      {}
      bool operator==(const item_id& other) const
      {
        return item_type == other.item_type &&
               item_hash == other.item_hash;
      }
  };

  enum core_message_type_enum
  {
    trx_message_type                             = 1000,
    block_message_type                           = 1001,
    core_message_type_first                      = 5000,
    item_ids_inventory_message_type              = 5001,
    blockchain_item_ids_inventory_message_type   = 5002,
    fetch_blockchain_item_ids_message_type       = 5003,
    fetch_items_message_type                     = 5004,
    item_not_available_message_type              = 5005,
    hello_message_type                           = 5006,
    connection_accepted_message_type             = 5007,
    connection_rejected_message_type             = 5008,
    address_request_message_type                 = 5009,
    address_message_type                         = 5010,
    closing_connection_message_type              = 5011,
    current_time_request_message_type            = 5012,
    current_time_reply_message_type              = 5013,
    check_firewall_message_type                  = 5014,
    check_firewall_reply_message_type            = 5015,
    get_current_connections_request_message_type = 5016,
    get_current_connections_reply_message_type   = 5017,
    core_message_type_last                       = 5099
  };

  const uint32_t core_protocol_version = EOS_NET_PROTOCOL_VERSION;

   struct trx_message
   {
      static const core_message_type_enum type;

      signed_transaction trx;
      trx_message() {}
      trx_message(signed_transaction transaction) :
        trx(std::move(transaction))
      {}
   };

   struct block_message
   {
      static const core_message_type_enum type;

      block_message(){}
      block_message(const signed_block& blk )
      :block(blk),block_id(blk.id()){}

      signed_block    block;
      block_id_type   block_id;

   };

  struct item_ids_inventory_message
  {
    static const core_message_type_enum type;

    uint32_t item_type;
    std::vector<item_hash_t> item_hashes_available;

    item_ids_inventory_message() {}
    item_ids_inventory_message(uint32_t item_type, const std::vector<item_hash_t>& item_hashes_available) :
      item_type(item_type),
      item_hashes_available(item_hashes_available)
    {}
  };

  struct blockchain_item_ids_inventory_message
  {
    static const core_message_type_enum type;

    uint32_t total_remaining_item_count;
    uint32_t item_type;
    std::vector<item_hash_t> item_hashes_available;

    blockchain_item_ids_inventory_message() {}
    blockchain_item_ids_inventory_message(uint32_t total_remaining_item_count,
                                          uint32_t item_type,
                                          const std::vector<item_hash_t>& item_hashes_available) :
      total_remaining_item_count(total_remaining_item_count),
      item_type(item_type),
      item_hashes_available(item_hashes_available)
    {}
  };

  struct fetch_blockchain_item_ids_message
  {
    static const core_message_type_enum type;

    uint32_t item_type;
    std::vector<item_hash_t> blockchain_synopsis;

    fetch_blockchain_item_ids_message() {}
    fetch_blockchain_item_ids_message(uint32_t item_type, const std::vector<item_hash_t>& blockchain_synopsis) :
      item_type(item_type),
      blockchain_synopsis(blockchain_synopsis)
    {}
  };

  struct fetch_items_message
  {
    static const core_message_type_enum type;

    uint32_t item_type;
    std::vector<item_hash_t> items_to_fetch;

    fetch_items_message() {}
    fetch_items_message(uint32_t item_type, const std::vector<item_hash_t>& items_to_fetch) :
      item_type(item_type),
      items_to_fetch(items_to_fetch)
    {}
  };

  struct item_not_available_message
  {
    static const core_message_type_enum type;

    item_id requested_item;

    item_not_available_message() {}
    item_not_available_message(const item_id& requested_item) :
      requested_item(requested_item)
    {}
  };

  struct hello_message
  {
    static const core_message_type_enum type;

    std::string                user_agent;
    uint32_t                   core_protocol_version;
    fc::ip::address            inbound_address;
    uint16_t                   inbound_port;
    uint16_t                   outbound_port;
    node_id_t                  node_public_key;
    fc::ecc::compact_signature signed_shared_secret;
    fc::sha256                 chain_id;
    fc::variant_object         user_data;

    hello_message() {}
    hello_message(const std::string& user_agent,
                  uint32_t core_protocol_version,
                  const fc::ip::address& inbound_address,
                  uint16_t inbound_port,
                  uint16_t outbound_port,
                  const node_id_t& node_public_key,
                  const fc::ecc::compact_signature& signed_shared_secret,
                  const fc::sha256& chain_id_arg,
                  const fc::variant_object& user_data ) :
      user_agent(user_agent),
      core_protocol_version(core_protocol_version),
      inbound_address(inbound_address),
      inbound_port(inbound_port),
      outbound_port(outbound_port),
      node_public_key(node_public_key),
      signed_shared_secret(signed_shared_secret),
      chain_id(chain_id_arg),
      user_data(user_data)
    {}
  };

  struct connection_accepted_message
  {
    static const core_message_type_enum type;

    connection_accepted_message() {}
  };

  enum class rejection_reason_code { unspecified,
                                     different_chain,
                                     already_connected,
                                     connected_to_self,
                                     not_accepting_connections,
                                     blocked,
                                     invalid_hello_message,
                                     client_too_old };

  struct connection_rejected_message
  {
    static const core_message_type_enum type;

    std::string                                   user_agent;
    uint32_t                                      core_protocol_version;
    fc::ip::endpoint                              remote_endpoint;
    std::string                                   reason_string;
    fc::enum_type<uint8_t, rejection_reason_code> reason_code;

    connection_rejected_message() {}
    connection_rejected_message(const std::string& user_agent, uint32_t core_protocol_version,
                                const fc::ip::endpoint& remote_endpoint, rejection_reason_code reason_code,
                                const std::string& reason_string) :
      user_agent(user_agent),
      core_protocol_version(core_protocol_version),
      remote_endpoint(remote_endpoint),
      reason_string(reason_string),
      reason_code(reason_code)
    {}
  };

  struct address_request_message
  {
    static const core_message_type_enum type;

    address_request_message() {}
  };

  enum class peer_connection_direction { unknown, inbound, outbound };
  enum class firewalled_state { unknown, firewalled, not_firewalled };

  struct address_info
  {
    fc::ip::endpoint          remote_endpoint;
    fc::time_point_sec        last_seen_time;
    fc::microseconds          latency;
    node_id_t                 node_id;
    fc::enum_type<uint8_t, peer_connection_direction> direction;
    fc::enum_type<uint8_t, firewalled_state> firewalled;

    address_info() {}
    address_info(const fc::ip::endpoint& remote_endpoint,
                 const fc::time_point_sec last_seen_time,
                 const fc::microseconds latency,
                 const node_id_t& node_id,
                 peer_connection_direction direction,
                 firewalled_state firewalled) :
      remote_endpoint(remote_endpoint),
      last_seen_time(last_seen_time),
      latency(latency),
      node_id(node_id),
      direction(direction),
      firewalled(firewalled)
    {}
  };

  struct address_message
  {
    static const core_message_type_enum type;

    std::vector<address_info> addresses;
  };

  struct closing_connection_message
  {
    static const core_message_type_enum type;

    std::string        reason_for_closing;
    bool               closing_due_to_error;
    fc::oexception     error;

    closing_connection_message() : closing_due_to_error(false) {}
    closing_connection_message(const std::string& reason_for_closing,
                               bool closing_due_to_error = false,
                               const fc::oexception& error = fc::oexception()) :
      reason_for_closing(reason_for_closing),
      closing_due_to_error(closing_due_to_error),
      error(error)
    {}
  };

  struct current_time_request_message
  {
    static const core_message_type_enum type;
    fc::time_point request_sent_time;

    current_time_request_message(){}
    current_time_request_message(const fc::time_point request_sent_time) :
      request_sent_time(request_sent_time)
    {}
  };

  struct current_time_reply_message
  {
    static const core_message_type_enum type;
    fc::time_point request_sent_time;
    fc::time_point request_received_time;
    fc::time_point reply_transmitted_time;

    current_time_reply_message(){}
    current_time_reply_message(const fc::time_point request_sent_time,
                               const fc::time_point request_received_time,
                               const fc::time_point reply_transmitted_time = fc::time_point()) :
      request_sent_time(request_sent_time),
      request_received_time(request_received_time),
      reply_transmitted_time(reply_transmitted_time)
    {}
  };

  struct check_firewall_message
  {
    static const core_message_type_enum type;
    node_id_t node_id;
    fc::ip::endpoint endpoint_to_check;
  };

  enum class firewall_check_result
  {
    unable_to_check,
    unable_to_connect,
    connection_successful
  };

  struct check_firewall_reply_message
  {
    static const core_message_type_enum type;
    node_id_t node_id;
    fc::ip::endpoint endpoint_checked;
    fc::enum_type<uint8_t, firewall_check_result> result;
  };

  struct get_current_connections_request_message
  {
    static const core_message_type_enum type;
  };

  struct current_connection_data
  {
    uint32_t           connection_duration; // in seconds
    fc::ip::endpoint   remote_endpoint;
    node_id_t          node_id;
    fc::microseconds   clock_offset;
    fc::microseconds   round_trip_delay;
    fc::enum_type<uint8_t, peer_connection_direction> connection_direction;
    fc::enum_type<uint8_t, firewalled_state> firewalled;
    fc::variant_object user_data;
  };

  struct get_current_connections_reply_message
  {
    static const core_message_type_enum type;
    uint32_t upload_rate_one_minute;
    uint32_t download_rate_one_minute;
    uint32_t upload_rate_fifteen_minutes;
    uint32_t download_rate_fifteen_minutes;
    uint32_t upload_rate_one_hour;
    uint32_t download_rate_one_hour;
    std::vector<current_connection_data> current_connections;
  };


} } // eos::net

FC_REFLECT_ENUM( eos::net::core_message_type_enum,
                 (trx_message_type)
                 (block_message_type)
                 (core_message_type_first)
                 (item_ids_inventory_message_type)
                 (blockchain_item_ids_inventory_message_type)
                 (fetch_blockchain_item_ids_message_type)
                 (fetch_items_message_type)
                 (item_not_available_message_type)
                 (hello_message_type)
                 (connection_accepted_message_type)
                 (connection_rejected_message_type)
                 (address_request_message_type)
                 (address_message_type)
                 (closing_connection_message_type)
                 (current_time_request_message_type)
                 (current_time_reply_message_type)
                 (check_firewall_message_type)
                 (check_firewall_reply_message_type)
                 (get_current_connections_request_message_type)
                 (get_current_connections_reply_message_type)
                 (core_message_type_last) )

FC_REFLECT( eos::net::trx_message, (trx) )
FC_REFLECT( eos::net::block_message, (block)(block_id) )

FC_REFLECT( eos::net::item_id, (item_type)
                               (item_hash) )
FC_REFLECT( eos::net::item_ids_inventory_message, (item_type)
                                                  (item_hashes_available) )
FC_REFLECT( eos::net::blockchain_item_ids_inventory_message, (total_remaining_item_count)
                                                             (item_type)
                                                             (item_hashes_available) )
FC_REFLECT( eos::net::fetch_blockchain_item_ids_message, (item_type)
                                                         (blockchain_synopsis) )
FC_REFLECT( eos::net::fetch_items_message, (item_type)
                                           (items_to_fetch) )
FC_REFLECT( eos::net::item_not_available_message, (requested_item) )
FC_REFLECT( eos::net::hello_message, (user_agent)
                                     (core_protocol_version)
                                     (inbound_address)
                                     (inbound_port)
                                     (outbound_port)
                                     (node_public_key)
                                     (signed_shared_secret)
                                     (chain_id)
                                     (user_data) )

FC_REFLECT_EMPTY( eos::net::connection_accepted_message )
FC_REFLECT_ENUM(eos::net::rejection_reason_code, (unspecified)
                                                 (different_chain)
                                                 (already_connected)
                                                 (connected_to_self)
                                                 (not_accepting_connections)
                                                 (blocked)
                                                 (invalid_hello_message)
                                                 (client_too_old))
FC_REFLECT( eos::net::connection_rejected_message, (user_agent)
                                                   (core_protocol_version)
                                                   (remote_endpoint)
                                                   (reason_code)
                                                   (reason_string))
FC_REFLECT_EMPTY( eos::net::address_request_message )
FC_REFLECT( eos::net::address_info, (remote_endpoint)
                                    (last_seen_time)
                                    (latency)
                                    (node_id)
                                    (direction)
                                    (firewalled) )
FC_REFLECT( eos::net::address_message, (addresses) )
FC_REFLECT( eos::net::closing_connection_message, (reason_for_closing)
                                                  (closing_due_to_error)
                                                  (error) )
FC_REFLECT_ENUM(eos::net::peer_connection_direction, (unknown)
                                                     (inbound)
                                                     (outbound))
FC_REFLECT_ENUM(eos::net::firewalled_state, (unknown)
                                            (firewalled)
                                            (not_firewalled))

FC_REFLECT(eos::net::current_time_request_message, (request_sent_time))
FC_REFLECT(eos::net::current_time_reply_message, (request_sent_time)
                                                 (request_received_time)
                                                 (reply_transmitted_time))
FC_REFLECT_ENUM(eos::net::firewall_check_result, (unable_to_check)
                                                 (unable_to_connect)
                                                 (connection_successful))
FC_REFLECT(eos::net::check_firewall_message, (node_id)(endpoint_to_check))
FC_REFLECT(eos::net::check_firewall_reply_message, (node_id)(endpoint_checked)(result))
FC_REFLECT_EMPTY(eos::net::get_current_connections_request_message)
FC_REFLECT(eos::net::current_connection_data, (connection_duration)
                                              (remote_endpoint)
                                              (node_id)
                                              (clock_offset)
                                              (round_trip_delay)
                                              (connection_direction)
                                              (firewalled)
                                              (user_data))
FC_REFLECT(eos::net::get_current_connections_reply_message, (upload_rate_one_minute)
                                                            (download_rate_one_minute)
                                                            (upload_rate_fifteen_minutes)
                                                            (download_rate_fifteen_minutes)
                                                            (upload_rate_one_hour)
                                                            (download_rate_one_hour)
                                                            (current_connections))

#include <unordered_map>
#include <fc/crypto/city.hpp>
#include <fc/crypto/sha224.hpp>
namespace std
{
    template<>
    struct hash<eos::net::item_id>
    {
       size_t operator()(const eos::net::item_id& item_to_hash) const
       {
          return fc::city_hash_size_t((char*)&item_to_hash, sizeof(item_to_hash));
       }
    };
}
