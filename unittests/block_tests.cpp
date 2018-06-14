#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester_network.hpp>
#include <eosio/chain/producer_object.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

//#define NON_VALIDATING_TEST
#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE( last_irreversible_update_bug_test ) { try {
   tester producers;
   tester disconnected;
   vector<account_name> producer_names = {N(inita), N(initb), N(initc), N(initd)};
   producers.create_accounts(producer_names);
   producers.set_producers(producer_names);
   
   signed_block_ptr block = producers.produce_block();
   disconnected.push_block(block);
   while (true) {
      block = producers.produce_block();
      disconnected.push_block(block);
      if ( block->producer == N(inita) )
         break;
   }

   auto produce_one_block = [&]( auto& t, int offset ) {
      t.control->abort_block();
      return t.produce_block(fc::milliseconds(config::block_interval_ms * offset));
   };

   // lets start at inita
   BOOST_CHECK_EQUAL( block->block_num(), 5 );
   BOOST_CHECK_EQUAL( block->producer, N(inita) );
   
   block = produce_one_block( producers, 12 );
   disconnected.push_block(block);
   BOOST_CHECK_EQUAL( block->producer, N(initb) );

   block = produce_one_block( producers, 12 );
   disconnected.push_block(block);
   BOOST_CHECK_EQUAL( block->producer, N(initc) );

   block = produce_one_block( producers, 12 );
   disconnected.push_block(block);
   BOOST_CHECK_EQUAL( block->producer, N(initd) );
   
   // start here
   block = produce_one_block( producers, 12 );
   disconnected.push_block(block);
   BOOST_CHECK_EQUAL( block->block_num(), 9 );
   BOOST_CHECK_EQUAL( block->producer, N(inita) );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 4 );

   block = produce_one_block( producers, 12 );
   disconnected.push_block(block);
   BOOST_CHECK_EQUAL( block->block_num(), 10 );
   BOOST_CHECK_EQUAL( block->producer, N(initb) );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 5 );

   block = produce_one_block( producers, 12 );
   disconnected.push_block(block);
   BOOST_CHECK_EQUAL( block->producer, N(initc) );
   BOOST_CHECK_EQUAL( block->block_num(), 11 );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 6 );

   auto left_fork = block; // disconnected fork (block# 14 )
   auto right_fork = block; // producers fork (block# 15

   // left fork
   left_fork = produce_one_block( disconnected, 12 );
   BOOST_CHECK_EQUAL( left_fork->block_num(), 12 );
   BOOST_CHECK_EQUAL( left_fork->producer, N(initd) );
   BOOST_CHECK_EQUAL( disconnected.control->last_irreversible_block_num(), 7 );

   left_fork = produce_one_block( disconnected, 36 );
   BOOST_CHECK_EQUAL( left_fork->block_num(), 13 );
   BOOST_CHECK_EQUAL( left_fork->producer, N(initc) );
   BOOST_CHECK_EQUAL( disconnected.control->last_irreversible_block_num(), 7 );

   left_fork = produce_one_block( disconnected, 12 );
   BOOST_CHECK_EQUAL( left_fork->block_num(), 14 );
   BOOST_CHECK_EQUAL( left_fork->producer, N(initd) );
   BOOST_CHECK_EQUAL( disconnected.control->last_irreversible_block_num(), 7 );

   // right fork
   right_fork = produce_one_block( producers, 36 );
   BOOST_CHECK_EQUAL( right_fork->block_num(), 12 );
   BOOST_CHECK_EQUAL( right_fork->producer, N(initb) );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 6 );

   right_fork = produce_one_block( producers, 36 );
   BOOST_CHECK_EQUAL( right_fork->block_num(), 13 );
   BOOST_CHECK_EQUAL( right_fork->producer, N(inita) );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 8 );

   right_fork = produce_one_block( producers, 24 );
   BOOST_CHECK_EQUAL( right_fork->block_num(), 14 );
   BOOST_CHECK_EQUAL( right_fork->producer, N(initc) );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 9 );

   right_fork = produce_one_block( producers, 12 );
   BOOST_CHECK_EQUAL( right_fork->block_num(), 15 );
   BOOST_CHECK_EQUAL( right_fork->producer, N(initd) );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 9 );
   
   right_fork = produce_one_block( producers, 12 );
   BOOST_CHECK_EQUAL( right_fork->block_num(), 16 );
   BOOST_CHECK_EQUAL( right_fork->producer, N(inita) );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 11 );

   right_fork = produce_one_block( producers, 12 );
   BOOST_CHECK_EQUAL( right_fork->block_num(), 17 );
   BOOST_CHECK_EQUAL( right_fork->producer, N(initb) );
   BOOST_CHECK_EQUAL( producers.control->last_irreversible_block_num(), 12);

   // TODO This should fail after chain_controller refactor, as this bug should not exist
   BOOST_CHECK_THROW(producers.sync_with( disconnected )  , assert_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( schedule_test ) { try {
  TESTER test;

  for( uint32_t i = 0; i < 200; ++i )
     test.produce_block();

  auto lib = test.control->last_irreversible_block_num();
  auto head = test.control->head_block_num();

  BOOST_CHECK_EQUAL( head - 1 ,  lib );

  //idump((lib)(head));

  test.close();
  test.open();

  auto rlib = test.control->last_irreversible_block_num();
  auto rhead = test.control->head_block_num();

  BOOST_CHECK_EQUAL( rhead ,  head );
  BOOST_CHECK_EQUAL( rlib ,  lib );

  for( uint32_t i = 0; i < 1000; ++i )
     test.produce_block();
  ilog("exiting");
  test.produce_block();
  BOOST_REQUIRE_EQUAL( test.validate(), true );
} FC_LOG_AND_RETHROW() }/// schedule_test

BOOST_AUTO_TEST_CASE(trx_variant ) {
   try {
      TESTER chain;

      // Create account transaction
      signed_transaction trx;
      name new_account_name = name("alice");
      authority owner_auth = authority( chain.get_public_key( new_account_name, "owner" ) );
      trx.actions.emplace_back( vector<permission_level>{{ config::system_account_name, config::active_name}},
                                eosio::chain::newaccount{
                                        .creator  = config::system_account_name,
                                        .name     = new_account_name,
                                        .owner    = owner_auth,
                                        .active   = authority( chain.get_public_key( new_account_name, "active" ) )
                                });
      trx.expiration = time_point_sec(chain.control->head_block_time()) + 100;
      trx.ref_block_num = (uint16_t)chain.control->head_block_num();
      trx.ref_block_prefix = (uint32_t)chain.control->head_block_id()._hash[1];

      auto original = fc::raw::pack( trx );
      // Convert to variant, and back to transaction object
      fc::variant var;
      eosio::chain::abi_serializer::to_variant(trx, var, chain.get_resolver());
      signed_transaction from_var;
      eosio::chain::abi_serializer::from_variant(var, from_var, chain.get_resolver());
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
                            eosio::chain::newaccount{
                               .creator  = config::system_account_name,
                               .name     = new_account_name,
                               .owner    = owner_auth,
                               .active   = authority(chain.get_public_key(new_account_name, "active"))
                            });
   chain.set_transaction_headers(trx, 90);
   trx.sign(chain.get_private_key(config::system_account_name, "active"), chain.control->get_chain_id());
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
                            eosio::chain::newaccount{
                               .creator  = config::system_account_name,
                               .name     = new_account_name,
                               .owner    = owner_auth,
                               .active   = authority(chain.get_public_key(new_account_name, "active"))
                            });
   trx.ref_block_num = static_cast<uint16_t>(chain.control->head_block_num());
   trx.ref_block_prefix = static_cast<uint32_t>(chain.control->head_block_id()._hash[1]);
   trx.sign(chain.get_private_key(config::system_account_name, "active"), chain.control->get_chain_id());
   // Unset expiration should throw
   BOOST_CHECK_THROW(chain.push_transaction(trx), transaction_exception);

   memset(&trx.expiration, 0, sizeof(trx.expiration)); // currently redundant, as default is all zeros, but may not always be.
   trx.sign(chain.get_private_key(config::system_account_name, "active"), chain.control->get_chain_id());
   // Expired transaction (January 1970) should throw
   BOOST_CHECK_THROW(chain.push_transaction(trx), expired_tx_exception);
   BOOST_REQUIRE_EQUAL( chain.validate(), true );
}


