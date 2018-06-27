/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>

using namespace eosio;
using namespace testing;
using namespace chain;

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE(block_with_invalid_tx_test)
{
   tester main;

   // First we create a valid block with valid transaction
   main.create_account(N(newacc));
   auto b = main.produce_block();

   // Make a copy of the valid block and corrupt the transaction
   auto copy_b = std::make_shared<signed_block>(*b);
   auto signed_tx = copy_b->transactions.back().trx.get<packed_transaction>().get_signed_transaction();
   auto& act = signed_tx.actions.back();
   auto act_data = act.data_as<newaccount>();
   // Make the transaction invalid by having the new account name the same as the creator name
   act_data.name = act_data.creator;
   act.data = fc::raw::pack(act_data);
   // Re-sign the transaction
   signed_tx.signatures.clear();
   signed_tx.sign(main.get_private_key(N(eosio), "active"), main.control->get_chain_id());
   // Replace the valid transaction with the invalid transaction 
   auto invalid_packed_tx = packed_transaction(signed_tx);
   copy_b->transactions.back().trx = invalid_packed_tx;

   // Re-sign the block
   auto header_bmroot = digest_type::hash( std::make_pair( copy_b->digest(), main.control->head_block_state()->blockroot_merkle.get_root() ) );
   auto sig_digest = digest_type::hash( std::make_pair(header_bmroot, main.control->head_block_state()->pending_schedule_hash) );
   copy_b->producer_signature = main.get_private_key(N(eosio), "active").sign(sig_digest);

   // Push block with invalid transaction to other chain
   tester validator;
   validator.control->abort_block();
   BOOST_REQUIRE_EXCEPTION(validator.control->push_block( copy_b ), fc::exception ,
   [] (const fc::exception &e)->bool {
      return e.code() == action_validate_exception::code_value ;
   }) ;
  
}

BOOST_AUTO_TEST_SUITE_END()
