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
   struct watch_update {
      bool             adding;
      watcher_def      watcher;
   };

   using sample_metric = pair<metric_kind,uint32_t>;

   struct link_sample {
      link_id    link;
      vector<pair<metric_kind,uint32_t> > down;
      vector<pair<metric_kind,uint32_t> > up;
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


   using topo_data = static_variant<watch_update, map_update, link_sample>;
   struct topology_message {
      node_id         origin;            // the origin of the message
      node_id         destination;       // set to 0 for bcast
      uint16_t        fwds;              // count of the number of forwards used
      uint16_t        ttl;               // time-to-live, the number of forwards allowed
      vector<topo_data> payload;         // the collecion of data to share
   };

   typedef std::shared_ptr<topology_message> topology_message_ptr;
}

FC_REFLECT(eosio::watch_update, (adding)(watcher))
FC_REFLECT(eosio::link_sample, (link)(down)(up))
FC_REFLECT(eosio::map_update, (add_nodes)(add_links)(drop_nodes)(drop_links))
FC_REFLECT(eosio::topology_message, (origin)(destination)(fwds)(ttl)(payload))
