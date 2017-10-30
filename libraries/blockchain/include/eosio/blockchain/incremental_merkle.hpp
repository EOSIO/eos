#pragma once
#include <eosio/blockchain/types.hpp>

namespace eosio { namespace blockchain {

/**
 * A balanced merkle tree built in such that the set of leaf nodes can be
 * appended to without triggering the reconstruction of inner nodes that
 * represent a complete subset of previous nodes.
 *
 * to achieve this new nodes can either imply an set of future nodes
 * that achieve a balanced tree OR realize one of these future nodes.
 *
 * Once a sub-tree contains only realized nodes its sub-root will never
 * change.  This allows proofs based on this merkle to be very stable
 * after some time has past only needing to update or add a single
 * value to maintain validity.
 */
class incremental_merkle {
   public:
      incremental_merkle();

      /**
       * Add a new node to the set of leaf nodes
       *
       * @param digest - the node to append to the incremental merkle tree
       */
      void append(const digest_type& digest);

      /**
       * return the current root of the incremental merkle
       *
       * @return
       */
      digest_type get_root() const;

   private:
      uint64_t                _node_count;
      vector<digest_type>     _active_nodes;
};

} } /// eosio::blockchain
