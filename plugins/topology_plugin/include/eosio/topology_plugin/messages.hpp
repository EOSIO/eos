/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/reflect/reflect.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/topology_plugin/net_metrics.hpp>
#include <eosio/topology_plugin/node_descriptor.hpp>
#include <eosio/topology_plugin/link_descriptor.hpp>

namespace eosio{
   using namespace fc;
   using namespace chain;

   struct link_sample {
      link_id    link;
      vector<topology_sample> down;
      vector<topology_sample> up;
   };

      /**
    * Map_update is used to communicate state changes that occur when nodes or links are added or removed
    */
   struct map_update {
      vector<node_descriptor> add_nodes; // list of nodes that are added to the network
      vector<link_descriptor> add_links; // list of links that are added to the network
      vector<node_id> drop_nodes;        // list of nodes to remove from the network
      vector<link_id> drop_links;        // list of links to remove from the network
   };

   struct fork_info {
      uint32_t      when;
      account_name  producer;
      block_id_type fork_base;
      uint16_t      depth;
      string        report;
   };

   using topology_data = static_variant<map_update, link_sample, fork_info>;

   struct topology_message {
      node_id               origin;            // the origin of the message
      node_id               destination;       // set to 0 for bcast
      mutable uint16_t      fwds;              // count of the number of forwards used
      uint16_t              ttl;               // time-to-live, the number of forwards allowed
      vector<topology_data> payload;           // the collecion of data to share
   };

   using topology_message_ptr = std::shared_ptr<topology_message>;
}

FC_REFLECT(eosio::link_sample, (link)(down)(up))
FC_REFLECT(eosio::map_update, (add_nodes)(add_links)(drop_nodes)(drop_links))
FC_REFLECT(eosio::fork_info,  (when)(producer)(fork_base)(depth)(report))
FC_REFLECT(eosio::topology_message, (origin)(destination)(fwds)(ttl)(payload))
