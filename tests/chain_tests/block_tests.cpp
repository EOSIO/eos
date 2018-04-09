#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester_network.hpp>
#include <eosio/chain/producer_object.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE( schedule_test ) { try {
  TESTER test;

  for( uint32_t i = 0; i < 200; ++i )
     test.produce_block();

  auto lib = test.control->last_irreversible_block_num();
  auto head = test.control->head_block_num();
  idump((lib)(head));

  test.close();
  test.open();

  auto rlib = test.control->last_irreversible_block_num();
  auto rhead = test.control->head_block_num();

  FC_ASSERT( rhead == lib );

  for( uint32_t i = 0; i < 1000; ++i )
     test.produce_block();
  ilog("exiting");
  test.produce_block();
  BOOST_REQUIRE_EQUAL( test.validate(), true );
} FC_LOG_AND_RETHROW() }/// schedule_test

BOOST_AUTO_TEST_CASE( push_block ) { try {
   TESTER test1;
   tester test2(false);

   test2.control->push_block(test1.produce_block());
   for (uint32 i = 0; i < 1000; ++i) {
      test2.push_block(test1.produce_block());
   }
   test1.create_account(N(alice));
   test2.push_block(test1.produce_block());

   test1.push_dummy(N(alice), "Foo!");
   test2.push_block(test1.produce_block());
   test1.produce_block();
   BOOST_REQUIRE_EQUAL( test1.validate(), true );
} FC_LOG_AND_RETHROW() }/// schedule_test

BOOST_AUTO_TEST_CASE( push_invalid_block ) { try {
   TESTER chain;

   // Create a new block
   signed_block new_block;
   auto head_time = chain.control->head_block_time();
   auto next_time = head_time + fc::microseconds(config::block_interval_us);
   uint32_t slot  = chain.control->get_slot_at_time( next_time );
   auto sch_pro   = chain.control->get_scheduled_producer(slot);
   auto priv_key  = chain.get_private_key( sch_pro, "active" );

   // On block action
   action on_block_act;
   on_block_act.account = config::system_account_name;
   on_block_act.name = N(onblock);
   on_block_act.authorization = vector<permission_level>{{config::system_account_name, config::active_name}};
   on_block_act.data = fc::raw::pack(chain.control->head_block_header());
   transaction trx;
   trx.actions.emplace_back(std::move(on_block_act));
   trx.set_reference_block(chain.control->head_block_id());
   trx.expiration = chain.control->head_block_time() + fc::seconds(1);

   // Add properties to block header
   new_block.previous = chain.control->head_block_id();
   new_block.timestamp = next_time;
   new_block.producer = sch_pro;
   new_block.block_mroot = chain.control->get_dynamic_global_properties().block_merkle_root.get_root();
   vector<transaction_metadata> input_metas;
   new_block.sign(priv_key);

   // Create a new empty region
   new_block.regions.resize(new_block.regions.size() + 1);
   // Pushing this block should fail, since every region inside a block should not be empty
   BOOST_REQUIRE_THROW(chain.control->push_block(new_block), tx_empty_region);

   // Create a new cycle inside the empty region
   new_block.regions.back().cycles_summary.resize(new_block.regions.back().cycles_summary.size() + 1);
   // Pushing this block should fail, since there should not be an empty cycle inside a block
   BOOST_REQUIRE_THROW(chain.control->push_block(new_block), tx_empty_cycle);

   // Create a new shard inside the empty cycle
   new_block.regions.back().cycles_summary.back().resize(new_block.regions.back().cycles_summary.back().size() + 1);
   // Pushing this block should fail, since there should not be an empty shard inside a block
   BOOST_REQUIRE_THROW(chain.control->push_block(new_block), tx_empty_shard);
} FC_LOG_AND_RETHROW() }/// push_invalid_block


// Utility function to check expected irreversible block
uint32_t calc_exp_last_irr_block_num(const base_tester& chain, const uint32_t& head_block_num) {
   const auto producers_size = chain.control->get_global_properties().active_producers.producers.size();
   const auto max_reversible_rounds = EOS_PERCENT(producers_size, config::percent_100 - config::irreversible_threshold_percent);
   if( max_reversible_rounds == 0) {
      return head_block_num - 1;
   } else {
      const auto current_round = head_block_num / config::producer_repetitions;
      const auto irreversible_round = current_round - max_reversible_rounds;
      return (uint32_t)((irreversible_round + 1) * config::producer_repetitions - 1);
   }
};

