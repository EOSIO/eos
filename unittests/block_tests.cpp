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
   auto copy_b = std::make_shared<signed_block>(std::move(*b));
   auto signed_tx = signed_transaction(copy_b->transactions.back().trx.get<packed_transaction>().to_packed_transaction_v0()->get_signed_transaction());
   auto &act = signed_tx.actions.back();
   auto act_data = act.data_as<newaccount>();
   // Make the transaction invalid by having the new account name the same as the creator name
   act_data.name = act_data.creator;
   act.data = fc::raw::pack(act_data);
   // Re-sign the transaction
   signed_tx.signatures.clear();
   signed_tx.sign(main.get_private_key(config::system_account_name, "active"), main.control->get_chain_id());
   // Replace the valid transaction with the invalid transaction
   auto invalid_packed_tx = packed_transaction(std::move(signed_tx), true);
   copy_b->transactions.back().trx = invalid_packed_tx;

   // Re-calculate the transaction merkle
   deque<digest_type> trx_digests;
   const auto &trxs = copy_b->transactions;
   for (const auto &a : trxs)
      trx_digests.emplace_back(a.digest());
   copy_b->transaction_mroot = merkle(move(trx_digests));

   // Re-sign the block
   auto header_bmroot = digest_type::hash(std::make_pair(copy_b->digest(), main.control->head_block_state()->blockroot_merkle.get_root()));
   auto sig_digest = digest_type::hash(std::make_pair(header_bmroot, main.control->head_block_state()->pending_schedule.schedule_hash));
   copy_b->producer_signature = main.get_private_key(config::system_account_name, "active").sign(sig_digest);

   // Push block with invalid transaction to other chain
   tester validator;
   auto bs = validator.control->create_block_state_future(copy_b->calculate_id(), copy_b);
   validator.control->abort_block();
   BOOST_REQUIRE_EXCEPTION(validator.control->push_block(bs, forked_branch_callback{}, trx_meta_cache_lookup{}), fc::exception,
                           [](const fc::exception &e) -> bool {
                              return e.code() == account_name_exists_exception::code_value;
                           });
}

std::pair<signed_block_ptr, signed_block_ptr> corrupt_trx_in_block(validating_tester& main, account_name act_name) {
   // First we create a valid block with valid transaction
   main.create_account(act_name);
   signed_block_ptr b = main.produce_block_no_validation();

   // Make a copy of the valid block and corrupt the transaction
   auto copy_b = std::make_shared<signed_block>(b->clone());
   const auto& packed_trx = copy_b->transactions.back().trx.get<packed_transaction>();
   auto signed_tx = packed_trx.get_signed_transaction();
   // Corrupt one signature
   signed_tx.signatures.clear();
   signed_tx.sign(main.get_private_key(act_name, "active"), main.control->get_chain_id());

   // Replace the valid transaction with the invalid transaction
   auto invalid_packed_tx = packed_transaction(signed_tx, packed_trx.get_compression());
   copy_b->transactions.back().trx = invalid_packed_tx;

   // Re-calculate the transaction merkle
   deque<digest_type> trx_digests;
   const auto& trxs = copy_b->transactions;
   for( const auto& a : trxs )
      trx_digests.emplace_back( a.digest() );
   copy_b->transaction_mroot = merkle( move(trx_digests) );

   // Re-sign the block
   auto header_bmroot = digest_type::hash( std::make_pair( copy_b->digest(), main.control->head_block_state()->blockroot_merkle.get_root() ) );
   auto sig_digest = digest_type::hash( std::make_pair(header_bmroot, main.control->head_block_state()->pending_schedule.schedule_hash) );
   copy_b->producer_signature = main.get_private_key(b->producer, "active").sign(sig_digest);
   return std::pair<signed_block_ptr, signed_block_ptr>(b, copy_b);
}

// verify that a block with a transaction with an incorrect signature, is blindly accepted from a trusted producer
BOOST_AUTO_TEST_CASE(trusted_producer_test)
{
   flat_set<account_name> trusted_producers = { N(defproducera), N(defproducerc) };
   validating_tester main(trusted_producers);
   // only using validating_tester to keep the 2 chains in sync, not to validate that the validating_node matches the main node,
   // since it won't be
   main.skip_validate = true;

   // First we create a valid block with valid transaction
   std::set<account_name> producers = { N(defproducera), N(defproducerb), N(defproducerc), N(defproducerd) };
   for (auto prod : producers)
       main.create_account(prod);
   auto b = main.produce_block();

   std::vector<account_name> schedule(producers.cbegin(), producers.cend());
   auto trace = main.set_producers(schedule);

   while (b->producer != N(defproducera)) {
      b = main.produce_block();
   }

   auto blocks = corrupt_trx_in_block(main, N(tstproducera));
   main.validate_push_block( blocks.second );
}

// like trusted_producer_test, except verify that any entry in the trusted_producer list is accepted
BOOST_AUTO_TEST_CASE(trusted_producer_verify_2nd_test)
{
   flat_set<account_name> trusted_producers = { N(defproducera), N(defproducerc) };
   validating_tester main(trusted_producers);
   // only using validating_tester to keep the 2 chains in sync, not to validate that the validating_node matches the main node,
   // since it won't be
   main.skip_validate = true;

   // First we create a valid block with valid transaction
   std::set<account_name> producers = { N(defproducera), N(defproducerb), N(defproducerc), N(defproducerd) };
   for (auto prod : producers)
       main.create_account(prod);
   auto b = main.produce_block();

   std::vector<account_name> schedule(producers.cbegin(), producers.cend());
   auto trace = main.set_producers(schedule);

   while (b->producer != N(defproducerc)) {
      b = main.produce_block();
   }

   auto blocks = corrupt_trx_in_block(main, N(tstproducera));
   main.validate_push_block( blocks.second );
}

