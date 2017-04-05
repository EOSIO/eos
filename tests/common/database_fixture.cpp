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

#include <eos/chain/account_object.hpp>
#include <eos/chain/producer_object.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

using namespace eos::chain::test;

uint32_t EOS_TESTING_GENESIS_TIMESTAMP = 1431700000;

namespace eos { namespace chain {

using std::cout;
using std::cerr;

database_fixture::database_fixture()
   : app(), db( *app.chain_database() )
{
   try {
   int argc = boost::unit_test::framework::master_test_suite().argc;
   char** argv = boost::unit_test::framework::master_test_suite().argv;
   for( int i=1; i<argc; i++ )
   {
      const std::string arg = argv[i];
      if( arg == "--record-assert-trip" )
         fc::enable_record_assert_trip = true;
      if( arg == "--show-test-names" )
         std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
   }
   init_account_pub_key = init_account_priv_key.get_public_key();

   boost::program_options::variables_map options;

   genesis_state.initial_timestamp = fc::time_point_sec( EOS_TESTING_GENESIS_TIMESTAMP );

   genesis_state.initial_producer_count = 10;
   for( int i = 0; i < genesis_state.initial_producer_count; ++i )
   {
      auto name = "init"+fc::to_string(i);
      genesis_state.initial_accounts.emplace_back(name,
                                                  init_account_priv_key.get_public_key(),
                                                  init_account_priv_key.get_public_key());
      genesis_state.initial_producers.push_back({name, init_account_priv_key.get_public_key()});
   }
   open_database();

   generate_block();

   set_expiration( db, trx );
   } catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
      throw;
   }

   return;
}

database_fixture::~database_fixture()
{ try {
   // If we're unwinding due to an exception, don't do any more checks.
   // This way, boost test's last checkpoint tells us approximately where the error was.
   if( !std::uncaught_exception() )
   {
      BOOST_CHECK( db.get_node_properties().skip_flags == database::skip_nothing );
   }

   if( data_dir )
      db.close();
   return;
} FC_CAPTURE_AND_RETHROW() }

fc::ecc::private_key database_fixture::generate_private_key(string seed)
{
   static const fc::ecc::private_key committee = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")));
   if( seed == "null_key" )
      return committee;
   return fc::ecc::private_key::regenerate(fc::sha256::hash(seed));
}

string database_fixture::generate_anon_acct_name()
{
   // names of the form "anon-acct-x123" ; the "x" is necessary
   //    to workaround issue #46
   return "anon-acct-x" + std::to_string( anon_acct_count++ );
}

void database_fixture::open_database()
{
   if( !data_dir ) {
      data_dir = fc::temp_directory( eos::utilities::temp_directory_path() );
      db.open(data_dir->path(), TEST_DB_SIZE, [this]{return genesis_state;});
   }
}

signed_block database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
{
   skip |= database::skip_undo_history_check;
   // skip == ~0 will skip checks specified in database::validation_steps
   auto block = db.generate_block(db.get_slot_time(miss_blocks + 1),
                            db.get_scheduled_producer(miss_blocks + 1),
                            key, skip);
   db.clear_pending();
   return block;
}

void database_fixture::generate_blocks( uint32_t block_count )
{
   for( uint32_t i = 0; i < block_count; ++i )
      generate_block();
}

void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks, uint32_t skip)
{
   if( miss_intermediate_blocks )
   {
      generate_block(skip);
      auto slots_to_miss = db.get_slot_at_time(timestamp);
      if( slots_to_miss <= 1 )
         return;
      --slots_to_miss;
      generate_block(skip, init_account_priv_key, slots_to_miss);
      return;
   }
   while( db.head_block_time() < timestamp )
      generate_block(skip);
}

namespace test {

void set_expiration( const database& db, transaction& tx )
{
   const chain_parameters& params = db.get_global_properties().parameters;
   tx.set_reference_block(db.head_block_id());
   tx.set_expiration( db.head_block_time() + fc::seconds( params.block_interval * (params.maintenance_skip_slots + 1) * 3 ) );
   return;
}

bool _push_block( database& db, const signed_block& b, uint32_t skip_flags /* = 0 */ )
{
   return db.push_block( b, skip_flags);
}

signed_transaction _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags /* = 0 */ )
{ try {
   auto pt = db.push_transaction( tx, skip_flags );
   return pt;
} FC_CAPTURE_AND_RETHROW((tx)) }

}

testing_fixture::testing_fixture() {
   default_genesis_state.initial_timestamp = fc::time_point_sec(EOS_TESTING_GENESIS_TIMESTAMP);
   default_genesis_state.initial_producer_count = EOS_DEFAULT_MIN_PRODUCER_COUNT;
   for (int i = 0; i < default_genesis_state.initial_producer_count; ++i) {
      auto name = std::string("init") + fc::to_string(i);
      auto private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name));
      public_key_type public_key = private_key.get_public_key();
      default_genesis_state.initial_accounts.emplace_back(name, public_key, public_key);
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
   EOS_ASSERT(itr != key_ring.end(), testing_exception,
              "Private key corresponding to public key ${k} not known.", ("k", public_key));
   return itr->second;
}

testing_database::testing_database(testing_fixture& fixture, std::string id,
                                   fc::optional<genesis_state_type> override_genesis_state)
   : genesis_state(override_genesis_state? *override_genesis_state : fixture.genesis_state()),
     fixture(fixture) {
   data_dir = fixture.get_temp_dir(id);
}

void testing_database::open() {
   database::open(data_dir, TEST_DB_SIZE, [this]{return genesis_state;});
}

void testing_database::produce_blocks(uint32_t count, uint32_t blocks_to_miss) {
   if (count == 0)
      return;

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
   databases[&new_database] = new_database.applied_block.connect([this](const signed_block& block) {
      if (!currently_propagating_block)
         propagate_block(block);
   });
}

void testing_network::disconnect_database(testing_database& leaving_database) {
   databases.erase(&leaving_database);
}

void testing_network::disconnect_all() {
   databases.clear();
}

void testing_network::propagate_block(const signed_block& block) {
   currently_propagating_block = true;
   for (const auto& pair : databases)
      pair.first->push_block(block);
   currently_propagating_block = false;
}

// eos::chain::test

} } // eos::chain