BOOST_AUTO_TEST_CASE(trx_variant ) {
   try {
      TESTER chain;

      // Create account transaction
      signed_transaction trx;
      name new_account_name = name("alice");
      authority owner_auth = authority( chain.get_public_key( new_account_name, "owner" ) );
      trx.actions.emplace_back( vector<permission_level>{{ config::system_account_name, config::active_name}},
                                contracts::newaccount{
                                        .creator  = config::system_account_name,
                                        .name     = new_account_name,
                                        .owner    = owner_auth,
                                        .active   = authority( chain.get_public_key( new_account_name, "active" ) ),
                                        .recovery = authority( chain.get_public_key( new_account_name, "recovery" ) ),
                                });
      trx.expiration = time_point_sec(chain.control->head_block_time()) + 100;
      trx.ref_block_num = (uint16_t)chain.control->head_block_num();
      trx.ref_block_prefix = (uint32_t)chain.control->head_block_id()._hash[1];

      auto original = fc::raw::pack( trx );
      // Convert to variant, and back to transaction object
      fc::variant var;
      contracts::abi_serializer::to_variant(trx, var, chain.get_resolver());
      signed_transaction from_var;
      contracts::abi_serializer::from_variant(var, from_var, chain.get_resolver());
      auto _process  = fc::raw::pack( from_var );

      FC_ASSERT( original == _process, "Transaction serialization not reversible" );
      BOOST_REQUIRE_EQUAL( chain.validate(), true );
   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }

}

BOOST_AUTO_TEST_CASE(trx_uniqueness) {
   TESTER chain;

   signed_transaction trx;
   name new_account_name = name("alice");
   authority owner_auth = authority(chain.get_public_key( new_account_name, "owner"));
   trx.actions.emplace_back(vector<permission_level>{{config::system_account_name, config::active_name}},
                            contracts::newaccount{
                               .creator  = config::system_account_name,
                               .name     = new_account_name,
                               .owner    = owner_auth,
                               .active   = authority(chain.get_public_key(new_account_name, "active")),
                               .recovery = authority(chain.get_public_key(new_account_name, "recovery)),"))
                            });
   chain.set_transaction_headers(trx, 90);
   trx.sign(chain.get_private_key(config::system_account_name, "active"), chain_id_type());
   chain.push_transaction(trx);

   BOOST_CHECK_THROW(chain.push_transaction(trx), tx_duplicate);
   BOOST_REQUIRE_EQUAL( chain.validate(), true );
}

BOOST_AUTO_TEST_CASE(invalid_expiration) {
   TESTER chain;

   signed_transaction trx;
   name new_account_name = name("alice");
   authority owner_auth = authority(chain.get_public_key( new_account_name, "owner"));
   trx.actions.emplace_back(vector<permission_level>{{config::system_account_name, config::active_name}},
                            contracts::newaccount{
                               .creator  = config::system_account_name,
                               .name     = new_account_name,
                               .owner    = owner_auth,
                               .active   = authority(chain.get_public_key(new_account_name, "active")),
                               .recovery = authority(chain.get_public_key(new_account_name, "recovery)),"))
                            });
   trx.ref_block_num = static_cast<uint16_t>(chain.control->head_block_num());
   trx.ref_block_prefix = static_cast<uint32_t>(chain.control->head_block_id()._hash[1]);
   trx.sign(chain.get_private_key(config::system_account_name, "active"), chain_id_type());
   // Unset expiration should throw
   BOOST_CHECK_THROW(chain.push_transaction(trx), transaction_exception);

   memset(&trx.expiration, 0, sizeof(trx.expiration)); // currently redundant, as default is all zeros, but may not always be.
   trx.sign(chain.get_private_key(config::system_account_name, "active"), chain_id_type());
   // Expired transaction (January 1970) should throw
   BOOST_CHECK_THROW(chain.push_transaction(trx), transaction_exception);
   BOOST_REQUIRE_EQUAL( chain.validate(), true );
}

BOOST_AUTO_TEST_CASE(transaction_expiration) {

   for (int i = 0; i < 2; ++i) {
      TESTER chain;
      signed_transaction trx;
      name new_account_name = name("alice");
      authority owner_auth = authority(chain.get_public_key( new_account_name, "owner"));
      trx.actions.emplace_back(vector<permission_level>{{config::system_account_name, config::active_name}},
                              contracts::newaccount{
                                 .creator  = config::system_account_name,
                                 .name     = new_account_name,
                                 .owner    = owner_auth,
                                 .active   = authority(chain.get_public_key(new_account_name, "active")),
                                 .recovery = authority(chain.get_public_key(new_account_name, "recovery)),"))
                              });
      trx.ref_block_num = static_cast<uint16_t>(chain.control->head_block_num());
      trx.ref_block_prefix = static_cast<uint32_t>(chain.control->head_block_id()._hash[1]);
      trx.expiration = chain.control->head_block_time() + fc::microseconds(i * 1000000);
      trx.sign(chain.get_private_key(config::system_account_name, "active"), chain_id_type());

      // expire in 1st time, pass in 2nd time
      if (i == 0)
         BOOST_CHECK_THROW(chain.push_transaction(trx), expired_tx_exception);
      else
         chain.push_transaction(trx);

      BOOST_REQUIRE_EQUAL( chain.validate(), true );
   }
}

