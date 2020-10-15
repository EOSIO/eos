#include <eosio/amqp_compressed_proof_plugin/amqp_compressed_proof_plugin.hpp>
#include <eosio/testing/tester.hpp>

#include <boost/test/data/test_case.hpp>

#include "compressed_proof_validator.hpp"

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

static const char fail_everything_wast[] = R"=====(
(module
 (export "apply" (func $apply))
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
  (unreachable)
 )
)
)=====";

static const char inliner_wast[] = R"=====(
(module
 (export "apply" (func $apply))
 (import "env" "send_inline" (func $send_inline (param i32 i32)))
 (memory $0 1)
 (data (i32.const  4) "\00\00\00\00\00\00\00\00")             ;;account (to be filled)
 (data (i32.const 12) "\00\00\00\00\00\00\00\00")             ;;action (to be filled)
 (data (i32.const 20) "\00")                                  ;;no permission (will get eosio.code implicitly)
 (data (i32.const 21) "\00")                                  ;;no data
 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (i64.store (i32.const 4) (get_local $0))                   ;;copy over account
   (set_local $2 (i64.sub (get_local $2) (i64.const 1)))      ;;subtract 1 from action
   (if (i64.eq (get_local $2) (i64.const 0)) (then            ;;if action is 0 now, return
    (return)
   ))
   (i64.store (i32.const 12) (get_local $2))                  ;;copy over action
   (loop
     (call $send_inline (i32.const 4) (i32.const 18))
     (set_local $2 (i64.sub (get_local $2) (i64.const 1)))    ;;subtract 1 from loop
     (br_if 0 (i32.wrap/i64 (get_local $2)))                  ;;if non-zero loop back around
   )
 )
)
)=====";

static const char send_def_then_blowup_wast[] = R"=====(
(module
 (export "apply" (func $apply))
 (import "env" "send_deferred" (func $send_deferred (param i32 i64 i32 i32 i32)))
 (memory $0 1)

 ;; construct payload for a deferred trx
 (data (i32.const 500) "\00\00\00\00")                      ;;expiration
 (data (i32.const 504) "\00\00\00\00\00\00")                ;;ref_block_num & ref_block_prefix
 (data (i32.const 510) "\7f")                               ;;max_net_usage_words
 (data (i32.const 511) "\7f")                               ;;max_cpu_usage_ms
 (data (i32.const 512) "\02")                               ;;delay_sec
 (data (i32.const 513) "\00")                               ;;no CFA
 (data (i32.const 514) "\01")                               ;;one action
   (data (i32.const 515) "\00\00\00\00\00\00\00\00")        ;;account (to be filled)
   (data (i32.const 523) "\00\00\00\e0\aa\a5\12\4d")        ;;action: "dodefer"
   (data (i32.const 531) "\01")                             ;;one permission
     (data (i32.const 532) "\00\00\00\00\00\00\00\00")      ;;account (to be filled)
     (data (i32.const 540) "\00\00\00\00\a8\ed\32\32")      ;;active
   (data (i32.const 548) "\00")                             ;;nodata
 (data (i32.const 549) "\00")                               ;;no trx extensions

 (func $apply (param $0 i64) (param $1 i64) (param $2 i64)
   (i64.store (i32.const 515) (get_local $0))                   ;;copy over account
   (i64.store (i32.const 532) (get_local $0))                   ;;copy over account

   ;;if action is "dodefer", blow up
   (if (i64.eq (get_local $2) (i64.const 0x4d12a5aae0000000)) (then
    (unreachable)
   ))
   ;;if action is "dothedew", send the deferred trx
   (if (i64.eq (get_local $2) (i64.const 0x4d32d5255c000000)) (then
    (call $send_deferred (i32.const 4) (get_local $0) (i32.const 500) (i32.const 512) (i32.const 0))
   ))
   ;;other actions, like "onerror", fall through successfully
 )
)
)=====";

