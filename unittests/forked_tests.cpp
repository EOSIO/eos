#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/fork_database.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio::chain;
using namespace eosio::testing;

private_key_type get_private_key( name keyname, string role ) {
   return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(string(keyname)+role));
}

public_key_type  get_public_key( name keyname, string role ){
   return get_private_key( keyname, role ).get_public_key();
}

BOOST_AUTO_TEST_SUITE(forked_tests)

BOOST_AUTO_TEST_CASE( irrblock ) try {
   tester c;
   c.produce_blocks(10);
   auto r = c.create_accounts( {N(dan),N(sam),N(pam),N(scott)} );
   auto res = c.set_producers( {N(dan),N(sam),N(pam),N(scott)} );
   vector<producer_key> sch = { {N(dan),get_public_key(N(dan), "active")},
                                {N(sam),get_public_key(N(sam), "active")},
                                {N(scott),get_public_key(N(scott), "active")},
                                {N(pam),get_public_key(N(pam), "active")}
                              };
   wlog("set producer schedule to [dan,sam,pam]");
   c.produce_blocks(50);

} FC_LOG_AND_RETHROW() 

BOOST_AUTO_TEST_CASE( forking ) try {
   tester c;
   c.produce_block();
   c.produce_block();
   auto r = c.create_accounts( {N(dan),N(sam),N(pam)} );
   wdump((fc::json::to_pretty_string(r)));
   c.produce_block();
   auto res = c.set_producers( {N(dan),N(sam),N(pam)} );
   vector<producer_key> sch = { {N(dan),get_public_key(N(dan), "active")},
                                {N(sam),get_public_key(N(sam), "active")},
                                {N(pam),get_public_key(N(pam), "active")}};
   wdump((fc::json::to_pretty_string(res)));
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
      c2.push_block( fb );
   }
   wlog( "end push c1 blocks to c2" );

   wlog( "c1 blocks:" );
   c.produce_blocks(3);
   signed_block_ptr b;
   b = c.produce_block();
   account_name expected_producer = N(dan);
   BOOST_REQUIRE_EQUAL( b->producer.to_string(), expected_producer.to_string() );

   b = c.produce_block();
   expected_producer = N(sam);
   BOOST_REQUIRE_EQUAL( b->producer.to_string(), expected_producer.to_string() );
   c.produce_blocks(10);
   c.create_accounts( {N(cam)} );
   c.set_producers( {N(dan),N(sam),N(pam),N(cam)} );
   wlog("set producer schedule to [dan,sam,pam,cam]");
   c.produce_block();
   // The next block should be produced by pam.

   // Sync second chain with first chain.
   wlog( "push c1 blocks to c2" );
   while( c2.control->head_block_num() < c.control->head_block_num() ) {
      auto fb = c.control->fetch_block_by_number( c2.control->head_block_num()+1 );
      c2.push_block( fb );
   }
   wlog( "end push c1 blocks to c2" );

   // Now sam and pam go on their own fork while dan is producing blocks by himself.

   wlog( "sam and pam go off on their own fork on c2 while dan produces blocks by himself in c1" );
   auto fork_block_num = c.control->head_block_num();

   wlog( "c2 blocks:" );
   c2.produce_blocks(12); // pam produces 12 blocks
   b = c2.produce_block( fc::milliseconds(config::block_interval_ms * 13) ); // sam skips over dan's blocks
   expected_producer = N(sam);
   BOOST_REQUIRE_EQUAL( b->producer.to_string(), expected_producer.to_string() );
   c2.produce_blocks(11 + 12);


   wlog( "c1 blocks:" );
   b = c.produce_block( fc::milliseconds(config::block_interval_ms * 13) ); // dan skips over pam's blocks
   expected_producer = N(dan);
   BOOST_REQUIRE_EQUAL( b->producer.to_string(), expected_producer.to_string() );
   c.produce_blocks(11);

   // dan on chain 1 now gets all of the blocks from chain 2 which should cause fork switch
   wlog( "push c2 blocks to c1" );
   for( uint32_t start = fork_block_num + 1, end = c2.control->head_block_num(); start <= end; ++start ) {
      wdump((start));
      auto fb = c2.control->fetch_block_by_number( start );
      c.push_block( fb );
   }
   wlog( "end push c2 blocks to c1" );

   wlog( "c1 blocks:" );
   c.produce_blocks(24);

   b = c.produce_block(); // Switching active schedule to version 2 happens in this block.
   expected_producer = N(pam);
   BOOST_REQUIRE_EQUAL( b->producer.to_string(), expected_producer.to_string() );

   b = c.produce_block();
   expected_producer = N(cam);
