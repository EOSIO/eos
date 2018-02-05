/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/test/unit_test.hpp>

#include "../../common/database_fixture.hpp"

#include <rate_limit_auth/rate_limit_auth.wast.hpp>
#include <currency/currency.wast.hpp>

using namespace eosio;
using namespace chain;

BOOST_AUTO_TEST_SUITE(wasm_tests)

//Test rate limiting with single authority
BOOST_FIXTURE_TEST_CASE(rate_limit_single_authority_test, testing_fixture)
{ try {
      Make_Blockchain(chain);
      chain.produce_blocks(10);
      Make_Account(chain, currency);
      Make_Account(chain, test1);
      Make_Account(chain, test2);
      chain.produce_blocks(1);


      types::setcode handler;
      handler.account = "currency";

      auto wasm = testing_blockchain::assemble_wast( currency_wast );
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      {
         eosio::chain::signed_transaction txn;
         txn.scope = {"currency"};
         txn.messages.resize(1);
         txn.messages[0].code = config::eos_contract_name;
         txn.messages[0].authorization.emplace_back(types::account_permission{"currency","active"});
         transaction_set_message(txn, 0, "setcode", handler);
         txn.expiration = chain.head_block_time() + 100;
         transaction_set_reference_block(txn, chain.head_block_id());
         chain.push_transaction(txn);
         chain.produce_blocks(1);

         txn.scope = sort_names({"test1","currency"});
         txn.messages.clear();
         transaction_emplace_message(txn, "currency",
                            vector<types::account_permission>{ {"currency","active"} },
                            "transfer", types::transfer{"currency", "test1", 10000000,""});
         chain.push_transaction(txn);

         txn.scope = sort_names({"test2","currency"});
         txn.messages.clear();
         transaction_emplace_message(txn, "currency",
                            vector<types::account_permission>{ {"currency","active"} },
                            "transfer", types::transfer{"currency", "test2", 10000000,""});
         chain.push_transaction(txn);
         chain.produce_blocks(1);
      }

      eosio::chain::signed_transaction txn;
      txn.scope = sort_names({"test1","inita"});
      txn.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(txn, chain.head_block_id());

      eosio::chain::signed_transaction txn2;
      txn2.scope = sort_names({"test2","inita"});
      txn2.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(txn2, chain.head_block_id());

      // sending 1800 transaction messages for 2 authorization accounts, which is the rate limit for this second
      // since there were no previous messages before this
      uint32_t i = 0;
      for (; i < 1800; ++i)
      {
         txn.messages.clear();
         transaction_emplace_message(txn, "currency",
                            vector<types::account_permission>{ {"test1","active"} },
                            "transfer", types::transfer{"test1", "inita", 1+i,""});
         chain.push_transaction(txn);

         txn2.messages.clear();
         transaction_emplace_message(txn2, "currency",
                            vector<types::account_permission>{ {"test2","active"} },
                            "transfer", types::transfer{"test2", "inita", 1+i,""});
         chain.push_transaction(txn2);
      }

      // at rate limit for test1, so it will be rejected
      try
      {
         txn.messages.clear();
         transaction_emplace_message(txn, "currency",
                            vector<types::account_permission>{ {"test1","active"} },
                            "transfer", types::transfer{"test1", "inita", i+1,""});
         chain.push_transaction(txn);
         BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
      }
      catch (const tx_msgs_auth_exceeded& ex)
      {
      }

      // at rate limit for test2, so it will be rejected
      try
      {
         txn.messages.clear();
         transaction_emplace_message(txn2, "currency",
                            vector<types::account_permission>{ {"test2","active"} },
                            "transfer", types::transfer{"test2", "inita", i+1,""});
         chain.push_transaction(txn2);
         BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
      }
      catch (const tx_msgs_auth_exceeded& ex)
      {
      }

} FC_LOG_AND_RETHROW() }

