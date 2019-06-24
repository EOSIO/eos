/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/reflect/reflect.hpp>
#include <eosio/topology_plugin/net_metrics.hpp>
#include <eosio/topology_plugin/node_descriptor.hpp>
#include <eosio/topology_plugin/link_descriptor.hpp>

namespace eosio{
   using namespace fc;
   /**
    * a watcher is an entity looking for datagrams containing metrics for a specific links
    */
   struct watcher {
      bool             adding;
      string           host_address;
      vector<link_id>  subjects;
      uint64_t         metrics;
   };

   struct link_sample {
      link_id    link;
      vector<pair<metric_kind,metric> > samples;
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


   using topo_data = static_variant<watcher, map_update, link_sample>;
   struct topology_message {
      node_id         origin;            // the origin of the message
      uint16_t        ttl;               // time-to-live, the number of forwards allowed
      vector<topo_data> payload;         // the collecion of data to share
   };

   typedef std::shared_ptr<topology_message> topology_message_ptr;
}

FC_REFLECT(eosio::watcher, (adding)(host_address)(subjects)(metrics))
FC_REFLECT(eosio::link_sample, (link)(samples))
FC_REFLECT(eosio::map_update, (add_nodes)(add_links)(drop_nodes)(drop_links))
FC_REFLECT(eosio::topology_message, (origin)(ttl)(payload))