BOOST_AUTO_TEST_CASE(transaction_expiration) {

   for (int i = 0; i < 2; ++i) {
      TESTER chain;
      signed_transaction trx;
      name new_account_name = name("alice");
      authority owner_auth = authority(chain.get_public_key( new_account_name, "owner"));
      trx.actions.emplace_back(vector<permission_level>{{config::system_account_name, config::active_name}},
                              eosio::chain::newaccount{
                                 .creator  = config::system_account_name,
                                 .name     = new_account_name,
                                 .owner    = owner_auth,
                                 .active   = authority(chain.get_public_key(new_account_name, "active"))
                              });
      trx.ref_block_num = static_cast<uint16_t>(chain.control->head_block_num());
      trx.ref_block_prefix = static_cast<uint32_t>(chain.control->head_block_id()._hash[1]);
      trx.expiration = chain.control->head_block_time() + fc::microseconds(i * 1000000);
      trx.sign(chain.get_private_key(config::system_account_name, "active"), chain.control->get_chain_id());

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
                           eosio::chain::newaccount{
                              .creator  = config::system_account_name,
                              .name     = new_account_name,
                              .owner    = owner_auth,
                              .active   = authority(chain.get_public_key(new_account_name, "active"))
                           });
   trx.ref_block_num = static_cast<uint16_t>(chain.control->head_block_num() + 1);
   trx.ref_block_prefix = static_cast<uint32_t>(chain.control->head_block_id()._hash[1]);
   trx.expiration = chain.control->head_block_time() + fc::microseconds(1000000);
   trx.sign(chain.get_private_key(config::system_account_name, "active"), chain.control->get_chain_id());

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
                                eosio::chain::newaccount{
                                        .creator  = config::system_account_name,
                                        .name     = new_account_name,
                                        .owner    = owner_auth,
                                        .active   = authority( chain.get_public_key( new_account_name, "active" ) )
                                });
      chain.set_transaction_headers(trx);

      // Add unneeded signature
      trx.sign( chain.get_private_key( name("random"), "active" ), chain.control->get_chain_id()  );

      // Check that it throws for irrelevant signatures
      BOOST_CHECK_THROW(chain.push_transaction( trx ), unsatisfied_authorization);

      BOOST_REQUIRE_EQUAL( chain.validate(), true );
   } FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE(no_auth) {
   try {
      TESTER chain;
      chain.create_account(name("joe"));

      chain.produce_blocks();

      // Use create account transaction as example
      signed_transaction trx;
      name new_account_name = name("alice");
      authority owner_auth = authority( chain.get_public_key( new_account_name, "owner" ) );
      trx.actions.emplace_back( vector<permission_level>{{ config::system_account_name, config::active_name}},
                                eosio::chain::newaccount{
                                      .creator  = config::system_account_name,
                                      .name     = new_account_name,
                                      .owner    = owner_auth,
                                      .active   = authority()
                                });
      chain.set_transaction_headers(trx);
      trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain.control->get_chain_id()  );

      // Check that it throws for no auth
      BOOST_CHECK_THROW(chain.push_transaction( trx ), action_validate_exception);

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

BOOST_AUTO_TEST_SUITE_END()