//   BOOST_REQUIRE_EQUAL( b->producer.to_string(), expected_producer.to_string() );
   c.produce_blocks(10);

   wlog( "push c1 blocks to c2" );
   while( c2.control->head_block_num() < c.control->head_block_num() ) {
      auto fb = c.control->fetch_block_by_number( c2.control->head_block_num()+1 );
      c2.push_block( fb );
   }
   wlog( "end push c1 blocks to c2" );

   // Now with four block producers active and two identical chains (for now),
   // we can test out the case that would trigger the bug in the old fork db code:
   fork_block_num = c.control->head_block_num();
   wlog( "cam and dan go off on their own fork on c1 while sam and pam go off on their own fork on c2" );
   wlog( "c1 blocks:" );
   c.produce_blocks(12); // dan produces 12 blocks
   c.produce_block( fc::milliseconds(config::block_interval_ms * 25) ); // cam skips over sam and pam's blocks
   c.produce_blocks(23); // cam finishes the remaining 11 blocks then dan produces his 12 blocks
   wlog( "c2 blocks:" );
   c2.produce_block( fc::milliseconds(config::block_interval_ms * 25) ); // pam skips over dan and sam's blocks
   c2.produce_blocks(11); // pam finishes the remaining 11 blocks
   c2.produce_block( fc::milliseconds(config::block_interval_ms * 25) ); // sam skips over cam and dan's blocks
   c2.produce_blocks(11); // sam finishes the remaining 11 blocks

   wlog( "now cam and dan rejoin sam and pam on c2" );
   c2.produce_block( fc::milliseconds(config::block_interval_ms * 13) ); // cam skips over pam's blocks (this block triggers a block on this branch to become irreversible)
   c2.produce_blocks(11); // cam produces the remaining 11 blocks
   b = c2.produce_block(); // dan produces a block

   // a node on chain 1 now gets all but the last block from chain 2 which should cause a fork switch
   wlog( "push c2 blocks (except for the last block by dan) to c1" );
   for( uint32_t start = fork_block_num + 1, end = c2.control->head_block_num() - 1; start <= end; ++start ) {
      auto fb = c2.control->fetch_block_by_number( start );
      c.push_block( fb );
   }
   wlog( "end push c2 blocks to c1" );
   wlog( "now push dan's block to c1 but first corrupt it so it is a bad block" );
   auto bad_block = *b;
   bad_block.transaction_mroot = bad_block.previous;
   c.control->abort_block();
   BOOST_REQUIRE_EXCEPTION(c.control->push_block( std::make_shared<signed_block>(bad_block) ), fc::exception,
      [] (const fc::exception &ex)->bool {
         return ex.to_detail_string().find("block not signed by expected key") != std::string::npos;
      });
} FC_LOG_AND_RETHROW()


/**
 *  This test verifies that the fork-choice rule favors the branch with
 *  the highest last irreversible block over one that is longer.
 */
BOOST_AUTO_TEST_CASE( prune_remove_branch ) try {
   tester c;
   c.produce_blocks(10);
   auto r = c.create_accounts( {N(dan),N(sam),N(pam),N(scott)} );
   auto res = c.set_producers( {N(dan),N(sam),N(pam),N(scott)} );
   wlog("set producer schedule to [dan,sam,pam,scott]");
   c.produce_blocks(50);

   tester c2;
   wlog( "push c1 blocks to c2" );
   while( c2.control->head_block_num() < c.control->head_block_num() ) {
      auto fb = c.control->fetch_block_by_number( c2.control->head_block_num()+1 );
      c2.push_block( fb );
   }

   // fork happen after block 61
   BOOST_REQUIRE_EQUAL(61, c.control->head_block_num());
   BOOST_REQUIRE_EQUAL(61, c2.control->head_block_num());

   int fork_num = c.control->head_block_num();

   auto nextproducer = [](tester &c, int skip_interval) ->account_name {
      auto head_time = c.control->head_block_time();
      auto next_time = head_time + fc::milliseconds(config::block_interval_ms * skip_interval);
      return c.control->head_block_state()->get_scheduled_producer(next_time).producer_name;   
   };

   // fork c: 2 producers: dan, sam
   // fork c2: 1 producer: scott
   int skip1 = 1, skip2 = 1;
   for (int i = 0; i < 50; ++i) {
      account_name next1 = nextproducer(c, skip1);
      if (next1 == N(dan) || next1 == N(sam)) {
         c.produce_block(fc::milliseconds(config::block_interval_ms * skip1)); skip1 = 1;
      } 
      else ++skip1;
      account_name next2 = nextproducer(c2, skip2);
      if (next2 == N(scott)) {
         c2.produce_block(fc::milliseconds(config::block_interval_ms * skip2)); skip2 = 1;
      } 
      else ++skip2;
   }

   BOOST_REQUIRE_EQUAL(87, c.control->head_block_num());
   BOOST_REQUIRE_EQUAL(73, c2.control->head_block_num());
   
   // push fork from c2 => c
   int p = fork_num;
   while ( p < c2.control->head_block_num()) {
      auto fb = c2.control->fetch_block_by_number(++p);
      c.push_block(fb);
   }

   BOOST_REQUIRE_EQUAL(73, c.control->head_block_num());

} FC_LOG_AND_RETHROW() 