BOOST_AUTO_TEST_CASE(invalid_tapos) {
   TESTER chain;
   signed_transaction trx;
   name new_account_name = name("alice");
   authority owner_auth = authority(chain.get_public_key( new_account_name, "owner"));
   trx.actions.emplace_back(vector<permission_level>{{config::system_account_name, config::active_name}},
                           contracts::newaccount{
                              .creator  = config::system_account_name,
                              .name     = new_account_name,
                              .owner    = owner_auth,
                              .active   = authority(chain.get_public_key(new_account_name, "active")),
                              .recovery = authority(chain.get_public_key(new_account_name, "recovery)),"))
                           });
   trx.ref_block_num = static_cast<uint16_t>(chain.control->head_block_num() + 1);
   trx.ref_block_prefix = static_cast<uint32_t>(chain.control->head_block_id()._hash[1]);
   trx.expiration = chain.control->head_block_time() + fc::microseconds(1000000);
   trx.sign(chain.get_private_key(config::system_account_name, "active"), chain_id_type());

   BOOST_CHECK_THROW(chain.push_transaction(trx), invalid_ref_block_exception);

   BOOST_REQUIRE_EQUAL(chain.validate(), true );
}

BOOST_AUTO_TEST_CASE(irrelevant_auth) {
   try {
      TESTER chain;
      chain.create_account(name("joe"));

      chain.produce_blocks();

      // Use create account transaction as example
      signed_transaction trx;
      name new_account_name = name("alice");
      authority owner_auth = authority( chain.get_public_key( new_account_name, "owner" ) );
      trx.actions.emplace_back( vector<permission_level>{{ config::system_account_name, config::active_name}},
                                contracts::newaccount{
                                        .creator  = config::system_account_name,
                                        .name     = new_account_name,
                                        .owner    = owner_auth,
                                        .active   = authority( chain.get_public_key( new_account_name, "active" ) ),
                                        .recovery = authority( chain.get_public_key( new_account_name, "recovery" ) ),
                                });
      chain.set_transaction_headers(trx);
      trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain_id_type()  );

      chain.push_transaction(trx, skip_transaction_signatures);
      chain.control->clear_pending();

      // Add unneeded signature
      trx.sign( chain.get_private_key( name("random"), "active" ), chain_id_type()  );

      // Check that it throws for irrelevant signatures
      BOOST_CHECK_THROW(chain.push_transaction( trx ), tx_irrelevant_sig);

      BOOST_REQUIRE_EQUAL( chain.validate(), true );
   } FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(name_test) {
   name temp;
   temp = "temp";
   BOOST_TEST( string("temp") == string(temp) );
   BOOST_TEST( string("temp.temp") == string(name("temp.temp")) );
   BOOST_TEST( string("") == string(name()) );
   BOOST_TEST_REQUIRE( string("hello") == string(name("hello")) );
   BOOST_TEST_REQUIRE( name(-1) == name(string(name(-1))) );
   BOOST_TEST_REQUIRE( string(name(-1)) == string(name(string(name(-1)))) );
}

