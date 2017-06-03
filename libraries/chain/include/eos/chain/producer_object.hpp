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
#include <eos/chain/types.hpp>
#include <eos/chain/BlockchainConfiguration.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {
class producer_object : public chainbase::object<producer_object_type, producer_object> {
   OBJECT_CTOR(producer_object)

   id_type          id;
   account_id_type  owner;
   uint64_t         last_aslot = 0;
   public_key_type  signing_key;
   int64_t          total_missed = 0;
   uint32_t         last_confirmed_block_num = 0;

   /// The blockchain configuration values this producer recommends
   BlockchainConfiguration configuration;

   /**
    * @brief Update the tally of votes for the producer, while maintaining virtual time accounting
    *
    * This function will update @ref votes to @ref new_votes, while also updating all of the @ref virtual_time
    * accounting as well.
    *
    * @param new_votes The new tally of votes for this producer
    * @param current_virtual_time The current virtual time (see @ref virtual_time)
    */
   void update_votes(ShareType new_votes, UInt128 current_virtual_time);

   /// The total votes for this producer
   /// @warning Do not update this value directly; use @ref update_votes instead!
   ShareType votes;

   /**
    * These fields are used for the producer scheduling algorithm which uses virtual time to ensure that all producers
    * are given proportional time for producing blocks.
    *
    * @ref votes is used to determine speed. The @ref virtual_scheduled_time is the expected time at which this
    * producer should complete a virtual lap which is defined as the position equal to
    * config::ProducerScheduleLapLength
    *
    * virtual_scheduled_time = virtual_last_update + (config::ProducerScheduleLapLength - virtual_position) / votes
    *
    * Every time the number of votes changes the virtual_position and virtual_scheduled_time must update. There is a
    * global current virtual_scheduled_time which gets updated every time a producer is scheduled. To update the
    * virtual_position the following math is performed.
    *
    * virtual_position       = virtual_position + votes * (virtual_current_time - virtual_last_update)
    * virtual_last_update    = virtual_current_time
    * votes                  += delta_vote
    * virtual_scheduled_time = virtual_last_update + (config::ProducerScheduleLapLength - virtual_position) / votes
    *
    */
   ///@defgroup virtual_time Virtual Time Scheduling
   /// @warning Do not update these values directly; use @ref update_votes instead!
   ///@{
   UInt128 virtual_last_update_time;
   UInt128 virtual_position;
   UInt128 virtual_scheduled_time = std::numeric_limits<UInt128>::max();
   ///@}
};

struct by_key;
struct by_owner;
using producer_multi_index = chainbase::shared_multi_index_container<
   producer_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<producer_object, producer_object::id_type, &producer_object::id>>,
      ordered_unique<tag<by_owner>, member<producer_object, account_id_type, &producer_object::owner>>,
      ordered_non_unique<tag<by_key>, member<producer_object, public_key_type, &producer_object::signing_key>>
   >
>;

} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::producer_object, eos::chain::producer_multi_index)

FC_REFLECT(eos::chain::producer_object, (id)(owner)(last_aslot)(signing_key)(total_missed)(last_confirmed_block_num)
           (configuration)(virtual_last_update_time)(virtual_position)(virtual_scheduled_time))
