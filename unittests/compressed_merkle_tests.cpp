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

static auto wire_signals_to_generator(compressed_proof_generator& generator, controller& ctrl) {
   std::vector<boost::signals2::scoped_connection> conns;
   conns.emplace_back(ctrl.irreversible_block.connect([&](const block_state_ptr& bsp) {
      generator.on_irreversible_block(bsp, ctrl);
   }));
   conns.emplace_back(ctrl.applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
      generator.on_applied_transaction(std::get<0>(t));
   }));
   conns.emplace_back(ctrl.accepted_block.connect([&](const block_state_ptr& p) {
      generator.on_accepted_block(p);
   }));
   return conns;
}

BOOST_AUTO_TEST_SUITE(test_compressed_proof)

BOOST_AUTO_TEST_CASE(test_proof_no_actions) try {
   tester chain(setup_policy::none);
   //I want to very precisely control the exact actions in the block; disable onblock
   chain.set_code(N(eosio), fail_everything_wast);
   chain.produce_block();

   compressed_proof_generator proof_generator;
   auto signal_connections = wire_signals_to_generator(proof_generator, *chain.control);

   proof_generator.add_result_callback([&](const auto&){return true;}, [&](auto&& x) {
      //no actions means no callback should ever be fired
      BOOST_FAIL("Callback should not be called");
   });

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

   compressed_proof_generator proof_generator;
   auto signal_connections = wire_signals_to_generator(proof_generator, *chain.control);

   bool expecting_callback = false;
   fc::sha256 computed_action_mroot;

   proof_generator.add_result_callback(
      [&](const chain::action& act){
         return act.account == N(interested);
      },
      [&](std::vector<char>&& serialized_compressed_proof) {
         BOOST_CHECK(expecting_callback);
         computed_action_mroot = validate_compressed_merkle_proof(serialized_compressed_proof, [](uint64_t receiver) {
            BOOST_CHECK(name(receiver) == N(interested));
         });
      }
   );

   unsigned nonce = 0;
   for(unsigned action_mask = 0; action_mask < 1<<n_actions; action_mask++) {
      for(unsigned action_bit = 0; action_bit < n_actions; ++action_bit) {
         bool push_interested = action_mask&1<<action_bit;

         if(push_interested)
            chain.push_action(N(interested), N(dothedew), N(interested), fc::mutable_variant_object()("nonce", nonce++));
         else
            chain.push_action(N(nointerested), N(dothedew), N(nointerested), fc::mutable_variant_object()("nonce", nonce++));
      }

      expecting_callback = false;
      fc::sha256 expected_action_mroot = chain.produce_block()->action_mroot;
      expecting_callback = action_mask; //when action_mask is 0, no interested action pushed meaning no callback called
      chain.produce_block();

      if(expecting_callback)
         BOOST_CHECK(expected_action_mroot == computed_action_mroot);
   }

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE(test_proof_arv_activation) try {
   tester chain(setup_policy::none);
   compressed_proof_generator proof_generator;
   auto signal_connections = wire_signals_to_generator(proof_generator, *chain.control);

   fc::sha256 computed_action_mroot;

   proof_generator.add_result_callback(
      [&](const chain::action& act){
         return true;
      },
      [&](std::vector<char>&& serialized_compressed_proof) {
         computed_action_mroot = validate_compressed_merkle_proof(serialized_compressed_proof, [](uint64_t receiver) {});
      }
   );

   std::optional<sha256> reversible_action_mroot;
   auto produce_and_check = [&]() {
      sha256 new_mroot = chain.produce_block()->action_mroot;
      if(reversible_action_mroot)
         BOOST_CHECK(*reversible_action_mroot == computed_action_mroot);
      reversible_action_mroot = new_mroot;
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

BOOST_AUTO_TEST_CASE(test_proof_presist_reversible) try {
   tester chain(setup_policy::none);
   fc::temp_file tempfile;

   unsigned blocks_seen = 0;

   {
      compressed_proof_generator proof_generator(tempfile.path());
      auto signal_connections = wire_signals_to_generator(proof_generator, *chain.control);
      proof_generator.add_result_callback(
         [&](const chain::action& act){
            return true;
         },
         [&](std::vector<char>&& serialized_compressed_proof) {
            blocks_seen++;
         }
      );

      chain.produce_blocks(5); //make 4 irreversible blocks; calling result callback 4 times
   }

   {
      compressed_proof_generator proof_generator(tempfile.path());
      auto signal_connections = wire_signals_to_generator(proof_generator, *chain.control);
      proof_generator.add_result_callback(
         [&](const chain::action& act){
            return true;
         },
         [&](std::vector<char>&& serialized_compressed_proof) {
            blocks_seen++;
         }
      );
      chain.produce_block(); //makes one more irreversible block
   }

   //so should have seen 5 blocks from callback
   BOOST_CHECK(blocks_seen == 5);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()

