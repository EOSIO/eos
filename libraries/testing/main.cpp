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



      for( uint32_t i = 0; i < 10; ++i ) {
         auto b = test.produce_block();
         test2.control->push_block( b );
         wdump((b));
      }

      const auto& b = test.control->get_database().get<balance_object, by_owner_name>( N(dan) );
      const auto& sb = test.control->get_database().get<staked_balance_object, by_owner_name>( N(dan) );

      FC_ASSERT( asset( sb.staked_balance ) == asset::from_string("100.0000 EOS") );
      idump((asset(sb.staked_balance)));

      const auto& sb2 = test2.control->get_database().get<staked_balance_object, by_owner_name>( N(dan) );
      FC_ASSERT( sb2.staked_balance == sb.staked_balance );

   } catch ( const fc::exception& e ) {
      elog("${e}", ("e",e.to_detail_string()) );
   }
}