// Simple test of block production and head_block_num tracking
BOOST_AUTO_TEST_CASE(produce_blocks)
{ try {
      TESTER chain;
      BOOST_TEST(chain.control->head_block_num() == 0);
      chain.produce_blocks();
      BOOST_TEST(chain.control->head_block_num() == 1);
      chain.produce_blocks(5);
      BOOST_TEST(chain.control->head_block_num() == 6);
      const auto& producers_size = chain.control->get_global_properties().active_producers.producers.size();
      chain.produce_blocks((uint32_t)producers_size);
      BOOST_TEST(chain.control->head_block_num() == producers_size + 6);
      BOOST_REQUIRE_EQUAL( chain.validate(), true );
   } FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(order_dependent_transactions)
{ try {
      TESTER chain;

      const auto& tester_account_name = name("tester");
      chain.create_account(name("tester"));
      chain.produce_blocks(10);

      // For this test case, we need two actions that dependent on each other
      // We use updateauth actions, where the second action depend on the auth created in first action
      chain.push_action(name("eosio"), name("updateauth"), name("tester"), fc::mutable_variant_object()
              ("account", "tester")
              ("permission", "first")
              ("parent", "active")
              ("data",  authority(chain.get_public_key(name("tester"), "first")))
              ("delay", 0));
      chain.push_action(name("eosio"), name("updateauth"), name("tester"), fc::mutable_variant_object()
              ("account", "tester")
              ("permission", "second")
              ("parent", "first")
              ("data",  authority(chain.get_public_key(name("tester"), "second")))
              ("delay", 0));

      // Ensure the related auths are created
      const auto* first_auth = chain.find<permission_object, by_owner>(boost::make_tuple(name("tester"), name("first")));
      BOOST_TEST(first_auth != nullptr);
      BOOST_TEST(first_auth->owner == name("tester"));
      BOOST_TEST(first_auth->name == name("first"));
      BOOST_TEST_REQUIRE(!first_auth->auth.keys.empty());
      BOOST_TEST(first_auth->auth.keys.front().key == chain.get_public_key(name("tester"), "first"));
      const auto* second_auth = chain.find<permission_object, by_owner>(boost::make_tuple(name("tester"), name("second")));
      BOOST_TEST(second_auth != nullptr);
      BOOST_TEST(second_auth->owner == name("tester"));
      BOOST_TEST(second_auth->name == name("second"));
      BOOST_TEST_REQUIRE(!second_auth->auth.keys.empty());
      BOOST_TEST(second_auth->auth.keys.front().key == chain.get_public_key(name("tester"), "second"));

      chain.produce_blocks();
      // Ensure correctness of the cycle
      BOOST_TEST(chain.control->head_block_num() == 11);
      BOOST_TEST(chain.control->fetch_block_by_number(11).valid());
      BOOST_TEST_REQUIRE(!chain.control->fetch_block_by_number(11)->regions.empty());
      BOOST_TEST_REQUIRE(!chain.control->fetch_block_by_number(11)->regions.front().cycles_summary.empty());
      BOOST_TEST_REQUIRE(chain.control->fetch_block_by_number(11)->regions.front().cycles_summary.size() >= 1);
      // First cycle has only on-block transaction
      BOOST_TEST(!chain.control->fetch_block_by_number(11)->regions.front().cycles_summary.front().empty());
      BOOST_TEST(chain.control->fetch_block_by_number(11)->regions.front().cycles_summary.front().front().transactions.size() == 1);
      BOOST_TEST(chain.control->fetch_block_by_number(11)->regions.front().cycles_summary.at(1).front().transactions.size() == 2);
      BOOST_REQUIRE_EQUAL( chain.validate(), true );
   } FC_LOG_AND_RETHROW() }

// Simple test of block production when a block is missed
BOOST_AUTO_TEST_CASE(missed_blocks)
{ try {
      TESTER chain;
      // Set inita-initu to be producers
      vector<account_name> producer_names;
      for (char i = 'a'; i <= 'u'; i++) {
         producer_names.emplace_back(std::string("init") + i);
      }
      chain.create_accounts(producer_names);
      chain.set_producers(producer_names);

      // Produce blocks until the next block production will use the new set of producers from beginning
      // First, end the current producer round
      chain.produce_blocks_until_end_of_round();
      // Second, end the next producer round (since the next round will start new set of producers from the middle)
      chain.produce_blocks_until_end_of_round();


      const auto& ref_block_num = chain.control->head_block_num();

      account_name skipped_producers[3] = {chain.control->get_scheduled_producer(config::producer_repetitions),
                                           chain.control->get_scheduled_producer(2 * config::producer_repetitions),
                                           chain.control->get_scheduled_producer(3 * config::producer_repetitions)};
      auto next_block_time = static_cast<fc::time_point>(chain.control->get_slot_time(4 * config::producer_repetitions));
      auto next_producer = chain.control->get_scheduled_producer(4 * config::producer_repetitions);

      BOOST_TEST(chain.control->head_block_num() == ref_block_num);
      const auto& blocks_to_miss = (config::producer_repetitions - 1) + 3 * config::producer_repetitions;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us), 0);

      BOOST_TEST(chain.control->head_block_num() == ref_block_num + 1);
      BOOST_TEST(static_cast<fc::string>(chain.control->head_block_time()) == static_cast<fc::string>(next_block_time));
      BOOST_TEST(chain.control->head_block_producer() ==  next_producer);
      BOOST_TEST(chain.control->get_producer(next_producer).total_missed == 0);

      for (auto producer : skipped_producers) {
         BOOST_TEST(chain.control->get_producer(producer).total_missed == config::producer_repetitions);
      }

      BOOST_REQUIRE_EQUAL( chain.validate(), true );
   } FC_LOG_AND_RETHROW() }

// Simple sanity test of test network: if databases aren't connected to the network, they don't sync to each other
BOOST_AUTO_TEST_CASE(no_network)
{ try {
      TESTER chain1, chain2;

      BOOST_TEST(chain1.control->head_block_num() == 0);
      BOOST_TEST(chain2.control->head_block_num() == 0);
      chain1.produce_blocks();
      BOOST_TEST(chain1.control->head_block_num() == 1);
      BOOST_TEST(chain2.control->head_block_num() == 0);
      chain2.produce_blocks(5);
      BOOST_TEST(chain1.control->head_block_num() == 1);
      BOOST_TEST(chain2.control->head_block_num() == 5);

      chain1.produce_block();
      chain2.produce_block();

      BOOST_REQUIRE_EQUAL( chain1.validate(), true );
      BOOST_REQUIRE_EQUAL( chain2.validate(), true );
} FC_LOG_AND_RETHROW() }

// Test that two databases on the same network do sync to each other
BOOST_AUTO_TEST_CASE(simple_network)
{ try {
      tester chain1, chain2;

      tester_network net;
      net.connect_blockchain(chain1);
      net.connect_blockchain(chain2);

      BOOST_TEST(chain1.control->head_block_num() == 0);
      BOOST_TEST(chain2.control->head_block_num() == 0);
      chain1.produce_blocks();
      BOOST_TEST(chain1.control->head_block_num() == 1);
      BOOST_TEST(chain2.control->head_block_num() == 1);
      BOOST_TEST(chain1.control->head_block_id().str() == chain2.control->head_block_id().str());
      chain2.produce_blocks(5);
      BOOST_TEST(chain1.control->head_block_num() == 6);
      BOOST_TEST(chain2.control->head_block_num() == 6);
      BOOST_TEST(chain1.control->head_block_id().str() == chain2.control->head_block_id().str());

      chain1.produce_block();
      chain2.produce_block();
      BOOST_REQUIRE_EQUAL( chain1.validate(), true );
      BOOST_REQUIRE_EQUAL( chain2.validate(), true );
} FC_LOG_AND_RETHROW() }