// verify that a block with a transaction with an incorrect signature, is rejected if it is not from a trusted producer
BOOST_AUTO_TEST_CASE(untrusted_producer_test)
{
   flat_set<account_name> trusted_producers = { N(defproducera), N(defproducerc) };
   validating_tester main(trusted_producers);
   // only using validating_tester to keep the 2 chains in sync, not to validate that the validating_node matches the main node,
   // since it won't be
   main.skip_validate = true;

   // First we create a valid block with valid transaction
   std::set<account_name> producers = { N(defproducera), N(defproducerb), N(defproducerc), N(defproducerd) };
   for (auto prod : producers)
       main.create_account(prod);
   auto b = main.produce_block();

   std::vector<account_name> schedule(producers.cbegin(), producers.cend());
   auto trace = main.set_producers(schedule);

   while (b->producer != N(defproducerb)) {
      b = main.produce_block();
   }

   auto blocks = corrupt_trx_in_block(main, N(tstproducera));
   BOOST_REQUIRE_EXCEPTION(main.validate_push_block( blocks.second ), fc::exception ,
   [] (const fc::exception &e)->bool {
      return e.code() == unsatisfied_authorization::code_value ;
   }) ;
}

/**
 * Ensure that the block broadcasted by producing node and receiving node is identical
 */
BOOST_AUTO_TEST_CASE(broadcasted_block_test)
{

  tester producer_node;
  tester receiving_node;

  signed_block_ptr bcasted_blk_by_prod_node;
  signed_block_ptr bcasted_blk_by_recv_node;

  producer_node.control->accepted_block.connect( [&](const block_state_ptr& bs) {
    bcasted_blk_by_prod_node = bs->block;
  });
  receiving_node.control->accepted_block.connect( [&](const block_state_ptr& bs) {
    bcasted_blk_by_recv_node = bs->block;
  });

  auto b = producer_node.produce_block();
  receiving_node.push_block(b);

  bytes bcasted_blk_by_prod_node_packed = fc::raw::pack(*bcasted_blk_by_prod_node);
  bytes bcasted_blk_by_recv_node_packed = fc::raw::pack(*bcasted_blk_by_recv_node);
  BOOST_CHECK(std::equal(bcasted_blk_by_prod_node_packed.begin(), bcasted_blk_by_prod_node_packed.end(), bcasted_blk_by_recv_node_packed.begin()));
}

/**
 * Verify abort block returns applied transactions in block
 */
BOOST_FIXTURE_TEST_CASE( abort_block_transactions, validating_tester) { try {

      produce_blocks(2);
      signed_transaction trx;

      account_name a = N(newco);
      account_name creator = config::system_account_name;

      // account does not exist before test
      BOOST_REQUIRE_EXCEPTION(control->get_account( a ), fc::exception,
                              [a] (const fc::exception& e)->bool {
                                 return std::string( e.what() ).find( a.to_string() ) != std::string::npos;
                              }) ;

      auto owner_auth = authority( get_public_key( a, "owner" ) );
      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                      .creator  = creator,
                                      .name     = a,
                                      .owner    = owner_auth,
                                      .active   = authority( get_public_key( a, "active" ) )
                                });
      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      auto trace = push_transaction( trx );

      control->get_account( a ); // throws if it does not exist

      auto unapplied_trxs = control->abort_block();

      // verify transaction returned from abort_block()
      BOOST_REQUIRE_EQUAL( 1,  unapplied_trxs.size() );
      BOOST_REQUIRE_EQUAL( trx.id(), unapplied_trxs.at(0)->id() );

      // account does not exist block was aborted which had transaction
      BOOST_REQUIRE_EXCEPTION(control->get_account( a ), fc::exception,
                              [a] (const fc::exception& e)->bool {
                                 return std::string( e.what() ).find( a.to_string() ) != std::string::npos;
                              }) ;

      produce_blocks(1);

   } FC_LOG_AND_RETHROW() }

/**
 * Verify abort block returns applied transactions in block
 */
BOOST_FIXTURE_TEST_CASE( abort_block_transactions_tester, validating_tester) { try {

      produce_blocks(2);
      signed_transaction trx;

      account_name a = N(newco);
      account_name creator = config::system_account_name;

      // account does not exist before test
      BOOST_REQUIRE_EXCEPTION(control->get_account( a ), fc::exception,
                              [a] (const fc::exception& e)->bool {
                                 return std::string( e.what() ).find( a.to_string() ) != std::string::npos;
                              }) ;

      auto owner_auth = authority( get_public_key( a, "owner" ) );
      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                      .creator  = creator,
                                      .name     = a,
                                      .owner    = owner_auth,
                                      .active   = authority( get_public_key( a, "active" ) )
                                });
      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      auto trace = push_transaction( trx );

      control->get_account( a ); // throws if it does not exist

      produce_block( fc::milliseconds(config::block_interval_ms*2) ); // aborts block, tester should reapply trx

      control->get_account( a ); // throws if it does not exist

      auto unapplied_trxs = control->abort_block(); // should be empty now

      BOOST_REQUIRE_EQUAL( 0,  unapplied_trxs.size() );

   } FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
