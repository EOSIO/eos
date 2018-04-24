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
      wlog("set producer schedule to [dan,sam,pam]");
      c.produce_blocks(30);

      auto r2 = c.create_accounts( {N(eosio.token)} );
      wdump((fc::json::to_pretty_string(r2)));
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
      wlog( "push c1 blocks to c2" );
      while( c2.control->head_block_num() < c.control->head_block_num() ) {
         auto fb = c.control->fetch_block_by_number( c2.control->head_block_num()+1 );
         c2.control->push_block( fb );
      }
      wlog( "end push c1 blocks to c2" );

      wlog( "c1 blocks:" );
      c.produce_blocks(3);
      signed_block_ptr b;
      b = c.produce_block();
      account_name expected_producer = N(dan);
      FC_ASSERT( b->producer == expected_producer,
                 "expected block ${n} to be produced by ${expected_producer} but was instead produced by ${actual_producer}",
                ("n", b->block_num())("expected_producer", expected_producer.to_string())("actual_producer", b->producer.to_string()) );

      b = c.produce_block();
      expected_producer = N(sam);
      FC_ASSERT( b->producer == expected_producer,
                 "expected block ${n} to be produced by ${expected_producer} but was instead produced by ${actual_producer}",
                ("n", b->block_num())("expected_producer", expected_producer.to_string())("actual_producer", b->producer.to_string()) );
      c.produce_blocks(10);
      c.create_accounts( {N(cam)} );
      c.set_producers( {N(dan),N(sam),N(pam),N(cam)}, 2 );
      wlog("set producer schedule to [dan,sam,pam,cam]");
      c.produce_block();
      // The next block should be produced by pam.

      // Sync second chain with first chain.
      wlog( "push c1 blocks to c2" );
      while( c2.control->head_block_num() < c.control->head_block_num() ) {
         auto fb = c.control->fetch_block_by_number( c2.control->head_block_num()+1 );
         c2.control->push_block( fb );
      }
      wlog( "end push c1 blocks to c2" );

      // Now sam and pam go on their own fork while dan is producing blocks by himself.

      wlog( "sam and pam go off on their own fork on c2 while dan produces blocks by himself in c1" );
      auto fork_block_num = c.control->head_block_num();

      wlog( "c2 blocks:" );
      c2.produce_blocks(12); // pam produces 12 blocks
      b = c2.produce_block( fc::milliseconds(config::block_interval_ms * 13) ); // sam skips over dan's blocks
      expected_producer = N(sam);
      FC_ASSERT( b->producer == expected_producer,
                 "expected block ${n} to be produced by ${expected_producer} but was instead produced by ${actual_producer}",
                ("n", b->block_num())("expected_producer", expected_producer.to_string())("actual_producer", b->producer.to_string()) );
      c2.produce_blocks(11 + 12);


      wlog( "c1 blocks:" );
      b = c.produce_block( fc::milliseconds(config::block_interval_ms * 13) ); // dan skips over pam's blocks
      expected_producer = N(dan);
      FC_ASSERT( b->producer == expected_producer,
                 "expected block ${n} to be produced by ${expected_producer} but was instead produced by ${actual_producer}",
                ("n", b->block_num())("expected_producer", expected_producer.to_string())("actual_producer", b->producer.to_string()) );
      c.produce_blocks(11);

      // dan on chain 1 now gets all of the blocks from chain 2 which should cause fork switch
      wlog( "push c2 blocks to c1" );
      for( uint32_t start = fork_block_num + 1, end = c2.control->head_block_num(); start <= end; ++start ) {
         auto fb = c2.control->fetch_block_by_number( start );
         c.control->push_block( fb );
      }
      wlog( "end push c2 blocks to c1" );

      wlog( "c1 blocks:" );
      c.produce_blocks(24);

      b = c.produce_block(); // Switching active schedule to version 2 happens in this block.
      expected_producer = N(pam);
      FC_ASSERT( b->producer == expected_producer,
                 "expected block ${n} to be produced by ${expected_producer} but was instead produced by ${actual_producer}",
                ("n", b->block_num())("expected_producer", expected_producer.to_string())("actual_producer", b->producer.to_string()) );

      b = c.produce_block();
      expected_producer = N(cam);
      FC_ASSERT( b->producer == expected_producer,
                 "expected block ${n} to be produced by ${expected_producer} but was instead produced by ${actual_producer}",
                ("n", b->block_num())("expected_producer", expected_producer.to_string())("actual_producer", b->producer.to_string()) );


   } FC_CAPTURE_AND_RETHROW()
   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
   }
}
