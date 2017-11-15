#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/balance_object.hpp>
#include <eosio/chain/contracts/staked_balance_objects.hpp>

using namespace eosio;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

int main( int argc, char** argv ) {

   try {
      tester test;
      tester test2;

      for( uint32_t i = 0; i < 50; ++i ) {
        auto b = test.produce_block();
        test2.control->push_block( b );
      }

      test.create_account( N(dan), "100.0000 EOS" );
      test.create_accounts( {N(stan), N(scott)} );

      for( uint32_t i = 0; i < 10; ++i ) {
         auto b = test.produce_block();
         test2.control->push_block( b );
         wdump((b));
      }

      const auto& b = test.get<balance_object, by_owner_name>( N(dan) );
      const auto& sb = test.get<staked_balance_object, by_owner_name>( N(dan) );

      FC_ASSERT( asset( sb.staked_balance ) == asset::from_string("100.0000 EOS") );
      idump((asset(sb.staked_balance)));

      const auto& sb2 = test2.get<staked_balance_object, by_owner_name>( N(dan) );
      FC_ASSERT( sb2.staked_balance == sb.staked_balance );

      test.transfer( N(inita), N(stan), asset::from_string( "20.0000 EOS" ), "hello world" );
      const auto& stan_balance = test.get<balance_object, by_owner_name>( N(stan) );
      FC_ASSERT( stan_balance.balance == 200000 );

   } catch ( const fc::exception& e ) {
      elog("${e}", ("e",e.to_detail_string()) );
   }
}
