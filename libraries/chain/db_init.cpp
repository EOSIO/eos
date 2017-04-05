/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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

#include <eos/chain/database.hpp>

#include <eos/chain/account_object.hpp>
#include <eos/chain/block_summary_object.hpp>
#include <eos/chain/chain_property_object.hpp>
#include <eos/chain/global_property_object.hpp>
#include <eos/chain/operation_history_object.hpp>
#include <eos/chain/transaction_object.hpp>
#include <eos/chain/producer_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/crypto/digest.hpp>

#include <boost/algorithm/string.hpp>

namespace eos { namespace chain {

// C++ requires that static class variables declared and initialized
// in headers must also have a definition in a single source file,
// else linker errors will occur [1].
//
// The purpose of this source file is to collect such definitions in
// a single place.
//
// [1] http://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char

void database::initialize_evaluators()
{
   _operation_evaluators.resize(255);
   // TODO: Figure out how to do this
}

void database::initialize_indexes()
{
   add_index<account_multi_index>();
   add_index<global_property_multi_index>();
   add_index<dynamic_global_property_multi_index>();
   add_index<block_summary_multi_index>();
   add_index<transaction_multi_index>();
   add_index<producer_multi_index>();
   add_index<chain_property_multi_index>();
}

void database::init_genesis(const genesis_state_type& genesis_state)
{ try {
   FC_ASSERT( genesis_state.initial_timestamp != time_point_sec(), "Must initialize genesis timestamp." );
   FC_ASSERT( genesis_state.initial_timestamp.sec_since_epoch() % EOS_DEFAULT_BLOCK_INTERVAL == 0,
              "Genesis timestamp must be divisible by EOS_DEFAULT_BLOCK_INTERVAL." );
   FC_ASSERT(genesis_state.initial_producer_count >= genesis_state.initial_producers.size(),
             "Initial producer count is ${c} but only ${w} producers were defined.",
             ("c", genesis_state.initial_producer_count)("w", genesis_state.initial_producers.size()));

   struct auth_inhibitor {
      auth_inhibitor(database& db) : db(db), old_flags(db.node_properties().skip_flags)
      { db.node_properties().skip_flags |= skip_authority_check; }
      ~auth_inhibitor()
      { db.node_properties().skip_flags = old_flags; }
   private:
      database& db;
      uint32_t old_flags;
   } inhibitor(*this);

   // Create initial accounts
   for (const auto& acct : genesis_state.initial_accounts) {
      create<account_object>([&acct](account_object& a) {
         a.name = acct.name.c_str();
         a.active_key = acct.active_key;
         a.owner_key = acct.owner_key;
      });
   }
   // Create initial producers
   std::vector<producer_id_type> initialProducers;
   for (const auto& producer : genesis_state.initial_producers) {
      auto owner = find<account_object, by_name>(producer.owner_name);
      FC_ASSERT(owner != nullptr, "Producer belongs to an unknown account: ${acct}", ("acct", producer.owner_name));
      auto id = create<producer_object>([&producer](producer_object& w) {
         w.signing_key = producer.block_signing_key;
         w.owner_name = producer.owner_name.c_str();
      }).id;
      initialProducers.push_back(id);
   }

   transaction_evaluation_state genesis_eval_state(this);

   // Initialize block summary index

   chain_id_type chain_id = genesis_state.compute_chain_id();

   // Create global properties
   create<global_property_object>([&](global_property_object& p) {
       p.parameters = genesis_state.initial_parameters;
       p.active_producers = initialProducers;
   });
   create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
      p.time = genesis_state.initial_timestamp;
      p.dynamic_flags = 0;
      p.recent_slots_filled = fc::uint128::max_value();
   });

   FC_ASSERT( (genesis_state.immutable_parameters.min_producer_count & 1) == 1, "min_producer_count must be odd" );

   create<chain_property_object>([&](chain_property_object& p)
   {
      p.chain_id = chain_id;
      p.immutable_parameters = genesis_state.immutable_parameters;
   } );
   create<block_summary_object>([&](block_summary_object&) {});

   //TODO: Figure out how to do this
   // Create initial accounts
   // Create initial producers
   // Set active producers
} FC_CAPTURE_AND_RETHROW() }

} }
