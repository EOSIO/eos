#include <eosio/testing/tester.hpp>

using namespace eosio;
using namespace eosio::testing;

int main( int argc, char** argv ) {

   try {
      tester test;
      test.produce_blocks(50);

      test.create_account( N(dan), "100.0000 EOS" );

   } catch ( const fc::exception& e ) {
      elog("${e}", ("e",e.to_detail_string()) );
   }
}