//// Test that two databases joining and leaving a network sync correctly after a fork
BOOST_AUTO_TEST_CASE(forked_network)
{ try {
      tester chain1, chain2;

      tester_network net;

      BOOST_TEST(chain1.control->head_block_num() == 0);
      BOOST_TEST(chain2.control->head_block_num() == 0);
      chain1.produce_blocks();
      BOOST_TEST(chain1.control->head_block_num() == 1);
      BOOST_TEST(chain2.control->head_block_num() == 0);
      BOOST_TEST(chain1.control->head_block_id().str() != chain2.control->head_block_id().str());

      net.connect_blockchain(chain1);
      net.connect_blockchain(chain2);
      BOOST_TEST(chain2.control->head_block_num() == 1);
      BOOST_TEST(chain1.control->head_block_id().str() == chain2.control->head_block_id().str());

      chain2.produce_blocks(5);
      BOOST_TEST(chain1.control->head_block_num() == 6);
      BOOST_TEST(chain2.control->head_block_num() == 6);
      BOOST_TEST(chain1.control->head_block_id().str() == chain2.control->head_block_id().str());

      net.disconnect_blockchain(chain1);
      auto blocks_to_miss = 1;
      chain1.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      chain2.produce_blocks();
      BOOST_TEST(chain1.control->head_block_num() == 7);
      BOOST_TEST(chain2.control->head_block_num() == 7);
      BOOST_TEST(chain1.control->head_block_id().str() != chain2.control->head_block_id().str());

      blocks_to_miss = 1;
      chain2.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      net.connect_blockchain(chain1);
      BOOST_TEST(chain1.control->head_block_num() == 8);
      BOOST_TEST(chain2.control->head_block_num() == 8);
      BOOST_TEST(chain1.control->head_block_id().str() == chain2.control->head_block_id().str());
   } FC_LOG_AND_RETHROW() }


