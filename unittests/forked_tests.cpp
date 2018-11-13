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

void push_blocks( tester& from, tester& to ) {
   while( to.control->fork_db_head_block_num() < from.control->fork_db_head_block_num() ) {
      auto fb = from.control->fetch_block_by_number( to.control->fork_db_head_block_num()+1 );
      to.push_block( fb );
   }
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

struct fork_tracker {
   vector<signed_block_ptr> blocks;
   incremental_merkle       block_merkle;
};

BOOST_AUTO_TEST_CASE( fork_with_bad_block ) try {
   tester bios;
   bios.produce_block();
   bios.produce_block();
   bios.create_accounts( {N(a),N(b),N(c),N(d),N(e)} );

   bios.produce_block();
   auto res = bios.set_producers( {N(a),N(b),N(c),N(d),N(e)} );

   // run until the producers are installed and its the start of "a's" round
   while( bios.control->pending_block_state()->header.producer.to_string() != "a" || bios.control->head_block_state()->header.producer.to_string() != "e") {
      bios.produce_block();
   }

   // sync remote node
   tester remote;
   push_blocks(bios, remote);

   // produce 6 blocks on bios
   for (int i = 0; i < 6; i ++) {
      bios.produce_block();
      BOOST_REQUIRE_EQUAL( bios.control->head_block_state()->header.producer.to_string(), "a" );
   }

   vector<fork_tracker> forks(7);
   // enough to skip A's blocks
   auto offset = fc::milliseconds(config::block_interval_ms * 13);

   // skip a's blocks on remote
   // create 7 forks of 7 blocks so this fork is longer where the ith block is corrupted
   for (size_t i = 0; i < 7; i ++) {
      auto b = remote.produce_block(offset);
      BOOST_REQUIRE_EQUAL( b->producer.to_string(), "b" );

      for (size_t j = 0; j < 7; j ++) {
         auto& fork = forks.at(j);

         if (j <= i) {
            auto copy_b = std::make_shared<signed_block>(*b);
            if (j == i) {
               // corrupt this block
               fork.block_merkle = remote.control->head_block_state()->blockroot_merkle;
               copy_b->action_mroot._hash[0] ^= 0x1ULL;
            } else if (j < i) {
               // link to a corrupted chain
               copy_b->previous = fork.blocks.back()->id();
            }

            // re-sign the block
            auto header_bmroot = digest_type::hash( std::make_pair( copy_b->digest(), fork.block_merkle.get_root() ) );
            auto sig_digest = digest_type::hash( std::make_pair(header_bmroot, remote.control->head_block_state()->pending_schedule_hash) );
            copy_b->producer_signature = remote.get_private_key(N(b), "active").sign(sig_digest);

            // add this new block to our corrupted block merkle
            fork.block_merkle.append(copy_b->id());
            fork.blocks.emplace_back(copy_b);
         } else {
            fork.blocks.emplace_back(b);
         }
      }

      offset = fc::milliseconds(config::block_interval_ms);
   }

   // go from most corrupted fork to least
   for (size_t i = 0; i < forks.size(); i++) {
      BOOST_TEST_CONTEXT("Testing Fork: " << i) {
         const auto& fork = forks.at(i);
         // push the fork to the original node
         for (int fidx = 0; fidx < fork.blocks.size() - 1; fidx++) {
            const auto& b = fork.blocks.at(fidx);
            // push the block only if its not known already
            if (!bios.control->fetch_block_by_id(b->id())) {
               bios.push_block(b);
            }
         }

         // push the block which should attempt the corrupted fork and fail
         BOOST_REQUIRE_THROW(bios.push_block(fork.blocks.back()), fc::exception);
      }
   }

   // make sure we can still produce a blocks until irreversibility moves
   auto lib = bios.control->head_block_state()->dpos_irreversible_blocknum;
   size_t tries = 0;
   while (bios.control->head_block_state()->dpos_irreversible_blocknum == lib && ++tries < 10000) {
      bios.produce_block();
   }

} FC_LOG_AND_RETHROW();

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
              ("maximum_supply", core_from_string("10000000.0000"))
      );

   wdump((fc::json::to_pretty_string(cr)));

   cr = c.push_action( N(eosio.token), N(issue), config::system_account_name, mutable_variant_object()
              ("to",       "dan" )
              ("quantity", core_from_string("100.0000"))
              ("memo", "")
      );

   wdump((fc::json::to_pretty_string(cr)));


   tester c2;
   wlog( "push c1 blocks to c2" );
   push_blocks(c, c2);
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
   push_blocks(c, c2);
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
   push_blocks(c, c2);
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
   auto bad_block_bs = c.control->create_block_state_future( std::make_shared<signed_block>(bad_block) );
   c.control->abort_block();
   BOOST_REQUIRE_EXCEPTION(c.control->push_block( bad_block_bs ), fc::exception,
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
   push_blocks(c, c2);

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


BOOST_AUTO_TEST_CASE( read_modes ) try {
   tester c;
   c.produce_block();
   c.produce_block();
   auto r = c.create_accounts( {N(dan),N(sam),N(pam)} );
   c.produce_block();
   auto res = c.set_producers( {N(dan),N(sam),N(pam)} );
   c.produce_blocks(200);
   auto head_block_num = c.control->head_block_num();

   tester head(true, db_read_mode::HEAD);
   push_blocks(c, head);
   BOOST_REQUIRE_EQUAL(head_block_num, head.control->fork_db_head_block_num());
   BOOST_REQUIRE_EQUAL(head_block_num, head.control->head_block_num());

   tester read_only(false, db_read_mode::READ_ONLY);
   push_blocks(c, read_only);
   BOOST_REQUIRE_EQUAL(head_block_num, read_only.control->fork_db_head_block_num());
   BOOST_REQUIRE_EQUAL(head_block_num, read_only.control->head_block_num());

   tester irreversible(true, db_read_mode::IRREVERSIBLE);
   push_blocks(c, irreversible);
   BOOST_REQUIRE_EQUAL(head_block_num, irreversible.control->fork_db_head_block_num());
   BOOST_REQUIRE_EQUAL(head_block_num - 49, irreversible.control->head_block_num());

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
