/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/block_summary_object.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

using namespace eosio;
using namespace chain;




BOOST_AUTO_TEST_SUITE(block_tests)


/**
 *  The purpose of this test is to demonstrate that it is possible
 *  to schedule 3M transactions into cycles where each cycle contains
 *  a set of independent transactions.  
 *
 *  This simple single threaded algorithm sorts 3M transactions each of which
 *  requires scope of 2 accounts chosen with a normal probability distribution
 *  among a set of 2 million accounts.
 *
 *  This algorithm executes in less than 0.5 seconds on a 4Ghz Core i7 and could
 *  potentially take just .05 seconds with 10+ CPU cores. Future improvements might
 *  use a more robust bloomfilter to identify collisions.
 */
///@{
struct Location {
   uint32_t thread = -1;
   uint32_t cycle  = -1;
   Location& operator=( const Location& l ) {
      thread = l.thread;
      cycle = l.cycle;
      return *this;
   }
};
struct ExTransaction : public transaction {
   Location location;
};

BOOST_AUTO_TEST_CASE(schedule_test) {
   vector<ExTransaction*> transactions(3*1000*1000);
   auto rand_scope = []() {
      return rand()%1000000 + rand()%1000000;
   };

   for( auto& t : transactions ) {
      t = new ExTransaction();
      t->scope = sort_names({ rand_scope(), rand_scope() });
   }

   int cycle = 0;
   std::vector<int> thread_count(1024);

   vector<ExTransaction*> postponed;
   postponed.reserve(transactions.size());
   auto current = transactions;

   vector<bool>  used(1024*1024);
   auto start = fc::time_point::now();
   bool scheduled = true;
   while( scheduled ) {
      scheduled = false;
      for( auto t : current ) {
         bool u = false;
         for( const auto& s : t->scope ) {
            if( used[s.value%used.size()] ) {
               u = true;
               postponed.push_back(t);
               break;
            }
         }
         if( !u ) {
            for( const auto& s : t->scope ) {
               used[s.value%used.size()] = true;
            }
            t->location.cycle  = cycle;
            t->location.thread = thread_count[cycle]++;
            scheduled = true;
         }
      }
      current.resize(0);
      used.resize(0); used.resize(1024*1024);
      std::swap( current, postponed );
      ++cycle;
      
   } 
   auto end = fc::time_point::now();
   thread_count.resize(cycle+1);
//   idump((cycle));
//   idump((thread_count));

   auto sort_time = end-start;
   edump((sort_time.count()/1000000.0));
}
///@} end of schedule test 


