#include <eosio/testing/tester.hpp>
#include <fc/io/json.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

using namespace eosio::chain;
using namespace eosio::testing;

int main( int argc, char** argv ) {
   try { try {
      tester c;
      c.produce_block();
      c.produce_block();
      c.produce_block();
      auto r = c.create_accounts( {N(dan),N(sam),N(pam)} );
      wdump((fc::json::to_pretty_string(r)));
      c.set_producers( {N(dan),N(sam),N(pam)}, 1 );
      c.produce_blocks(30);

      c.create_accounts( {N(eosio.token)} );
      c.set_code( N(eosio.token), eosio_token_wast );
      c.set_abi( N(eosio.token), eosio_token_abi );
      c.produce_blocks(10);


      auto cr = c.push_action( N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
                 ("issuer",       "eosio" )
                 ("maximum_supply", "10000000.0000 EOS")
                 ("can_freeze", 0)
                 ("can_recall", 0)
                 ("can_whitelist", 0)
         );

      wdump((fc::json::to_pretty_string(cr)));

      cr = c.push_action( N(eosio.token), N(issue), N(eosio), mutable_variant_object()
                 ("to",       "dan" )
                 ("quantity", "100.0000 EOS")
                 ("memo", "")
         );

      wdump((fc::json::to_pretty_string(cr)));


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
