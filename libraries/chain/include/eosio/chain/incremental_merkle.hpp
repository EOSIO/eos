#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/merkle.hpp>
#include <fc/io/raw.hpp>

namespace eosio { namespace chain {

namespace detail {

/**
 * given an unsigned integral number return the smallest
 * power-of-2 which is greater than or equal to the given number
 *
 * @param value - an unsigned integral
 * @return - the minimum power-of-2 which is >= value
 */
constexpr uint64_t next_power_of_2(uint64_t value) {
   value -= 1;
   value |= value >> 1;
   value |= value >> 2;
   value |= value >> 4;
   value |= value >> 8;
   value |= value >> 16;
   value |= value >> 32;
   value += 1;
   return value;
}

/**
 * Given a power-of-2 (assumed correct) return the number of leading zeros
 *
 * This is a classic count-leading-zeros in parallel without the necessary
 * math to make it safe for anything that is not already a power-of-2
 *
 * @param value - and integral power-of-2
 * @return the number of leading zeros
 */
constexpr int clz_power_2(uint64_t value) {
   int lz = 64;

   if (value) lz--;
   if (value & 0x00000000FFFFFFFFULL) lz -= 32;
   if (value & 0x0000FFFF0000FFFFULL) lz -= 16;
   if (value & 0x00FF00FF00FF00FFULL) lz -= 8;
   if (value & 0x0F0F0F0F0F0F0F0FULL) lz -= 4;
   if (value & 0x3333333333333333ULL) lz -= 2;
   if (value & 0x5555555555555555ULL) lz -= 1;

   return lz;
}

/**
 * Given a number of nodes return the depth required to store them
 * in a fully balanced binary tree.
 *
 * @param node_count - the number of nodes in the implied tree
 * @return the max depth of the minimal tree that stores them
 */
constexpr int calcluate_max_depth(uint64_t node_count) {
   if (node_count == 0) {
      return 0;
   }
   auto implied_count = next_power_of_2(node_count);
   return clz_power_2(implied_count) + 1;
}

template<typename ContainerA, typename ContainerB>
inline void move_nodes(ContainerA& to, const ContainerB& from) {
   to.clear();
   to.insert(to.begin(), from.begin(), from.end());
}

template<typename Container>
inline void move_nodes(Container& to, Container&& from) {
   to = std::forward<Container>(from);
}


} /// detail

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
template<typename DigestType, template<typename ...> class Container = vector, typename ...Args>
class incremental_merkle_impl {
   public:
      incremental_merkle_impl()
      :_node_count(0)
      {}

      template<typename Allocator>
      incremental_merkle_impl( Allocator&& alloc ):_active_nodes(forward<Allocator>(alloc)){}

      /*
      template<template<typename ...> class OtherContainer, typename ...OtherArgs>
      incremental_merkle_impl( incremental_merkle_impl<DigestType, OtherContainer, OtherArgs...>&& other )
      :_node_count(other._node_count)
      ,_active_nodes(other._active_nodes.begin(), other.active_nodes.end())
      {}

      incremental_merkle_impl( incremental_merkle_impl&& other )
      :_node_count(other._node_count)
      ,_active_nodes(std::forward<decltype(_active_nodes)>(other._active_nodes))
      {}
      */

      /**
       * Add a node to the incremental tree and recalculate the _active_nodes so they
       * are prepared for the next append.
       *
       * The algorithm for this is to start at the new node and retreat through the tree
       * for any node that is the concatenation of a fully-realized node and a partially
       * realized node we must record the value of the fully-realized node in the new
       * _active_nodes so that the next append can fetch it.   Fully realized nodes and
       * Fully implied nodes do not have an effect on the _active_nodes.
       *
       * For convention _AND_ to allow appends when the _node_count is a power-of-2, the
       * current root of the incremental tree is always appended to the end of the new
       * _active_nodes.
       *
       * In practice, this can be done iteratively by recording any "left" value that
       * is to be combined with an implied node.
       *
       * If the appended node is a "left" node in its pair, it will immediately push itself
       * into the new active nodes list.
       *
       * If the new node is a "right" node it will begin collapsing upward in the tree,
       * reading and discarding the "left" node data from the old active nodes list, until
       * it becomes a "left" node.  It must then push the "top" of its current collapsed
       * sub-tree into the new active nodes list.
       *
       * Once any value has been added to the new active nodes, all remaining "left" nodes
       * should be present in the order they are needed in the previous active nodes as an
       * artifact of the previous append.  As they are read from the old active nodes, they
       * will need to be copied in to the new active nodes list as they are still needed
       * for future appends.
       *
       * As a result, if an append collapses the entire tree while always being the "right"
       * node, the new list of active nodes will be empty and by definition the tree contains
       * a power-of-2 number of nodes.
       *
       * Regardless of the contents of the new active nodes list, the top "collapsed" value
       * is appended.  If this tree is _not_ a power-of-2 number of nodes, this node will
       * not be used in the next append but still serves as a conventional place to access
       * the root of the current tree. If this _is_ a power-of-2 number of nodes, this node
       * will be needed during then collapse phase of the next append so, it serves double
       * duty as a legitimate active node and the conventional storage location of the root.
       *
       *
       * @param digest - the node to add
       * @return - the new root
       */
      const DigestType& append(const DigestType& digest) {
         bool partial = false;
         auto max_depth = detail::calcluate_max_depth(_node_count + 1);
         auto current_depth = max_depth - 1;
         auto index = _node_count;
         auto top = digest;
         auto active_iter = _active_nodes.begin();
         auto updated_active_nodes = vector<DigestType>();
         updated_active_nodes.reserve(max_depth);

         while (current_depth > 0) {
            if (!(index & 0x1)) {
               // we are collapsing from a "left" value and an implied "right" creating a partial node

               // we only need to append this node if it is fully-realized and by definition
               // if we have encountered a partial node during collapse this cannot be
               // fully-realized
               if (!partial) {
                  updated_active_nodes.emplace_back(top);
               }

               // calculate the partially realized node value by implying the "right" value is identical
               // to the "left" value
               top = DigestType::hash(make_canonical_pair(top, top));
               partial = true;
            } else {
               // we are collapsing from a "right" value and an fully-realized "left"

               // pull a "left" value from the previous active nodes
               const auto& left_value = *active_iter;
               ++active_iter;

               // if the "right" value is a partial node we will need to copy the "left" as future appends still need it
               // otherwise, it can be dropped from the set of active nodes as we are collapsing a fully-realized node
               if (partial) {
                  updated_active_nodes.emplace_back(left_value);
               }

               // calculate the node
               top = DigestType::hash(make_canonical_pair(left_value, top));
            }

            // move up a level in the tree
            current_depth--;
            index = index >> 1;
         }

         // append the top of the collapsed tree (aka the root of the merkle)
         updated_active_nodes.emplace_back(top);

         // store the new active_nodes
         detail::move_nodes(_active_nodes, std::move(updated_active_nodes));

         // update the node count
         _node_count++;

         return _active_nodes.back();

      }

      /**l
       * return the current root of the incremental merkle
       *
       * @return
       */
      DigestType get_root() const {
         if (_node_count > 0) {
            return _active_nodes.back();
         } else {
            return DigestType();
         }
      }

   private:
      uint64_t                         _node_count;
      Container<DigestType, Args...>   _active_nodes;
};

typedef incremental_merkle_impl<digest_type>               incremental_merkle;
typedef incremental_merkle_impl<digest_type,shared_vector> shared_incremental_merkle;

} } /// eosio::chain