BOOST_AUTO_TEST_CASE(confirmation) try {

   tester c;
   c.produce_blocks(10);
   auto r = c.create_accounts( {N(dan),N(sam),N(pam),N(scott)} );
   auto res = c.set_producers( {N(dan),N(sam),N(pam),N(scott)} );

   private_key_type priv_sam = c.get_private_key( N(sam), "active" );
   private_key_type priv_dan = c.get_private_key( N(dan), "active" );
   private_key_type priv_pam = c.get_private_key( N(pam), "active" );
   private_key_type priv_scott = c.get_private_key( N(scott), "active" );
   private_key_type priv_invalid = c.get_private_key( N(invalid), "active" );

   wlog("set producer schedule to [dan,sam,pam,scott]");
   c.produce_blocks(50);
   c.control->abort_block(); // discard pending block

   BOOST_REQUIRE_EQUAL(61, c.control->head_block_num());

   // 55 is by dan
   block_state_ptr blk = c.control->fork_db().get_block_in_current_chain_by_num(55);
   block_state_ptr blk61 = c.control->fork_db().get_block_in_current_chain_by_num(61);
   block_state_ptr blk50 = c.control->fork_db().get_block_in_current_chain_by_num(50);

   BOOST_REQUIRE_EQUAL(0, blk->bft_irreversible_blocknum);
   BOOST_REQUIRE_EQUAL(0, blk->confirmations.size());

   // invalid signature
   BOOST_REQUIRE_EXCEPTION(c.control->push_confirmation(header_confirmation{blk->id, N(sam), priv_invalid.sign(blk->sig_digest())}),
      fc::exception,
      [] (const fc::exception &ex)->bool {
      return ex.to_detail_string().find("confirmation not signed by expected key") != std::string::npos;
      });

   // invalid schedule
   BOOST_REQUIRE_EXCEPTION(c.control->push_confirmation(header_confirmation{blk->id, N(invalid), priv_invalid.sign(blk->sig_digest())}),
      fc::exception,
      [] (const fc::exception &ex)->bool {
      return ex.to_detail_string().find("producer not in current schedule") != std::string::npos;
      });

   // signed by sam
   c.control->push_confirmation(header_confirmation{blk->id, N(sam), priv_sam.sign(blk->sig_digest())});

   BOOST_REQUIRE_EQUAL(0, blk->bft_irreversible_blocknum);
   BOOST_REQUIRE_EQUAL(1, blk->confirmations.size());

   // double confirm not allowed
   BOOST_REQUIRE_EXCEPTION(c.control->push_confirmation(header_confirmation{blk->id, N(sam), priv_sam.sign(blk->sig_digest())}),
      fc::exception,
      [] (const fc::exception &ex)->bool {
      return ex.to_detail_string().find("block already confirmed by this producer") != std::string::npos;
      });

   // signed by dan
   c.control->push_confirmation(header_confirmation{blk->id, N(dan), priv_dan.sign(blk->sig_digest())});

   BOOST_REQUIRE_EQUAL(0, blk->bft_irreversible_blocknum);
   BOOST_REQUIRE_EQUAL(2, blk->confirmations.size());

   // signed by pam
   c.control->push_confirmation(header_confirmation{blk->id, N(pam), priv_pam.sign(blk->sig_digest())});

   // we have more than 2/3 of confirmations, bft irreversible number should be set
   BOOST_REQUIRE_EQUAL(55, blk->bft_irreversible_blocknum);
   BOOST_REQUIRE_EQUAL(55, blk61->bft_irreversible_blocknum); // bft irreversible number will propagate to higher block
   BOOST_REQUIRE_EQUAL(0, blk50->bft_irreversible_blocknum); // bft irreversible number will not propagate to lower block
   BOOST_REQUIRE_EQUAL(3, blk->confirmations.size());

   // signed by scott
   c.control->push_confirmation(header_confirmation{blk->id, N(scott), priv_scott.sign(blk->sig_digest())});

   BOOST_REQUIRE_EQUAL(55, blk->bft_irreversible_blocknum);
   BOOST_REQUIRE_EQUAL(4, blk->confirmations.size());

   // let's confirm block 50 as well
   c.control->push_confirmation(header_confirmation{blk50->id, N(sam), priv_sam.sign(blk50->sig_digest())});
   c.control->push_confirmation(header_confirmation{blk50->id, N(dan), priv_dan.sign(blk50->sig_digest())});
   c.control->push_confirmation(header_confirmation{blk50->id, N(pam), priv_pam.sign(blk50->sig_digest())});
   BOOST_REQUIRE_EQUAL(50, blk50->bft_irreversible_blocknum); // bft irreversible number will not propagate to lower block

   block_state_ptr blk54 = c.control->fork_db().get_block_in_current_chain_by_num(54);
   BOOST_REQUIRE_EQUAL(50, blk54->bft_irreversible_blocknum);
   BOOST_REQUIRE_EQUAL(55, blk->bft_irreversible_blocknum); // bft irreversible number will not be updated to lower value
   BOOST_REQUIRE_EQUAL(55, blk61->bft_irreversible_blocknum);

   c.produce_blocks(20);

   block_state_ptr blk81 = c.control->fork_db().get_block_in_current_chain_by_num(81);
   BOOST_REQUIRE_EQUAL(55, blk81->bft_irreversible_blocknum); // bft irreversible number will propagate into new blocks

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
