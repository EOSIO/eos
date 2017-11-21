#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>

#include <eosio/chain/contracts/balance_object.hpp>
#include <eosio/chain/contracts/staked_balance_objects.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE(transfer_tests)



BOOST_AUTO_TEST_CASE( transfer_test ) { try {
  tester test;
  test.produce_blocks(2);

  test.create_accounts( {N(dan), N(bart)} );
  test.transfer( N(inita), N(dan), "10.0000 EOS", "memo" );

  {
     const auto& dans_balance = test.get<balance_object,by_owner_name>( N(dan) );
     FC_ASSERT( dans_balance.balance == asset::from_string("10.0000 EOS") );
  }

  test.produce_block();

  {
     const auto& dans_balance = test.get<balance_object,by_owner_name>( N(dan) );
     FC_ASSERT( dans_balance.balance == asset::from_string("10.0000 EOS") );
  }

  /// insufficient funds
  BOOST_REQUIRE_THROW(test.transfer(N(dan),N(bart), "11.0000 EOS", "memo"), action_validate_exception);

  /// this should succeed because dan has sufficient balance
  test.transfer(N(dan),N(bart), "10.0000 EOS", "memo");


  /// verify that bart now has 10.000
  const auto& barts_balance = test.get<balance_object,by_owner_name>( N(bart) );
  FC_ASSERT( barts_balance.balance == asset::from_string("10.0000 EOS") );

  {
     /// verify that dan now has 0.000
     const auto& dans_balance = test.get<balance_object,by_owner_name>( N(dan) );
     FC_ASSERT( dans_balance.balance == asset::from_string("0.0000 EOS") );
  }


  /// should throw because -1 becomes uint64 max which is greater than balance
  BOOST_REQUIRE_THROW(test.transfer(N(bart),N(dan), asset(-1), "memo"), action_validate_exception);



  {
      auto from = N(bart);
      auto to   = N(dan);
      asset amount(1);
      signed_transaction trx;
      trx.write_scope = {from,to};
      trx.actions.emplace_back( vector<permission_level>{{from,config::active_name}},
                                contracts::transfer{
                                   .from   = from,
                                   .to     = to,
                                   .amount = amount.amount,
                                   .memo   = "memo" 
                                } );

      test.set_tapos( trx );
      BOOST_REQUIRE_THROW( test.control->push_transaction( trx ), tx_missing_sigs );
      trx.sign( test.get_private_key( from, "active" ), chain_id_type()  ); 
      test.control->push_transaction( trx );
  }

  {
      auto from = N(bart);
      auto to   = N(dan);
      asset amount(1);
      signed_transaction trx;
      trx.write_scope = {from,to};
      trx.actions.emplace_back( vector<permission_level>{{to,config::active_name}},
                                contracts::transfer{
                                   .from   = from,
                                   .to     = to,
                                   .amount = amount.amount,
                                   .memo   = "memo" 
                                } );

      test.set_tapos( trx );
      trx.sign( test.get_private_key( to, "active" ), chain_id_type()  ); 
      /// action not provided from authority
      BOOST_REQUIRE_THROW( test.control->push_transaction( trx ), tx_missing_auth);
  }

  {
      auto from = N(bart);
      auto to   = N(dan);
      asset amount(1);
      signed_transaction trx;
     // trx.write_scope = {from,to};
      trx.actions.emplace_back( vector<permission_level>{{from,config::active_name}},
                                contracts::transfer{
                                   .from   = from,
                                   .to     = to,
                                   .amount = amount.amount,
                                   .memo   = "memo" 
                                } );

      test.set_tapos( trx );
      trx.sign( test.get_private_key( from, "active" ), chain_id_type()  ); 
      BOOST_REQUIRE_THROW( test.control->push_transaction( trx ), tx_missing_write_scope);
  }

} FC_LOG_AND_RETHROW() } /// transfer_test


/**
 *  This test case will attempt to allow one account to transfer on behalf
 *  of another account by updating the active authority.
 */
BOOST_AUTO_TEST_CASE( transfer_delegation ) { try {
   tester test;
   test.produce_blocks(2);
   test.create_accounts( {N(dan),N(bart),N(trust)} );
   test.transfer( N(inita), N(dan), "10.0000 EOS" );
   test.transfer( N(inita), N(trust), "10.0000 EOS" );
   test.transfer( N(trust), N(dan), "1.0000 EOS" );

   test.produce_block();

   auto dans_active_auth = authority( 1, {}, 
                          {
                            { .permission = {N(trust),config::active_name}, .weight = 1}
                          });

   test.set_authority( N(dan), config::active_name,  dans_active_auth );

   const auto& danauth = test.control->get_permission( permission_level{N(dan),config::active_name} );
   const auto& trustauth = test.control->get_permission( permission_level{N(trust),config::active_name} );


   test.produce_block();

  /// execute a transfer from dan to bart signed by trust
  {
      auto from = N(dan);
      auto to   = N(bart);
      asset amount(1);

      signed_transaction trx;
      trx.write_scope = {from,to};
      trx.actions.emplace_back( vector<permission_level>{{from,config::active_name}},
                                contracts::transfer{
                                   .from   = from,
                                   .to     = to,
                                   .amount = amount.amount,
                                   .memo   = "memo" 
                                } );

      test.set_tapos( trx );
      trx.sign( test.get_private_key( N(trust), "active" ), chain_id_type()  ); 

      /// action not provided from authority
      test.control->push_transaction( trx );
  }

} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_SUITE_END()
