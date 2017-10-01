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
#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>
#include <boost/signals2/shared_connection_block.hpp>

#include <eos/chain/account_object.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/authority_checker.hpp>

#include <eos/utilities/tempdir.hpp>

#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/native_contract/native_contract_chain_administrator.hpp>
#include <eos/native_contract/objects.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/range/adaptor/map.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

uint32_t EOS_TESTING_GENESIS_TIMESTAMP = 1431700005;

namespace eos { namespace chain {
   using namespace native::eos;
   using namespace native;

testing_fixture::testing_fixture() {
   default_genesis_state.initial_timestamp = fc::time_point_sec(EOS_TESTING_GENESIS_TIMESTAMP);
   for (int i = 0; i < config::BlocksPerRound; ++i) {
      auto name = std::string("inita"); name.back()+=i;
      auto private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name));
      public_key_type public_key = private_key.get_public_key();
      default_genesis_state.initial_accounts.emplace_back(name, 0, 100000, public_key, public_key);
      store_private_key(private_key);

      private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name + ".producer"));
      public_key = private_key.get_public_key();
      default_genesis_state.initial_producers.emplace_back(name, public_key);
      store_private_key(private_key);
   }
}

fc::path testing_fixture::get_temp_dir(std::string id) {
   if (id.empty()) {
      anonymous_temp_dirs.emplace_back();
      return anonymous_temp_dirs.back().path();
   }
   if (named_temp_dirs.count(id))
      return named_temp_dirs[id].path();
   return named_temp_dirs.emplace(std::make_pair(id, fc::temp_directory())).first->second.path();
}

const native_contract::genesis_state_type& testing_fixture::genesis_state() const {
   return default_genesis_state;
}

native_contract::genesis_state_type& testing_fixture::genesis_state() {
   return default_genesis_state;
}

void testing_fixture::store_private_key(const private_key_type& key) {
   key_ring[key.get_public_key()] = key;
}

private_key_type testing_fixture::get_private_key(const public_key_type& public_key) const {
   auto itr = key_ring.find(public_key);
   EOS_ASSERT(itr != key_ring.end(), missing_key_exception,
              "Private key corresponding to public key ${k} not known.", ("k", public_key));
   return itr->second;
}

flat_set<public_key_type> testing_fixture::available_keys() const {
   auto range = key_ring | boost::adaptors::map_keys;
   return {range.begin(), range.end()};
}

testing_blockchain::testing_blockchain(chainbase::database& db, fork_database& fork_db, block_log& blocklog,
                                   chain_initializer_interface& initializer, testing_fixture& fixture)
   : chain_controller(db, fork_db, blocklog, initializer, native_contract::make_administrator()),
     db(db),
     fixture(fixture) {}

void testing_blockchain::produce_blocks(uint32_t count, uint32_t blocks_to_miss) {
   if (count == 0)
      return;

   for (int i = 0; i < count; ++i) {
      auto slot = blocks_to_miss + 1;
      auto producer = get_producer(get_scheduled_producer(slot));
      auto private_key = fixture.get_private_key(producer.signing_key);
      generate_block(get_slot_time(slot), producer.owner, private_key, block_schedule::in_single_thread, 
                     skip_trx_sigs? chain_controller::skip_transaction_signatures : 0);
   }
}

void testing_blockchain::sync_with(testing_blockchain& other) {
   // Already in sync?
   if (head_block_id() == other.head_block_id())
      return;
   // If other has a longer chain than we do, sync it to us first
   if (head_block_num() < other.head_block_num())
      return other.sync_with(*this);

   auto sync_dbs = [](testing_blockchain& a, testing_blockchain& b) {
      for (int i = 1; i <= a.head_block_num(); ++i) {
         auto block = a.fetch_block_by_number(i);
         if (block && !b.is_known_block(block->id())) {
            b.push_block(*block);
         }
      }
   };

   sync_dbs(*this, other);
   sync_dbs(other, *this);
}

types::Asset testing_blockchain::get_liquid_balance(const types::AccountName& account) {
   return get_database().get<BalanceObject, native::eos::byOwnerName>(account).balance;
}

types::Asset testing_blockchain::get_staked_balance(const types::AccountName& account) {
   return get_database().get<StakedBalanceObject, native::eos::byOwnerName>(account).stakedBalance;
}

types::Asset testing_blockchain::get_unstaking_balance(const types::AccountName& account) {
   return get_database().get<StakedBalanceObject, native::eos::byOwnerName>(account).unstakingBalance;
}

std::set<types::AccountName> testing_blockchain::get_approved_producers(const types::AccountName& account) {
   const auto& sbo = get_database().get<StakedBalanceObject, byOwnerName>(account);
   if (sbo.producerVotes.contains<ProducerSlate>()) {
      auto range = sbo.producerVotes.get<ProducerSlate>().range();
      return {range.begin(), range.end()};
   }
   return {};
}

types::PublicKey testing_blockchain::get_block_signing_key(const types::AccountName& producerName) {
   return get_database().get<producer_object, by_owner>(producerName).signing_key;
}

void testing_blockchain::sign_transaction(SignedTransaction& trx) const {
   auto keys = get_required_keys(trx, fixture.available_keys());
   for (const auto& k : keys) {
      // TODO: Use a real chain_id here
      trx.sign(fixture.get_private_key(k), chain_id_type{});
   }
}

fc::optional<ProcessedTransaction> testing_blockchain::push_transaction(SignedTransaction trx, uint32_t skip_flags) {
   if (skip_trx_sigs)
      skip_flags |= chain_controller::skip_transaction_signatures;

   if (auto_sign_trxs) {
      sign_transaction(trx);
   }

   if (hold_for_review) {
      review_storage = std::make_pair(trx, skip_flags);
      return {};
   }
   return chain_controller::push_transaction(trx, skip_flags);
}

void testing_network::connect_blockchain(testing_blockchain& new_database) {
   if (blockchains.count(&new_database))
      return;

   // If the network isn't empty, sync the new database with one of the old ones. The old ones are already in sync with
   // each other, so just grab one arbitrarily. The old databases are connected to the propagation signals, so when one
   // of them gets synced, it will propagate blocks to the others as well.
   if (!blockchains.empty()) {
        blockchains.begin()->first->sync_with(new_database);
   }

   // The new database is now in sync with any old ones; go ahead and connect the propagation signal.
    blockchains[&new_database] = new_database.applied_block.connect([this, &new_database](const signed_block& block) {
      propagate_block(block, new_database);
   });
}

void testing_network::disconnect_database(testing_blockchain& leaving_database) {
    blockchains.erase(&leaving_database);
}

void testing_network::disconnect_all() {
    blockchains.clear();
}

void testing_network::propagate_block(const signed_block& block, const testing_blockchain& skip_db) {
   for (const auto& pair : blockchains) {
      if (pair.first == &skip_db) continue;
      boost::signals2::shared_connection_block blocker(pair.second);
      pair.first->push_block(block);
   }
}

} } // eos::chain
