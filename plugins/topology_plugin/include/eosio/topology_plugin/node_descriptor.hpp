/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/reflect/reflect.hpp>
#include <eosio/chain/name.hpp>
#include <fc/crypto/sha256.hpp>

namespace eosio{
   using node_id = uint64_t;

  enum node_status
      {
       scheduled,    // a producer that is in the 21
       standby,      // a producer that is not in the 21
       clone,        // a clone of that hot swappable
       running,      // a node that is not a producer
       no_topology,  // a node has not loaded the topology plugin
       rejecting     // the node is offline
      };

   enum node_role
      {
       producer = 0x01,  // a node containing named block producers
       backup   = 0x02,  // a redundant producer
       api      = 0x04,  // a node that accepts connections from the internet
       full     = 0x08,  // a "full" node
       gateway  = 0x10,  // a node that spans two network
       special  = 0x20   // a node that includes special purpose modules
      };

   struct node_descriptor {
      node_id      my_id; // a copy of the computed unique id for this node
      std::string  location;      // a combination of the bp_name and p2p address
      node_role    role;          // see list above
      node_status  status;        // see list above
      std::string  version;       // software version running
      std::vector<chain::name> producers; // zero or more named producers hosted in this node.
   };


}

FC_REFLECT_ENUM( eosio::node_status, (scheduled)(standby)(clone)(running)(no_topology)(rejecting) )
FC_REFLECT_ENUM( eosio::node_role, (producer)(backup)(api)(full)(gateway)(special) )
FC_REFLECT( eosio::node_descriptor, (my_id)(location)(role)(status)(version)(producers))