//Test rate limiting with multiple authorities
BOOST_FIXTURE_TEST_CASE(rate_limit_multi_authority_test, testing_fixture)
{ try {
   Make_Blockchain(chain);
   chain.produce_blocks(10);
   Make_Account(chain, currency);
   Make_Account(chain, test1);
   Make_Account(chain, test2);
   Make_Account(chain, test3);
   Make_Account(chain, test4);
   Make_Account(chain, test5);
   Make_Account(chain, test11);
   Make_Account(chain, test12);
   Make_Account(chain, test13);
   Make_Account(chain, test14);
   Make_Account(chain, test15);
   Make_Account(chain, test21);
   Make_Account(chain, test22);
   Make_Account(chain, test23);
   Make_Account(chain, test24);
   Make_Account(chain, test25);
   Make_Account(chain, test31);
   Make_Account(chain, test32);
   Make_Account(chain, test33);
   Make_Account(chain, test34);
   Make_Account(chain, test35);
   Make_Account(chain, test41);
   Make_Account(chain, test42);
   Make_Account(chain, test43);
   Make_Account(chain, test44);
   Make_Account(chain, test45);
   chain.produce_blocks(1);


   types::setcode handler;
   handler.account = "currency";

   auto wasm = testing_blockchain::assemble_wast( currency_wast );
   handler.code.resize(wasm.size());
   memcpy( handler.code.data(), wasm.data(), wasm.size() );

   types::setcode handler2;
   handler2.account = "test1";

   auto wasm2 = testing_blockchain::assemble_wast( rate_limit_auth_wast );
   handler2.code.resize(wasm2.size());
   memcpy( handler2.code.data(), wasm2.data(), wasm2.size() );


   // setup currency code, and transfer tokens so accounts can do transfers
   if (false)
   {
      eosio::chain::signed_transaction txn;
      txn.scope = {"currency"};
      txn.messages.resize(1);
      txn.messages[0].code = config::eos_contract_name;
      txn.messages[0].authorization.emplace_back(types::account_permission{"currency","active"});
      transaction_set_message(txn, 0, "setcode", handler);
      txn.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(txn, chain.head_block_id());
      chain.push_transaction(txn);
      chain.produce_blocks(1);
   }


   // setup test1 code account, this will be used for most messages, to reach code account rate limit
   {
      eosio::chain::signed_transaction txn;
      txn.scope = {"test1"};
      txn.messages.resize(1);
      txn.messages[0].code = config::eos_contract_name;
      txn.messages[0].authorization.emplace_back(types::account_permission{"test1","active"});
      transaction_set_message(txn, 0, "setcode", handler2);
      txn.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(txn, chain.head_block_id());
      chain.push_transaction(txn);
      chain.produce_blocks(1);
   }


   // setup test5 code account, this will be used to verify that code account rate limit only affects individual code accounts
   {
      handler2.account = "test5";
      eosio::chain::signed_transaction txn;
      txn.scope = {"test5"};
      txn.messages.resize(1);
      txn.messages[0].code = config::eos_contract_name;
      txn.messages[0].authorization.emplace_back(types::account_permission{"test5","active"});
      transaction_set_message(txn, 0, "setcode", handler2);
      txn.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(txn, chain.head_block_id());
      chain.push_transaction(txn);
      chain.produce_blocks(1);
   }


   // setup transactions
   eosio::chain::signed_transaction txn;
   txn.scope = sort_names({"test1","test2"});
   txn.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn, chain.head_block_id());

   eosio::chain::signed_transaction txn2;
   txn2.scope = sort_names({"test2","test3"});
   txn2.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn2, chain.head_block_id());

   eosio::chain::signed_transaction txn3;
   txn3.scope = sort_names({"test1","test4"});
   txn3.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn3, chain.head_block_id());

   // looping 900 times to put some accounts (test1 and test2) at 1800 rate limit
   for (uint32_t i = 0; i < 900; ++i)
   {
      txn.messages.clear();
      transaction_emplace_message(txn, "test1",
                         vector<types::account_permission>{ {"test1","active"},{"test2","active"} },
                         "transfer", types::transfer{"test1", "test2", i+1, ""});
      chain.push_transaction(txn);

      txn2.messages.clear();
      transaction_emplace_message(txn2, "test1",
                         vector<types::account_permission>{ {"test2","active"},{"test3","active"} },
                         "transfer", types::transfer{"test2", "test3", i+1, ""});
      chain.push_transaction(txn2);

      txn3.messages.clear();
      transaction_emplace_message(txn3, "test1",
                         vector<types::account_permission>{ {"test1","active"},{"test4","active"} },
                         "transfer", types::transfer{"test1", "test4", i+1, ""});
      chain.push_transaction(txn3);

   }
   // auth test1 - 1800 transaction messages
   // auth test2 - 1800 transaction messages
   // auth test3 - 900 transaction messages
   // auth test4 - 900 transaction messages
   // code test1 - 5400 transaction messages

   // test1 at rate limit, should be rejected
   try
   {
      txn.scope = sort_names({"test1"});
      txn.messages.clear();
      transaction_emplace_message(txn, "currency",
                         vector<types::account_permission>{ {"test1","active"} },
                         "transfer", types::transfer{"test1", "test4", 1000,""});
      chain.push_transaction(txn);
      BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
   }
   catch (const tx_msgs_auth_exceeded& ex)
   {
   }


   // test2 at rate limit, should be rejected
   try
   {
      txn2.scope = sort_names({"test2"});
      txn2.messages.clear();
      transaction_emplace_message(txn2, "currency",
                         vector<types::account_permission>{ {"test2","active"} },
                         "transfer", types::transfer{"test2", "test3", 1000,""});
      chain.push_transaction(txn2);
      BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
   }
   catch (const tx_msgs_auth_exceeded& ex)
   {
   }


   eosio::chain::signed_transaction txn4;
   txn4.scope = sort_names({"test3","test4"});
   txn4.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn4, chain.head_block_id());


   // looping 900 times to put remaining accounts at 1800 rate limit
   for (uint32_t i = 0; i < 900; ++i)
   {
      txn4.messages.clear();
      transaction_emplace_message(txn4, "test1",
                         vector<types::account_permission>{ {"test3","active"},{"test4","active"} },
                         "transfer", types::transfer{"test3", "test4", i+1, ""});
      chain.push_transaction(txn4);

   }
   // auth test1 - 1800 transaction messages
   // auth test2 - 1800 transaction messages
   // auth test3 - 1800 transaction messages
   // auth test4 - 1800 transaction messages
   // code test1 - 7200 transaction messages


   // test3 at rate limit, should be rejected
   try
   {
      txn.scope = sort_names({"test3"});
      txn.messages.clear();
      transaction_emplace_message(txn, "currency",
                         vector<types::account_permission>{ {"test3","active"} },
                         "transfer", types::transfer{"test3", "test5", 1000,""});
      chain.push_transaction(txn);
      BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
   }
   catch (const tx_msgs_auth_exceeded& ex)
   {
   }


   // test4 at rate limit, should be rejected
   try
   {
      txn2.scope = sort_names({"test4"});
      txn2.messages.clear();
      transaction_emplace_message(txn2, "currency",
                         vector<types::account_permission>{ {"test4","active"} },
                         "transfer", types::transfer{"test4", "test5", 1000,""});
      chain.push_transaction(txn2);
      BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
   }
   catch (const tx_msgs_auth_exceeded& ex)
   {
   }


   eosio::chain::signed_transaction txn11;
   txn11.scope = sort_names({"test11","test31"});
   txn11.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn11, chain.head_block_id());

   eosio::chain::signed_transaction txn12;
   txn12.scope = sort_names({"test12","test32"});
   txn12.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn12, chain.head_block_id());

   eosio::chain::signed_transaction txn13;
   txn13.scope = sort_names({"test13","test33"});
   txn13.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn13, chain.head_block_id());

   eosio::chain::signed_transaction txn14;
   txn14.scope = sort_names({"test14","test34"});
   txn14.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn14, chain.head_block_id());

   eosio::chain::signed_transaction txn15;
   txn15.scope = sort_names({"test15","test35"});
   txn15.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn15, chain.head_block_id());

   eosio::chain::signed_transaction txn21;
   txn21.scope = sort_names({"test21","test41"});
   txn21.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn21, chain.head_block_id());

   eosio::chain::signed_transaction txn22;
   txn22.scope = sort_names({"test22","test42"});
   txn22.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn22, chain.head_block_id());

   eosio::chain::signed_transaction txn23;
   txn23.scope = sort_names({"test23","test43"});
   txn23.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn23, chain.head_block_id());

   eosio::chain::signed_transaction txn24;
   txn24.scope = sort_names({"test24","test44"});
   txn24.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn24, chain.head_block_id());

   eosio::chain::signed_transaction txn25;
   txn25.scope = sort_names({"test25","test45"});
   txn25.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(txn25, chain.head_block_id());

   // looping 1440 times with 10 different transactions to get to the 18000 code account rate limit for test1
   uint32_t i = 0;
   for (; i < 1440; ++i)
   {
      txn11.messages.clear();
      transaction_emplace_message(txn11, "test1",
                         vector<types::account_permission>{ {"test11","active"},{"test31","active"} },
                         "transfer", types::transfer{"test11", "test31", i+1, ""});
      chain.push_transaction(txn11);

      txn12.messages.clear();
      transaction_emplace_message(txn12, "test1",
                         vector<types::account_permission>{ {"test12","active"},{"test32","active"} },
                         "transfer", types::transfer{"test12", "test32", i+1, ""});
      chain.push_transaction(txn12);

      txn13.messages.clear();
      transaction_emplace_message(txn13, "test1",
                         vector<types::account_permission>{ {"test13","active"},{"test33","active"} },
                         "transfer", types::transfer{"test13", "test33", i+1, ""});
      chain.push_transaction(txn13);

      txn14.messages.clear();
      transaction_emplace_message(txn14, "test1",
                         vector<types::account_permission>{ {"test14","active"},{"test34","active"} },
                         "transfer", types::transfer{"test14", "test34", i+1, ""});
      chain.push_transaction(txn14);

      txn15.messages.clear();
      transaction_emplace_message(txn15, "test1",
                         vector<types::account_permission>{ {"test15","active"},{"test35","active"} },
                         "transfer", types::transfer{"test15", "test35", i+1, ""});
      chain.push_transaction(txn15);

      txn21.messages.clear();
      transaction_emplace_message(txn21, "test1",
                         vector<types::account_permission>{ {"test21","active"},{"test41","active"} },
                         "transfer", types::transfer{"test21", "test41", i+1, ""});
      chain.push_transaction(txn21);

      txn22.messages.clear();
      transaction_emplace_message(txn22, "test1",
                         vector<types::account_permission>{ {"test22","active"},{"test42","active"} },
                         "transfer", types::transfer{"test22", "test42", i+1, ""});
      chain.push_transaction(txn22);

      txn23.messages.clear();
      transaction_emplace_message(txn23, "test1",
                         vector<types::account_permission>{ {"test23","active"},{"test43","active"} },
                         "transfer", types::transfer{"test23", "test43", i+1, ""});
      chain.push_transaction(txn23);

      txn24.messages.clear();
      transaction_emplace_message(txn24, "test1",
                         vector<types::account_permission>{ {"test24","active"},{"test44","active"} },
                         "transfer", types::transfer{"test24", "test44", i+1, ""});
      chain.push_transaction(txn24);

      txn25.messages.clear();
      transaction_emplace_message(txn25, "test1",
                         vector<types::account_permission>{ {"test25","active"},{"test45","active"} },
                         "transfer", types::transfer{"test25", "test45", i+1, ""});
      chain.push_transaction(txn25);

   }
   // code test1 - 18000 transaction messages


   // reached rate limit, should be rejected
   try
   {
      txn11.messages.clear();
      transaction_emplace_message(txn11, "test1",
                         vector<types::account_permission>{ {"test11","active"},{"test21","active"} },
                         "transfer", types::transfer{"test11", "test21", i+1, ""});
      chain.push_transaction(txn11);
      BOOST_FAIL("Should have gotten tx_msgs_code_exceeded exception.");
   }
   catch (const tx_msgs_code_exceeded& ex)
   {
   }

   // verify that only test1 is blocked
   txn11.messages.clear();
   transaction_emplace_message(txn11, "test5",
                      vector<types::account_permission>{ {"test11","active"},{"test31","active"} },
                      "transfer", types::transfer{"test11", "test31", i+1, ""});
   chain.push_transaction(txn11);


   // still should be rejected
   try
   {
      txn11.messages.clear();
      transaction_emplace_message(txn11, "test1",
                         vector<types::account_permission>{ {"test11","active"},{"test21","active"} },
                         "transfer", types::transfer{"test11", "test21", i+1, ""});
      chain.push_transaction(txn11);
      BOOST_FAIL("Should have gotten tx_msgs_code_exceeded exception.");
   }
   catch (const tx_msgs_code_exceeded& ex)
   {
   }

   chain.produce_blocks(1);

   // rate limit calculation will now be at least at a new second, so will always be able to handle a new transaction message
   txn11.messages.clear();
   transaction_emplace_message(txn11, "test1",
                      vector<types::account_permission>{ {"test11","active"},{"test21","active"} },
                      "transfer", types::transfer{"test11", "test21", i+1, ""});
   chain.push_transaction(txn11);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