static const char test_account_abi[] = R"=====(
{
   "version": "eosio::abi/1.0",
   "types": [],
   "structs": [{"name":"dothedew", "base": "", "fields": [{"name":"nonce", "type":"uint32"}]}],
   "actions": [{"name":"dothedew", "type": "dothedew", "ricardian_contract":""}],
   "tables": [],
   "ricardian_clauses": [],
   "variants": [],
   "action_results": []
}
)=====";

static const char multi_account_abi[] = R"=====(
{
   "version": "eosio::abi/1.0",
   "types": [],
   "structs": [{"name":"dothedew", "base": "", "fields": [{"name":"nonce", "type":"uint32"}]}],
   "actions": [{"name":"apple", "type": "dothedew", "ricardian_contract":""},
               {"name":"banana", "type": "dothedew", "ricardian_contract":""},
               {"name":"carrot", "type": "dothedew", "ricardian_contract":""}],
   "tables": [],
   "ricardian_clauses": [],
   "variants": [],
   "action_results": []
}
)=====";

BOOST_AUTO_TEST_SUITE(test_compressed_proof)

BOOST_AUTO_TEST_CASE(test_proof_no_actions) try {
   tester chain(setup_policy::none);
   //I want to very precisely control the exact actions in the block; disable onblock
   chain.set_code(N(eosio), fail_everything_wast);
   chain.produce_block();

   compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
      [&](const auto&){return true;},  //include all actions in proof
      [&](auto bsp, auto&& x) {
         //but no actions in block means no callback should ever be fired
         BOOST_FAIL("Callback should not be called");
      }
   )});

   chain.produce_blocks(2);
} FC_LOG_AND_RETHROW()

//This test will create 1 to n actions in a block. For each of n, it will then test all permutations of
// interested vs non-interested actions. That is, when n is 1, there will be 1 action in a block which
// will be tested as an interested action, and then tested as a non-interested action. When n is 2, there
// will be 4 total combinations tried, etc
BOOST_DATA_TEST_CASE(test_proof_actions, boost::unit_test::data::xrange(1u, 10u), n_actions) try {
   tester chain(setup_policy::none);
   chain.create_accounts({N(interested), N(nointerested)});
   chain.set_abi(N(interested), test_account_abi);
   chain.set_abi(N(nointerested), test_account_abi);
   chain.produce_block();
   //this test wants to very precisely control the exact actions in the block; disable onblock
   chain.set_code(N(eosio), fail_everything_wast);
   chain.produce_block();

   bool expecting_callback = false;
   fc::sha256 computed_action_mroot;

   compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
      [&](const chain::action& act){
         return act.account == N(interested);
      },
      [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
         BOOST_CHECK(expecting_callback);
         computed_action_mroot = validate_compressed_merkle_proof(serialized_compressed_proof, [](uint64_t receiver, uint64_t action) {
            BOOST_CHECK(name(receiver) == N(interested));
         });
      }
   )});

   unsigned nonce = 0;
   for(unsigned action_mask = 0; action_mask < 1<<n_actions; action_mask++) {
      for(unsigned action_bit = 0; action_bit < n_actions; ++action_bit) {
         bool push_interested = action_mask&1<<action_bit;

         if(push_interested)
            chain.push_action(N(interested), N(dothedew), N(interested), fc::mutable_variant_object()("nonce", nonce++));
         else
            chain.push_action(N(nointerested), N(dothedew), N(nointerested), fc::mutable_variant_object()("nonce", nonce++));
      }

      expecting_callback = action_mask; //when action_mask is 0, no interested action pushed meaning no callback called
      fc::sha256 expected_action_mroot = chain.produce_block()->action_mroot;

      if(expecting_callback)
         BOOST_CHECK(expected_action_mroot == computed_action_mroot);
   }

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE(test_proof_arv_activation) try {
   tester chain(setup_policy::none);
   fc::sha256 computed_action_mroot;

   compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
      [&](const chain::action& act){
         return true;
      },
      [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
         computed_action_mroot = validate_compressed_merkle_proof(serialized_compressed_proof, [](uint64_t receiver, uint64_t action) {});
      }
   )});

   auto produce_and_check = [&]() {
      sha256 new_mroot = chain.produce_block()->action_mroot;
      BOOST_CHECK(new_mroot == computed_action_mroot);
   };
   chain.execute_setup_policy(setup_policy::preactivate_feature_and_new_bios);

   chain.create_account(N(interested));
   chain.set_abi(N(interested), test_account_abi);
   produce_and_check();
   produce_and_check();

   //activate ARV
   const auto& pfm = chain.control->get_protocol_feature_manager();
   auto d = pfm.get_builtin_digest(builtin_protocol_feature_t::action_return_value);
   chain.preactivate_protocol_features({*d});

   //put something else in the same block too
   chain.push_action(N(interested), N(dothedew), N(interested), fc::mutable_variant_object()("nonce", 0));

   produce_and_check();
   produce_and_check();

   //another post activation test
   chain.push_action(N(interested), N(dothedew), N(interested), fc::mutable_variant_object()("nonce", 0));

   produce_and_check();
   produce_and_check();
} FC_LOG_AND_RETHROW()

