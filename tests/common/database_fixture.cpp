/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>
#include <boost/signals2/shared_connection_block.hpp>

#include <eos/chain/account_object.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/authority_checker.hpp>
#include <eos/producer_plugin/producer_plugin.hpp>

#include <eos/utilities/tempdir.hpp>

#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/native_contract/native_contract_chain_administrator.hpp>
#include <eos/native_contract/objects.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <boost/range/adaptor/map.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

uint32_t EOS_TESTING_GENESIS_TIMESTAMP = 1431700005;

namespace eosio { namespace chain {

testing_fixture::testing_fixture() {
   default_genesis_state.initial_timestamp = fc::time_point_sec(EOS_TESTING_GENESIS_TIMESTAMP);
   for (int i = 0; i < config::blocks_per_round; ++i) {
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
   : chain_controller(db, fork_db, blocklog, initializer, native_contract::make_administrator(),
                      ::eosio::chain_plugin::default_transaction_execution_time * 1000,
                      ::eosio::chain_plugin::default_received_block_transaction_execution_time * 1000,
                      ::eosio::chain_plugin::default_create_block_transaction_execution_time * 1000,
                       chain_controller::txn_msg_limits{}),
     db(db),
     fixture(fixture) {}

testing_blockchain::testing_blockchain(chainbase::database& db, fork_database& fork_db, block_log& blocklog,
                                       chain_initializer_interface& initializer, testing_fixture& fixture,
                                       uint32_t transaction_execution_time_msec,
                                       uint32_t received_block_execution_time_msec,
                                       uint32_t create_block_execution_time_msec,
                                       const chain_controller::txn_msg_limits& rate_limits)
   : chain_controller(db, fork_db, blocklog, initializer, native_contract::make_administrator(),
                      transaction_execution_time_msec * 1000,
                      received_block_execution_time_msec * 1000,
                      create_block_execution_time_msec * 1000,
                      rate_limits),
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
                     chain_controller::created_block | (skip_trx_sigs? chain_controller::skip_transaction_signatures : 0));
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
            b.push_block(*block, chain_controller::validation_steps::created_block);
         }
      }
   };

   sync_dbs(*this, other);
   sync_dbs(other, *this);
}

types::asset testing_blockchain::get_liquid_balance(const types::account_name& account) {
   return get_database().get<balance_object, eosio::chain::by_owner_name>(account).balance;
}

types::asset testing_blockchain::get_staked_balance(const types::account_name& account) {
   return get_database().get<staked_balance_object, eosio::chain::by_owner_name>(account).staked_balance;
}

types::asset testing_blockchain::get_unstaking_balance(const types::account_name& account) {
   return get_database().get<staked_balance_object, eosio::chain::by_owner_name>(account).unstaking_balance;
}

std::set<types::account_name> testing_blockchain::get_approved_producers(const types::account_name& account) {
   const auto& sbo = get_database().get<staked_balance_object, by_owner_name>(account);
   if (sbo.producer_votes.contains<producer_slate>()) {
      auto range = sbo.producer_votes.get<producer_slate>().range();
      return {range.begin(), range.end()};
   }
   return {};
}

types::public_key testing_blockchain::get_block_signing_key(const types::account_name& producerName) {
   return get_database().get<producer_object, by_owner>(producerName).signing_key;
}

void testing_blockchain::sign_transaction(signed_transaction& trx) const {
   auto keys = get_required_keys(trx, fixture.available_keys());
   for (const auto& k : keys) {
      // TODO: Use a real chain_id here
      trx.sign(fixture.get_private_key(k), chain_id_type{});
   }
}

fc::optional<processed_transaction> testing_blockchain::push_transaction(signed_transaction trx, uint32_t skip_flags) {
   if (skip_trx_sigs)
      skip_flags |= chain_controller::skip_transaction_signatures;

   if (auto_sign_trxs) {
      sign_transaction(trx);
   }

   if (hold_for_review) {
      review_storage = std::make_pair(trx, skip_flags);
      return {};
   }
   return chain_controller::push_transaction(trx, skip_flags | chain_controller::pushed_transaction);
}

vector<uint8_t> testing_blockchain::assemble_wast( const std::string& wast ) {
   //   std::cout << "\n" << wast << "\n";
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << "Error parsing WebAssembly text file:" << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_ASSERT( !"error parsing wast" );
   }

   try
   {
      // Serialize the WebAssembly module.
      Serialization::ArrayOutputStream stream;
      WASM::serialize(stream,module);
      return stream.getBytes();
   }
   catch(Serialization::FatalSerializationException exception)
   {
      std::cerr << "Error serializing WebAssembly binary file:" << std::endl;
      std::cerr << exception.message << std::endl;
      throw;
   }
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
      pair.first->push_block(block, chain_controller::created_block);
   }
}

} } // eosio::chain