#if 0
// Check that the recent_slots_filled bitmap is being updated correctly
BOOST_AUTO_TEST_CASE( rsf_missed_blocks )
{ try {
      tester chain;
      chain.produce_blocks();

      auto rsf = [&]() -> string
      {
         auto rsf = chain.control->get_dynamic_global_properties().recent_slots_filled;
         string result = "";
         result.reserve(64);
         for( int i=0; i<64; i++ )
         {
            result += ((rsf & 1) == 0) ? '0' : '1';
            rsf >>= 1;
         }
         return result;
      };

      auto pct = []( uint32_t x ) -> uint32_t
      {
         return uint64_t( config::percent_100 ) * x / 64;
      };

      BOOST_TEST( rsf() ==
                  "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == config::percent_100 );

      auto blocks_to_miss = 1;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(127-64) );

      blocks_to_miss = 1;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0101111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(126-64) );

      blocks_to_miss = 2;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0010101111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(124-64) );

      blocks_to_miss = 3;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0001001010111111111111111111111111111111111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(121-64) );

      blocks_to_miss = 5;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0000010001001010111111111111111111111111111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(116-64) );

      blocks_to_miss = 8;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0000000010000010001001010111111111111111111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(108-64) );

      blocks_to_miss = 13;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0000000000000100000000100000100010010101111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(95-64) );

      chain.produce_blocks();
      BOOST_TEST( rsf() ==
                  "1000000000000010000000010000010001001010111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(95-64) );

      chain.produce_blocks();
      BOOST_TEST( rsf() ==
                  "1100000000000001000000001000001000100101011111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(95-64) );

      chain.produce_blocks();
      BOOST_TEST( rsf() ==
                  "1110000000000000100000000100000100010010101111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(95-64) );

      chain.produce_blocks();
      BOOST_TEST( rsf() ==
                  "1111000000000000010000000010000010001001010111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(95-64) );

      chain.produce_blocks(64);
      BOOST_TEST( rsf() ==
                  "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == config::percent_100 );

      blocks_to_miss = 63;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0000000000000000000000000000000000000000000000000000000000000001"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(1) );

      blocks_to_miss = 32;
      chain.produce_block(fc::microseconds((blocks_to_miss + 1) * config::block_interval_us));
      BOOST_TEST( rsf() ==
                  "0000000000000000000000000000000010000000000000000000000000000000"
      );
      BOOST_TEST( chain.control->producer_participation_rate() == pct(1) );
   } FC_LOG_AND_RETHROW() }
#endif

// Check that a db rewinds to the LIB after being closed and reopened
BOOST_AUTO_TEST_CASE(restart_db)
{ try {
      TESTER chain;


      {
         chain.produce_blocks(20);

         BOOST_TEST(chain.control->head_block_num() == 20);
         BOOST_TEST(chain.control->last_irreversible_block_num() == calc_exp_last_irr_block_num(chain, 20));
         chain.close();
      }

      {
         chain.open();
         // After restarting, we should have rewound to the last irreversible block.
         BOOST_TEST(chain.control->head_block_num() == calc_exp_last_irr_block_num(chain, 20));
         chain.produce_blocks(60);
         BOOST_TEST(chain.control->head_block_num() == calc_exp_last_irr_block_num(chain, 80));
      }
   } FC_LOG_AND_RETHROW() }

// Check that a db which is closed and reopened successfully syncs back with the network, including retrieving blocks
// that it missed while it was down
BOOST_AUTO_TEST_CASE(sleepy_db)
{ try {
      TESTER producer;

      tester_network net;
      net.connect_blockchain(producer);

      producer.produce_blocks(20);

      // The new node, sleepy, joins, syncs, disconnects
      tester sleepy;
      net.connect_blockchain(sleepy);
      BOOST_TEST(producer.control->head_block_num() == 20);
      BOOST_TEST(sleepy.control->head_block_num() == 20);
      net.disconnect_blockchain(sleepy);
      // Close sleepy
      sleepy.close();

      // 5 new blocks are produced
      producer.produce_blocks(5);
      BOOST_TEST(producer.control->head_block_num() == 25);

      // Sleepy is reborn! Check that it is now rewound to the LIB...
      sleepy.open();
      BOOST_TEST(sleepy.control->head_block_num() == calc_exp_last_irr_block_num(sleepy, 20));

      // Reconnect sleepy to the network and check that it syncs up to the present
      net.connect_blockchain(sleepy);
      BOOST_TEST(sleepy.control->head_block_num() == 25);
      BOOST_TEST(sleepy.control->head_block_id().str() == producer.control->head_block_id().str());
   } FC_LOG_AND_RETHROW() }

// Test reindexing the blockchain
BOOST_FIXTURE_TEST_CASE(reindex, validating_tester)
{ try {
      // Create shared configuration, so the new chain can be recreated from existing block log
      chain_controller::controller_config cfg;
      fc::temp_directory tempdir;
      cfg.block_log_dir      = tempdir.path() / "blocklog";
      cfg.shared_memory_dir  = tempdir.path() / "shared";
      cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
      cfg.genesis.initial_key = get_public_key( config::system_account_name, "active" );

      {
         // Create chain with shared configuration
         tester chain(cfg);
         chain.produce_blocks(100);
         BOOST_TEST(chain.control->last_irreversible_block_num() == calc_exp_last_irr_block_num(chain, 100));
      }

      {
         // Create a new chain with shared configuration,
         // Since it is sharing the same blocklog folder, it should has blocks up to the previous last irreversible block
         tester chain(cfg);
         BOOST_TEST(chain.control->head_block_num() == calc_exp_last_irr_block_num(chain, 100));
         chain.produce_blocks(20);
         BOOST_TEST(chain.control->head_block_num() == calc_exp_last_irr_block_num(chain, 120));
      }
   } FC_LOG_AND_RETHROW() }

// Test wiping a database and resyncing with an ongoing network
BOOST_AUTO_TEST_CASE(wipe)
{ try {
      tester chain1, chain2;

      tester_network net;
      net.connect_blockchain(chain1);
      net.connect_blockchain(chain2);

      {
         // Create db3 with a temporary data dir
         tester chain3;
         net.connect_blockchain(chain3);

         chain1.produce_blocks(3);
         chain2.produce_blocks(3);
         BOOST_TEST(chain1.control->head_block_num() == 6);
         BOOST_TEST(chain2.control->head_block_num() == 6);
         BOOST_TEST(chain3.control->head_block_num() == 6);
         BOOST_TEST(chain1.control->head_block_id().str() == chain2.control->head_block_id().str());
         BOOST_TEST(chain1.control->head_block_id().str() == chain3.control->head_block_id().str());

         net.disconnect_blockchain(chain3);
      }

      {
         // Create new chain3 with a new temporary data dir
         tester chain3;
         BOOST_TEST(chain3.control->head_block_num() == 0);

         net.connect_blockchain(chain3);
         BOOST_TEST(chain3.control->head_block_num(), 6);

         chain1.produce_blocks(3);
         chain2.produce_blocks(3);
         BOOST_TEST(chain1.control->head_block_num() == 12);
         BOOST_TEST(chain2.control->head_block_num() == 12);
         BOOST_TEST(chain3.control->head_block_num() == 12);
         BOOST_TEST(chain1.control->head_block_id().str() == chain2.control->head_block_id().str());
         BOOST_TEST(chain1.control->head_block_id().str() == chain3.control->head_block_id().str());
      }

   } FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(irrelevant_sig_soft_check) {
   try {
      tester chain;

      // Make an account, but add an extra signature to the transaction
      signed_transaction trx;
      chain.set_transaction_headers(trx);

      name new_account_name = name("alice");
      authority owner_auth = authority( chain.get_public_key( new_account_name, "owner" ) );

      trx.actions.emplace_back( vector<permission_level>{{ config::system_account_name, config::active_name}},
                                contracts::newaccount{
                                        .creator  = config::system_account_name,
                                        .name     = new_account_name,
                                        .owner    = owner_auth,
                                        .active   = authority( chain.get_public_key( new_account_name, "active" ) ),
                                        .recovery = authority( chain.get_public_key( new_account_name, "recovery" ) ),
                                });
      chain.set_transaction_headers(trx);
      trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain_id_type()  );
      trx.sign( chain.get_private_key( name("random"), "active" ), chain_id_type()  );

      // Check that it throws for irrelevant signatures
      BOOST_REQUIRE_THROW(chain.push_transaction( trx ), tx_irrelevant_sig);

      // Check that it throws for multiple signatures by the same key
      trx.signatures.clear();
      trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain_id_type() );
      trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain_id_type() );
      BOOST_REQUIRE_THROW(chain.push_transaction( trx ), tx_irrelevant_sig);

      // Sign the transaction properly and push to the block
      trx.signatures.clear();
      trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain_id_type() );
      chain.push_transaction( trx );

      // Produce block so the transaction gets included in the block
      chain.produce_blocks();

      // Now check that a second blockchain accepts the block
      tester newchain;
      tester_network net;
      net.connect_blockchain(chain);
      net.connect_blockchain(newchain);
      BOOST_TEST((newchain.find<account_object, by_name>("alice")) != nullptr);
   } FC_LOG_AND_RETHROW()
}

