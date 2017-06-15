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
#include <fc/uint128.hpp>
#include <fc/array.hpp>

#include <eos/chain/types.hpp>
#include <eos/chain/BlockchainConfiguration.hpp>

#include <chainbase/chainbase.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {

   /**
    * @class global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are set by committee_members to tune the blockchain parameters.
    */
   class global_property_object : public chainbase::object<global_property_object_type, global_property_object>
   {
      OBJECT_CTOR(global_property_object)

      id_type id;
      BlockchainConfiguration configuration;
      std::array<AccountName, config::BlocksPerRound> active_producers;
   };



   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
    * current values of global blockchain properties.
    */
   class dynamic_global_property_object : public chainbase::object<dynamic_global_property_object_type, dynamic_global_property_object>
   {
        OBJECT_CTOR(dynamic_global_property_object)

        id_type           id;
        uint32_t          head_block_number = 0;
        block_id_type     head_block_id;
        time_point_sec    time;
        AccountName       current_producer;
        uint32_t          accounts_registered_this_interval = 0;
        
        /**
         * The current absolute slot number.  Equal to the total
         * number of slots since genesis.  Also equal to the total
         * number of missed slots plus head_block_number.
         */
        uint64_t          current_absolute_slot = 0;
        
        /**
         * Bitmap used to compute producer participation. Stores
         * a high bit for each generated block, a low bit for
         * each missed block. Least significant bit is most
         * recent block.
         *
         * NOTE: This bitmap always excludes the head block,
         * which, by definition, exists. The least significant
         * bit corresponds to the block with number
         * head_block_num()-1
         *
         * e.g. if the least significant 5 bits were 10011, it
         * would indicate that the last two blocks prior to the
         * head block were produced, the two before them were
         * missed, and the one before that was produced.
         */
        uint64_t recent_slots_filled;
        
        uint32_t last_irreversible_block_num = 0;
   };

   using global_property_multi_index = chainbase::shared_multi_index_container<
      global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(global_property_object, global_property_object::id_type, id)
         >
      >
   >;

   using dynamic_global_property_multi_index = chainbase::shared_multi_index_container<
      dynamic_global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(dynamic_global_property_object, dynamic_global_property_object::id_type, id)
         >
      >
   >;

}}

CHAINBASE_SET_INDEX_TYPE(eos::chain::global_property_object, eos::chain::global_property_multi_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::dynamic_global_property_object,
                         eos::chain::dynamic_global_property_multi_index)

FC_REFLECT(eos::chain::dynamic_global_property_object,
           (head_block_number)
           (head_block_id)
           (time)
           (current_producer)
           (accounts_registered_this_interval)
           (current_absolute_slot)
           (recent_slots_filled)
           (last_irreversible_block_num)
          )

FC_REFLECT(eos::chain::global_property_object,
           (configuration)
           (active_producers)
          )