BOOST_FIXTURE_TEST_CASE(trx_variant, testing_fixture) {
  
   try {
   Make_Blockchain(chain)
   name from("from"), to("to");
   uint64_t amount = 10;

   eosio::chain::processed_transaction trx;
   trx.scope = sort_names({from,to});
   transaction_emplace_message(trx, "eos", 
                      vector<types::account_permission>{ {from,"active"} },
                      "transfer", types::transfer{from, to, amount, ""});
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());

   auto original = fc::raw::pack( trx );
   auto var      = chain.transaction_to_variant( trx );
   auto from_var = chain.transaction_from_variant( var );
   auto _process  = fc::raw::pack( from_var );

   /*
   idump((trx));
   idump((var));
   idump((from_var));
   idump((original));
   idump((_process));
   */
   FC_ASSERT( original == _process, "Transaction serialization not reversible" );
   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_FIXTURE_TEST_CASE(irrelevant_auth, testing_fixture) {
   try {
   Make_Blockchain(chain)
   Make_Account(chain, joe);
   chain.produce_blocks();

   processed_transaction trx;
   trx.scope = sort_names({"joe", "inita"});
   transaction_emplace_message(trx, config::eos_contract_name, vector<types::account_permission>{{"inita", "active"}},
                      "transfer", types::transfer{"inita", "joe", 50,""});
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());
   chain.push_transaction(trx, chain_controller::skip_transaction_signatures);

   chain.clear_pending();
   chain.push_transaction(trx, chain_controller::skip_transaction_signatures);
   chain.clear_pending();

   trx.messages.front().authorization.emplace_back(types::account_permission{"initb", "active"});
   BOOST_CHECK_THROW(chain.push_transaction(trx, chain_controller::skip_transaction_signatures), tx_irrelevant_auth);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(name_test) {
   using eosio::types::name;
   name temp;
   temp = "temp";
   BOOST_CHECK_EQUAL( string("temp"), string(temp) );
   BOOST_CHECK_EQUAL( string("temp.temp"), string(name("temp.temp")) );
   BOOST_CHECK_EQUAL( string(""), string(name()) );
   BOOST_REQUIRE_EQUAL( string("hello"), string(name("hello")) );
   BOOST_REQUIRE_EQUAL( name(-1), name(string(name(-1))) );
   BOOST_REQUIRE_EQUAL( string(name(-1)), string(name(string(name(-1)))) );
}

// Simple test of block production and head_block_num tracking
BOOST_FIXTURE_TEST_CASE(produce_blocks, testing_fixture)
{ try {
      Make_Blockchain(chain)

      BOOST_CHECK_EQUAL(chain.head_block_num(), 0);
      chain.produce_blocks();
      BOOST_CHECK_EQUAL(chain.head_block_num(), 1);
      chain.produce_blocks(5);
      BOOST_CHECK_EQUAL(chain.head_block_num(), 6);
      chain.produce_blocks(chain.get_global_properties().active_producers.size());
      BOOST_CHECK_EQUAL(chain.head_block_num(), chain.get_global_properties().active_producers.size() + 6);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(order_dependent_transactions, testing_fixture)
{ try {
      Make_Blockchain(chain);
      Make_Account(chain, newguy);
      chain.produce_blocks(10);

      Transfer_Asset(chain, inita, newguy, asset(100));
      Transfer_Asset(chain, newguy, inita, asset(1));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("newguy"), asset(99));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), asset(100000-199));
      chain.produce_blocks();

      BOOST_CHECK_EQUAL(chain.head_block_num(), 11);
      BOOST_CHECK(chain.fetch_block_by_number(11).valid());
      BOOST_CHECK(!chain.fetch_block_by_number(11)->cycles.empty());
      BOOST_CHECK(!chain.fetch_block_by_number(11)->cycles.front().empty());
      BOOST_CHECK_EQUAL(chain.fetch_block_by_number(11)->cycles.front().front().user_input.size(), 2);
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("newguy"), asset(99));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), asset(100000-199));
} FC_LOG_AND_RETHROW() }

// Simple test of block production when a block is missed
BOOST_FIXTURE_TEST_CASE(missed_blocks, testing_fixture)
{ try {
      Make_Blockchain(chain)

      chain.produce_blocks();
      BOOST_CHECK_EQUAL(chain.head_block_num(), 1);

      account_name skipped_producers[3] = {chain.get_scheduled_producer(1),
                                           chain.get_scheduled_producer(2),
                                           chain.get_scheduled_producer(3)};
      auto next_block_time = chain.get_slot_time(4);
      auto next_producer = chain.get_scheduled_producer(4);

      BOOST_CHECK_EQUAL(chain.head_block_num(), 1);
      chain.produce_blocks(1, 3);
      BOOST_CHECK_EQUAL(chain.head_block_num(), 2);
      BOOST_CHECK_EQUAL(chain.head_block_time().to_iso_string(), next_block_time.to_iso_string());
      BOOST_CHECK_EQUAL(chain.head_block_producer(), next_producer);
      BOOST_CHECK_EQUAL(chain.get_producer(next_producer).total_missed, 0);

      for (auto producer : skipped_producers) {
         BOOST_CHECK_EQUAL(chain.get_producer(producer).total_missed, 1);
      }
} FC_LOG_AND_RETHROW() }

// Simple sanity test of test network: if databases aren't connected to the network, they don't sync to each other
BOOST_FIXTURE_TEST_CASE(no_network, testing_fixture)
{ try {
      Make_Blockchains((chain1)(chain2));

      BOOST_CHECK_EQUAL(chain1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      chain1.produce_blocks();
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      chain2.produce_blocks(5);
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 5);
} FC_LOG_AND_RETHROW() }