//This test isn't as interesting now that reversibility is completely handled by db mode, keeping it around anyways
BOOST_AUTO_TEST_CASE(test_proof_presist_reversible) try {
   tester base_chain(setup_policy::none);
   base_chain.produce_blocks(10);

   auto push_blocks = [](tester& from, tester& to, uint32_t block_num_limit) {
      while(to.control->fork_db_pending_head_block_num()
               < std::min( from.control->fork_db_pending_head_block_num(), block_num_limit)) {
         auto fb = from.control->fetch_block_by_number( to.control->fork_db_pending_head_block_num()+1 );
         to.push_block( fb );
      }
   };

   tester chain(setup_policy::none, chain::db_read_mode::IRREVERSIBLE);
   unsigned blocks_seen = 0;

   {
      compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
         [&](const chain::action& act){
            return true;
         },
         [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
            blocks_seen++;
         }
      )});

      push_blocks(base_chain, chain, 6); //make 4 irreversible blocks (2, 3, 4, 5); calling result callback 4 times
   }

   {
      compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
         [&](const chain::action& act){
            return true;
         },
         [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
            blocks_seen++;
         }
      )});

      push_blocks(base_chain, chain, 7); //makes one more irreversible block
   }

   //so should have seen 5 blocks from callback
   BOOST_CHECK(blocks_seen == 5);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE(test_proof_ooo_sequence) try {
   tester chain(setup_policy::none);

   unsigned num_actions = 0;
   fc::sha256 computed_action_mroot;

   compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
      [&](const chain::action& act) {
         return act.account == N(interested);
      },
      [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
         computed_action_mroot = validate_compressed_merkle_proof(serialized_compressed_proof, [&](uint64_t receiver, uint64_t action) {
            BOOST_CHECK(name(receiver) == N(interested));
            num_actions++;
         });
      }
   )});

   chain.create_account(N(interested));
   chain.set_code(N(interested), inliner_wast);
   chain.set_abi(N(interested), test_account_abi);
   chain.produce_block();

   signed_transaction trx;
   trx.actions.emplace_back(action({permission_level{N(interested), N(owner)}}, N(interested), name(5ull), {}));
   chain.set_transaction_headers(trx, 10, 0);
   trx.sign(chain.get_private_key(N(interested)), chain.control->get_chain_id() );
   chain.push_transaction( trx );

   sha256 expected_mroot = chain.produce_block()->action_mroot;
   BOOST_CHECK(expected_mroot == computed_action_mroot);
   BOOST_CHECK(num_actions == 65);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE(test_proof_delayed) try {
   tester chain(setup_policy::none);
   fc::sha256 computed_action_mroot;
   bool seen_interested = false;

   compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
      [&](const chain::action& act) {
         return true;
      },
      [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
         computed_action_mroot = validate_compressed_merkle_proof(serialized_compressed_proof, [&](uint64_t receiver, uint64_t action) {
            seen_interested |= name(receiver) == N(interested);
         });
      }
   )});

   auto produce_and_check = [&]() {
      sha256 new_mroot = chain.produce_block()->action_mroot;
      BOOST_CHECK(new_mroot == computed_action_mroot);
   };

   chain.create_account(N(interested));
   chain.set_abi(N(interested), test_account_abi);
   produce_and_check();

   chain.push_action(N(interested), N(dothedew), N(interested), fc::mutable_variant_object()("nonce", 0), 100, 2);

   produce_and_check();
   produce_and_check();
   BOOST_CHECK(seen_interested == false);

   //no specific number, but enough for delayed to execute
   produce_and_check();
   produce_and_check();
   produce_and_check();
   produce_and_check();
   produce_and_check();
   BOOST_CHECK(seen_interested);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE(test_proof_deferred_soft_fail) try {
   tester chain(setup_policy::none);
   fc::sha256 computed_action_mroot;
   bool seen_interested = false;

   compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
      [&](const chain::action& act) {
         return true;
      },
      [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
         computed_action_mroot = validate_compressed_merkle_proof(serialized_compressed_proof, [&](uint64_t receiver, uint64_t action) {
            seen_interested |= name(receiver) == N(interested) && action_name(action) == N(onerror);
         });
      }
   )});

   auto produce_and_check = [&]() {
      sha256 new_mroot = chain.produce_block()->action_mroot;
      BOOST_CHECK(new_mroot == computed_action_mroot);
   };

   chain.create_account(N(interested));
   chain.set_code(N(interested), send_def_then_blowup_wast);
   chain.set_abi(N(interested), test_account_abi);
   produce_and_check();

   chain.push_action(N(interested), N(dothedew), N(interested), fc::mutable_variant_object()("nonce", 0), 100);

   produce_and_check();
   produce_and_check();
   BOOST_CHECK(seen_interested == false);

   //no specific number, but enough for the deferred trx to execute
   produce_and_check();
   produce_and_check();
   produce_and_check();
   produce_and_check();
   produce_and_check();
   BOOST_CHECK(seen_interested);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE(test_proof_multiple) try {
   tester chain(setup_policy::none);

   fc::sha256 computed_action_mroot1, computed_action_mroot2, expected_mroot;
   bool expect_banana = false, expect_spoon = false;

   compressed_proof_generator proof_generator(*chain.control, {std::make_pair(
      //first up, this callback cares about everything on the spoon account
      [&](const chain::action& act){
         return act.account == N(spoon);
      },
      [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
         BOOST_CHECK(expect_spoon);
         computed_action_mroot1 = validate_compressed_merkle_proof(serialized_compressed_proof, [](uint64_t receiver, uint64_t action) {
            BOOST_CHECK(name(receiver) == N(spoon));
         });
      }
   ),
   std::make_pair(
      //next up, this callback only cares about bananas on the spoon account
      [&](const chain::action& act){
         return act.account == N(spoon) && act.name == N(banana);
      },
      [&](auto bsp, std::vector<char>&& serialized_compressed_proof) {
         BOOST_CHECK(expect_banana);
         computed_action_mroot2 = validate_compressed_merkle_proof(serialized_compressed_proof, [](uint64_t receiver, uint64_t action) {
            BOOST_CHECK(name(receiver) == N(spoon));
         });
      }
   )});

   chain.create_account(N(spoon));
   chain.set_abi(N(spoon), multi_account_abi);
   chain.produce_block();

   unsigned nonce = 0;

   //block with an apple
   chain.push_action(N(spoon), N(apple), N(spoon), fc::mutable_variant_object()("nonce", nonce++));
   expect_spoon = true;
   BOOST_CHECK(chain.produce_block()->action_mroot == computed_action_mroot1);

   //block with a couple apples and a banana
   chain.push_action(N(spoon), N(apple), N(spoon), fc::mutable_variant_object()("nonce", nonce++));
   chain.push_action(N(spoon), N(apple), N(spoon), fc::mutable_variant_object()("nonce", nonce++));
   chain.push_action(N(spoon), N(banana), N(spoon), fc::mutable_variant_object()("nonce", nonce++));
   expect_banana = true;
   BOOST_CHECK(chain.produce_block()->action_mroot == computed_action_mroot1 && computed_action_mroot1 == computed_action_mroot2);

   //block with nothing (except onblock)
   expect_spoon = expect_banana = false;
   chain.produce_block();

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()

