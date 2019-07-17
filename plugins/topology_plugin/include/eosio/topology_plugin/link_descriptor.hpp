/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/reflect/reflect.hpp>
#include <eosio/topology_plugin/node_descriptor.hpp>

namespace eosio{
   using link_id = uint64_t;

   enum link_role
      {
       blocks       = 0x1,  // link is used to send blocks to peers
       transactions = 0x2,  // link is used to sent trx to peers
       control      = 0x4,   // link is used to send control messages to peers
       combined     = 0x7
      };

   struct link_descriptor {
      link_id      my_id;    // the computed unique identifier for this link
      node_id      active;   // the identifier of the "client" connector
      node_id      passive;  // the identifier of the "server" connector
      uint16_t     hops;     // the number of hops detected via the icmp TTL counter
      link_role    role;     // how the link is used
   };

}

FC_REFLECT_ENUM( eosio::link_role, (blocks)(transactions)(control)(combined))
FC_REFLECT( eosio::link_descriptor, (my_id)(active)(passive)(hops)(role))
