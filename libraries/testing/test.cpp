#include <eosio/testing/tester.hpp>

using namespace eosio::chain;
using namespace eosio::testing;

int main( int argc, char** argv ) {
   try { try {
      tester c;
      c.produce_block();
      c.produce_block();
      c.produce_block();
      c.create_accounts( {N(dan),N(sam),N(pam)} );
      c.set_producers( {N(dan),N(sam),N(pam)}, 1 );
      c.produce_blocks(60);

      tester c2;
      wlog( "push to c2" );
      while( c2.control->head_block_num() < c.control->head_block_num() ) {
         auto fb = c.control->fetch_block_by_number( c2.control->head_block_num()+1 );
         c2.control->push_block( fb );
      }




   } FC_CAPTURE_AND_RETHROW() 
   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
   }
}
