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

#include <eos/utilities/tempdir.hpp>

#include <eos/native_system_contract_plugin/native_system_contract_plugin.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

uint32_t EOS_TESTING_GENESIS_TIMESTAMP = 1431700005;

namespace eos { namespace chain {

testing_fixture::testing_fixture() {
   default_genesis_state.initial_timestamp = fc::time_point_sec(EOS_TESTING_GENESIS_TIMESTAMP);
   default_genesis_state.immutable_parameters.min_producer_count = config::ProducerCount;
   for (int i = 0; i < default_genesis_state.immutable_parameters.min_producer_count; ++i) {
      auto name = std::string("init") + fc::to_string(i);
      auto private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name));
      public_key_type public_key = private_key.get_public_key();
      default_genesis_state.initial_accounts.emplace_back(name, 100000, public_key, public_key);
      key_ring[public_key] = private_key;

      private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name + ".producer"));
      public_key = private_key.get_public_key();
      default_genesis_state.initial_producers.emplace_back(name, public_key);
      key_ring[public_key] = private_key;
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

const genesis_state_type&testing_fixture::genesis_state() const {
   return default_genesis_state;
}

genesis_state_type&testing_fixture::genesis_state() {
   return default_genesis_state;
}

private_key_type testing_fixture::get_private_key(const public_key_type& public_key) const {
   auto itr = key_ring.find(public_key);
   EOS_ASSERT(itr != key_ring.end(), missing_key_exception,
              "Private key corresponding to public key ${k} not known.", ("k", public_key));
   return itr->second;
}

testing_database::testing_database(testing_fixture& fixture, std::string id,
                                   fc::optional<genesis_state_type> override_genesis_state)
   : genesis_state(override_genesis_state? *override_genesis_state : fixture.genesis_state()),
     fixture(fixture) {
   data_dir = fixture.get_temp_dir(id);
   // Install the system contract implementation
   native_system_contract_plugin::install(*this);
}

void testing_database::open() {
   database::open(data_dir, TEST_DB_SIZE, [this]{return genesis_state;});
}

void testing_database::replay() {
   database::replay(data_dir, TEST_DB_SIZE, genesis_state);
}

void testing_database::wipe(bool include_blocks) {
   database::wipe(data_dir, include_blocks);
}

void testing_database::produce_blocks(uint32_t count, uint32_t blocks_to_miss) {
   if (count == 0)
      return;

   BOOST_REQUIRE_MESSAGE(is_open(), "Producing blocks on closed db... Did you forget to open it?");

   for (int i = 0; i < count; ++i) {
      auto slot = blocks_to_miss + 1;
      auto producer_id = get_scheduled_producer(slot);
      const auto& producer = get(producer_id);
      auto private_key = fixture.get_private_key(producer.signing_key);
      generate_block(get_slot_time(slot), producer_id, private_key, 0);
   }
}

void testing_database::sync_with(testing_database& other) {
   // Already in sync?
   if (head_block_id() == other.head_block_id())
      return;
   // If other has a longer chain than we do, sync it to us first
   if (head_block_num() < other.head_block_num())
      return other.sync_with(*this);

   auto sync_dbs = [](testing_database& a, testing_database& b) {
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

void testing_network::connect_database(testing_database& new_database) {
   if (databases.count(&new_database))
      return;

   // If the network isn't empty, sync the new database with one of the old ones. The old ones are already in sync with
   // eachother, so just grab one arbitrarily. The old databases are connected to the propagation signals, so when one
   // of them gets synced, it will propagate blocks to the others as well.
   if (!databases.empty()) {
      databases.begin()->first->sync_with(new_database);
   }

   // The new database is now in sync with any old ones; go ahead and connect the propagation signal.
   databases[&new_database] = new_database.applied_block.connect([this, &new_database](const signed_block& block) {
      propagate_block(block, new_database);
   });
}

void testing_network::disconnect_database(testing_database& leaving_database) {
   databases.erase(&leaving_database);
}

void testing_network::disconnect_all() {
   databases.clear();
}

void testing_network::propagate_block(const signed_block& block, const testing_database& skip_db) {
   for (const auto& pair : databases) {
      if (pair.first == &skip_db) continue;
      boost::signals2::shared_connection_block blocker(pair.second);
      pair.first->push_block(block);
   }
}

} } // eos::chain