// Test that two databases on the same network do sync to each other
BOOST_FIXTURE_TEST_CASE(simple_network, testing_fixture)
{ try {
      Make_Blockchains((chain1)(chain2))
      Make_Network(net, (chain1)(chain2))

      BOOST_CHECK_EQUAL(chain1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      chain1.produce_blocks();
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
      chain2.produce_blocks(5);
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 6);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 6);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Test that two databases joining and leaving a network sync correctly after a fork
BOOST_FIXTURE_TEST_CASE(forked_network, testing_fixture)
{ try {
      Make_Blockchains((chain1)(chain2))
      Make_Network(net)

      BOOST_CHECK_EQUAL(chain1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      chain1.produce_blocks();
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      BOOST_CHECK_NE(chain1.head_block_id().str(), chain2.head_block_id().str());

      net.connect_blockchain(chain1);
      net.connect_blockchain(chain2);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());

      chain2.produce_blocks(5);
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 6);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 6);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());

      net.disconnect_database(chain1);
      chain1.produce_blocks(1, 1);
      chain2.produce_blocks();
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 7);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 7);
      BOOST_CHECK_NE(chain1.head_block_id().str(), chain2.head_block_id().str());

      chain2.produce_blocks(1, 1);
      net.connect_blockchain(chain1);
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 8);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 8);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Check that the recent_slots_filled bitmap is being updated correctly
BOOST_FIXTURE_TEST_CASE( rsf_missed_blocks, testing_fixture )
{ try {
      Make_Blockchain(chain)
      chain.produce_blocks();

      auto rsf = [&]() -> string
      {
         auto rsf = chain.get_dynamic_global_properties().recent_slots_filled;
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
         return uint64_t( config::percent100 ) * x / 64;
      };

      BOOST_CHECK_EQUAL( rsf(),
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), config::percent100 );

      chain.produce_blocks(1, 1);
      BOOST_CHECK_EQUAL( rsf(),
         "0111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(127-64) );

      chain.produce_blocks(1, 1);
      BOOST_CHECK_EQUAL( rsf(),
         "0101111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(126-64) );

      chain.produce_blocks(1, 2);
      BOOST_CHECK_EQUAL( rsf(),
         "0010101111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(124-64) );

      chain.produce_blocks(1, 3);
      BOOST_CHECK_EQUAL( rsf(),
         "0001001010111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(121-64) );

      chain.produce_blocks(1, 5);
      BOOST_CHECK_EQUAL( rsf(),
         "0000010001001010111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(116-64) );

      chain.produce_blocks(1, 8);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000010000010001001010111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(108-64) );

      chain.produce_blocks(1, 13);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000100000000100000100010010101111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1000000000000010000000010000010001001010111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1100000000000001000000001000001000100101011111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1110000000000000100000000100000100010010101111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1111000000000000010000000010000010001001010111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks(1, 64);
      BOOST_CHECK_EQUAL( rsf(),
         "1111111111111111111111111111111111111111111111111111111111111111"
      ); // For very small producer nets.  All zeros otherwise.
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), config::percent100 );

      chain.produce_blocks(1, 63);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000000000000000000000000000000000001"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(1) );

      chain.produce_blocks(1, 32);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000010000000000000000000000000000000"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(1) );
} FC_LOG_AND_RETHROW() }

// Check that a db rewinds to the LIB after being closed and reopened
BOOST_FIXTURE_TEST_CASE(restart_db, testing_fixture)
{ try {
      auto lag = eos_percent(config::blocks_per_round, config::irreversible_threshold_percent);
      {
         Make_Blockchain(chain, x);

         chain.produce_blocks(20);

         BOOST_CHECK_EQUAL(chain.head_block_num(), 20);
         BOOST_CHECK_EQUAL(chain.last_irreversible_block_num(), 20 - lag);
      }

      {
         Make_Blockchain(chain, x);

         // After restarting, we should have rewound to the last irreversible block.
         BOOST_CHECK_EQUAL(chain.head_block_num(), 20 - lag);
         chain.produce_blocks(5);
         BOOST_CHECK_EQUAL(chain.head_block_num(), 25 - lag);
      }
} FC_LOG_AND_RETHROW() }

// Check that a db which is closed and reopened successfully syncs back with the network, including retrieving blocks
// that it missed while it was down
BOOST_FIXTURE_TEST_CASE(sleepy_db, testing_fixture)
{ try {
      Make_Blockchain(producer)
      Make_Network(net, (producer))

      auto lag = eos_percent(config::blocks_per_round, config::irreversible_threshold_percent);
      producer.produce_blocks(20);

      {
         // The new node, sleepy, joins, syncs, disconnects
         Make_Blockchain(sleepy, sleepy)
         net.connect_blockchain(sleepy);
         BOOST_CHECK_EQUAL(producer.head_block_num(), 20);
         BOOST_CHECK_EQUAL(sleepy.head_block_num(), 20);

         net.disconnect_database(sleepy);
      }

      // 5 new blocks are produced
      producer.produce_blocks(5);
      BOOST_CHECK_EQUAL(producer.head_block_num(), 25);

      // Sleepy is reborn! Check that it is now rewound to the LIB...
      Make_Blockchain(sleepy, sleepy)
      BOOST_CHECK_EQUAL(sleepy.head_block_num(), 20 - lag);

      // Reconnect sleepy to the network and check that it syncs up to the present
      net.connect_blockchain(sleepy);
      BOOST_CHECK_EQUAL(sleepy.head_block_num(), 25);
      BOOST_CHECK_EQUAL(sleepy.head_block_id().str(), producer.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Test reindexing the blockchain
BOOST_FIXTURE_TEST_CASE(reindex, testing_fixture)
{ try {
      auto lag = eos_percent(config::blocks_per_round, config::irreversible_threshold_percent);
      {
         chainbase::database db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE);
         block_log log(get_temp_dir("log"));
         fork_database fdb;
         native_contract::native_contract_chain_initializer initr(genesis_state());
         testing_blockchain chain(db, fdb, log, initr, *this);

         chain.produce_blocks(100);

         BOOST_CHECK_EQUAL(chain.last_irreversible_block_num(), 100 - lag);
      }

      {
         chainbase::database db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE);
         block_log log(get_temp_dir("log"));
         fork_database fdb;
         native_contract::native_contract_chain_initializer initr(genesis_state());
         testing_blockchain chain(db, fdb, log, initr, *this);

         BOOST_CHECK_EQUAL(chain.head_block_num(), 100 - lag);
         chain.produce_blocks(20);
         BOOST_CHECK_EQUAL(chain.head_block_num(), 120 - lag);
      }
} FC_LOG_AND_RETHROW() }

// Test wiping a database and resyncing with an ongoing network
BOOST_FIXTURE_TEST_CASE(wipe, testing_fixture)
{ try {
      Make_Blockchains((chain1)(chain2))
      Make_Network(net, (chain1)(chain2))
      {
         // Create db3 with a temporary data dir
         Make_Blockchain(chain3)
         net.connect_blockchain(chain3);

         chain1.produce_blocks(3);
         chain2.produce_blocks(3);
         BOOST_CHECK_EQUAL(chain1.head_block_num(), 6);
         BOOST_CHECK_EQUAL(chain2.head_block_num(), 6);
         BOOST_CHECK_EQUAL(chain3.head_block_num(), 6);
         BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
         BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain3.head_block_id().str());

         net.disconnect_database(chain3);
      }

      {
         // Create new chain3 with a new temporary data dir
         Make_Blockchain(chain3)
         BOOST_CHECK_EQUAL(chain3.head_block_num(), 0);

         net.connect_blockchain(chain3);
         BOOST_CHECK_EQUAL(chain3.head_block_num(), 6);

         chain1.produce_blocks(3);
         chain2.produce_blocks(3);
         BOOST_CHECK_EQUAL(chain1.head_block_num(), 12);
         BOOST_CHECK_EQUAL(chain2.head_block_num(), 12);
         BOOST_CHECK_EQUAL(chain3.head_block_num(), 12);
         BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
         BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain3.head_block_id().str());
      }
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(irrelevant_sig_soft_check, testing_fixture) {
   try {
      Make_Blockchain(chain);
      chain.set_hold_transactions_for_review(true);
      chain.set_skip_transaction_signature_checking(false);
      chain.set_auto_sign_transactions(true);

      // Make an account, but add an extra signature to the transaction
      Make_Account(chain, alice);
      // Check that it throws for irrelevant signatures
      BOOST_CHECK_THROW(chain.review_transaction([](signed_transaction& trx, auto) {
                           trx.sign(fc::ecc::private_key::regenerate(fc::digest("an unknown key")), {});
                           return true;
                        }), tx_irrelevant_sig);
      // Push it through with a skip flag
      chain.review_transaction([](signed_transaction& trx, uint32_t& skip) {
         trx.sign(fc::ecc::private_key::regenerate(fc::digest("an unknown key")), {});
         skip |= chain_controller::skip_transaction_signatures;
         return true;
      });

      // Skip sig checks so we can produce a block with the oversigned transaction
      chain.set_skip_transaction_signature_checking(true);
      chain.produce_blocks();

      // Now check that a second blockchain accepts the block with the oversigned transaction
      Make_Blockchain(newchain);
      Make_Network(net, (chain)(newchain));
      BOOST_CHECK_NE((newchain_db.find<account_object, by_name>("alice")), nullptr);
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