//
BOOST_AUTO_TEST_CASE(irrelevant_sig_hard_check) {
   try {
      {
         tester chain;

         // Make an account, but add an extra signature to the transaction
         signed_transaction trx;

         name new_account_name = name("alice");
         authority owner_auth = authority( chain.get_public_key( new_account_name, "owner" ) );

         trx.actions.emplace_back( vector<permission_level>{{ config::system_account_name, config::active_name}},
                                   contracts::newaccount{
                                           .creator  = config::system_account_name,
                                           .name     = new_account_name,
                                           .owner    = owner_auth,
                                           .active   = authority( chain.get_public_key( new_account_name, "active" ) ),
                                           .recovery = authority( chain.get_public_key( new_account_name, "recovery" ) ),
                                   });
         chain.set_transaction_headers(trx);
         trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain_id_type()  );
         trx.sign( chain.get_private_key( name("random"), "active" ), chain_id_type()  );

         // Force push transaction with irrelevant signatures using a skip flag
         chain.push_transaction( trx, skip_transaction_signatures );
         // Produce block so the transaction gets included in the block
         chain.produce_blocks();

         // Now check that a second blockchain rejects the block with the oversigned transaction
         tester newchain;
         tester_network net;
         net.connect_blockchain(chain);
         BOOST_CHECK_THROW(net.connect_blockchain(newchain), tx_irrelevant_sig);
      }

      {
         tester chain;

         // Make an account, but add an extra signature to the transaction
         signed_transaction trx;

         name new_account_name = name("alice");
         authority owner_auth = authority( chain.get_public_key( new_account_name, "owner" ) );

         trx.actions.emplace_back( vector<permission_level>{{ config::system_account_name, config::active_name}},
                                   contracts::newaccount{
                                           .creator  = config::system_account_name,
                                           .name     = new_account_name,
                                           .owner    = owner_auth,
                                           .active   = authority( chain.get_public_key( new_account_name, "active" ) ),
                                           .recovery = authority( chain.get_public_key( new_account_name, "recovery" ) ),
                                   });
         chain.set_transaction_headers(trx);
         trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain_id_type() );
         trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain_id_type() );

         // Force push transaction with multiple signatures by the same key using a skip flag
         chain.push_transaction( trx, skip_transaction_signatures );
         // Produce block so the transaction gets included in the block
         chain.produce_blocks();

         // Now check that a second blockchain rejects the block with the oversigned transaction
         tester newchain;
         tester_network net;
         net.connect_blockchain(chain);
         BOOST_CHECK_THROW(net.connect_blockchain(newchain), tx_irrelevant_sig);
      }
   } FC_LOG_AND_RETHROW()
}

// Test reindexing the blockchain
BOOST_AUTO_TEST_CASE(block_id_sig_independent)
{ try {
      validating_tester chain;
      // Create a new block
      signed_block new_block;
      auto next_time = chain.control->head_block_time() + fc::microseconds(config::block_interval_us);
      uint32_t slot  = chain.control->get_slot_at_time( next_time );
      auto sch_pro   = chain.control->get_scheduled_producer(slot);

      // On block action
      action on_block_act;
      on_block_act.account = config::system_account_name;
      on_block_act.name = N(onblock);
      on_block_act.authorization = vector<permission_level>{{config::system_account_name, config::active_name}};
      on_block_act.data = fc::raw::pack(chain.control->head_block_header());
      transaction trx;
      trx.actions.emplace_back(std::move(on_block_act));
      trx.set_reference_block(chain.control->head_block_id());
      trx.expiration = chain.control->head_block_time() + fc::seconds(1);
      trx.max_kcpu_usage = 2000; // 1 << 24;

      // Add properties to block header
      new_block.previous = chain.control->head_block_id();
      new_block.timestamp = next_time;
      new_block.producer = sch_pro;
      new_block.block_mroot = chain.control->get_dynamic_global_properties().block_merkle_root.get_root();
      vector<transaction_metadata> input_metas;

      // Sign the block with active signature
      new_block.sign(chain.get_private_key( sch_pro, "active" ));
      auto block_id_act_sig = new_block.id();

      // Sign the block with other signature
      new_block.sign(chain.get_private_key( sch_pro, "other" ));
      auto block_id_othr_sig = new_block.id();

      // The block id should be independent of the signature
      BOOST_TEST(block_id_act_sig == block_id_othr_sig);
   } FC_LOG_AND_RETHROW() }


// Test transaction signature chain_controller::get_required_keys
BOOST_AUTO_TEST_CASE(get_required_keys)
{ try {
      validating_tester chain;

      account_name a = N(kevin);
      account_name creator = config::system_account_name;
      signed_transaction trx;

      authority owner_auth = authority( chain.get_public_key( a, "owner" ) );

      // any transaction will do
      trx.actions.emplace_back( std::vector<permission_level>{{creator,config::active_name}},
                                contracts::newaccount{
                                      .creator  = creator,
                                      .name     = a,
                                      .owner    = owner_auth,
                                      .active   = authority( chain.get_public_key( a, "active" ) ),
                                      .recovery = authority( chain.get_public_key( a, "recovery" ) ),
                                });

      chain.set_transaction_headers(trx);
      BOOST_REQUIRE_THROW(chain.push_transaction(trx), tx_missing_sigs);

      const auto priv_key_not_needed_1 = chain.get_private_key("alice", "blah");
      const auto priv_key_not_needed_2 = chain.get_private_key("alice", "owner");
      const auto priv_key_needed = chain.get_private_key(creator, "active");

      flat_set<public_key_type> available_keys = { priv_key_not_needed_1.get_public_key(),
                                                   priv_key_not_needed_2.get_public_key(),
                                                   priv_key_needed.get_public_key() };
      auto required_keys = chain.validating_node->get_required_keys(trx, available_keys);
      BOOST_TEST( required_keys.size() == 1 );
      BOOST_TEST( *required_keys.begin() == priv_key_needed.get_public_key() );
      trx.sign( priv_key_needed, chain_id_type() );
      chain.push_transaction(trx);

      chain.produce_blocks();

   } FC_LOG_AND_RETHROW() }


// Test transaction_mroot matches with the specification in Github #1972 https://github.com/EOSIO/eos/issues/1972
// Which is a root of a Merkle tree over commitments for each region processed in the block ordered in ascending region id order.
// Commitment for each region is a merkle tree over commitments for each shard inside the cycle of that region.
// Commitment for the shard itself is a merkle tree over the transactions commitments inside that shard.
// The transaction commitment is digest of the concentanation of region_id, cycle_index, shard_index, tx_index,
// transaction_receipt and packed_trx_digest (if the tx is an input tx, which doesn't include implicit/ deferred tx)
BOOST_AUTO_TEST_CASE(transaction_mroot)
{ try {
   validating_tester chain;
   // Finalize current block (which has set contract transaction for eosio)
   chain.produce_block();

   // any transaction will do
   vector<transaction_trace> traces = chain.create_accounts({"test1", "test2", "test3"});

   // Calculate expected tx roots
   vector<digest_type> tx_roots;
   for( uint64_t tx_index = 0; tx_index < traces.size(); tx_index++ ) {
      const auto& tx = traces[tx_index];
      digest_type::encoder enc;
      // region_id, cycle_index, shard_index, tx_index, transaction_receipt and packed_trx_digest (if the tx is an input tx)
      fc::raw::pack( enc, tx.region_id );
      fc::raw::pack( enc, tx.cycle_index );
      fc::raw::pack( enc, tx.shard_index );
      fc::raw::pack( enc, tx_index );
      fc::raw::pack( enc, *static_cast<const transaction_receipt*>(&tx) );
      if( tx.packed_trx_digest.valid() ) fc::raw::pack( enc, *tx.packed_trx_digest );
      tx_roots.emplace_back(enc.result());
   }
   auto expected_shard_tx_root = merkle(tx_roots);

   // Hardcoded on_block tx_root, since there's no easy way to calculate the tx_root with current interface
   auto on_block_tx_root = digest_type("aa63d366cc2ef41746bb150258d1c0662c8133469f785425cb996dbe2a227086");
   // There is only 1 region, 2 cycle, 1 shard in first cycle, 1 shard in second cycle
   auto expected_tx_mroot = merkle({on_block_tx_root, expected_shard_tx_root});

   // Compare with head block tx mroot
   chain.produce_block();
   auto head_block_tx_mroot = chain.control->head_block_header().transaction_mroot;
   BOOST_TEST(expected_tx_mroot.str() == head_block_tx_mroot.str());

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
