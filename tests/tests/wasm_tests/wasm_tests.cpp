/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/test/unit_test.hpp>

#include "../../common/database_fixture.hpp"

#include <rate_limit_auth/rate_limit_auth.wast.hpp>
#include <currency/currency.wast.hpp>

using namespace eos;
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
         eos::chain::SignedTransaction trx;
         trx.scope = {"currency"};
         trx.messages.resize(1);
         trx.messages[0].code = config::EosContractName;
         trx.messages[0].authorization.emplace_back(types::AccountPermission{"currency","active"});
         transaction_set_message(trx, 0, "setcode", handler);
         trx.expiration = chain.head_block_time() + 100;
         transaction_set_reference_block(trx, chain.head_block_id());
         chain.push_transaction(trx);
         chain.produce_blocks(1);

         trx.scope = sort_names({"test1","currency"});
         trx.messages.clear();
         transaction_emplace_message(trx, "currency",
                            vector<types::AccountPermission>{ {"currency","active"} },
                            "transfer", types::transfer{"currency", "test1", 10000000,""});
         chain.push_transaction(trx);

         trx.scope = sort_names({"test2","currency"});
         trx.messages.clear();
         transaction_emplace_message(trx, "currency",
                            vector<types::AccountPermission>{ {"currency","active"} },
                            "transfer", types::transfer{"currency", "test2", 10000000,""});
         chain.push_transaction(trx);
         chain.produce_blocks(1);
      }

      eos::chain::SignedTransaction trx;
      trx.scope = sort_names({"test1","inita"});
      trx.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(trx, chain.head_block_id());

      eos::chain::SignedTransaction trx2;
      trx2.scope = sort_names({"test2","inita"});
      trx2.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(trx2, chain.head_block_id());

      for (uint32_t i = 0; i < 1800; ++i)
      {
         trx.messages.clear();
         transaction_emplace_message(trx, "currency",
                            vector<types::AccountPermission>{ {"test1","active"} },
                            "transfer", types::transfer{"test1", "inita", 1+i,""});
         chain.push_transaction(trx);

         trx2.messages.clear();
         transaction_emplace_message(trx2, "currency",
                            vector<types::AccountPermission>{ {"test2","active"} },
                            "transfer", types::transfer{"test2", "inita", 1+i,""});
         chain.push_transaction(trx2);
      }

      try
      {
         transaction_emplace_message(trx, "currency",
                            vector<types::AccountPermission>{ {"inita","active"} },
                            "transfer", types::transfer{"inita", "test1", 10,""});
         chain.push_transaction(trx);
         BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
      }
      catch (const tx_msgs_auth_exceeded& ex)
      {
      }

      try
      {
         transaction_emplace_message(trx2, "currency",
                            vector<types::AccountPermission>{ {"inita","active"} },
                            "transfer", types::transfer{"inita", "test2", 10,""});
         chain.push_transaction(trx2);
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

   {
      eos::chain::SignedTransaction trx;
      trx.scope = {"currency"};
      trx.messages.resize(1);
      trx.messages[0].code = config::EosContractName;
      trx.messages[0].authorization.emplace_back(types::AccountPermission{"currency","active"});
      transaction_set_message(trx, 0, "setcode", handler);
      trx.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(trx, chain.head_block_id());
      chain.push_transaction(trx);
      chain.produce_blocks(1);

      trx.scope = sort_names({"test1","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test1", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test2","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test2", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test3","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test3", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test5","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test5", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test11","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test11", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test12","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test12", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test13","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test13", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test14","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test14", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test15","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test15", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test21","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test21", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test22","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test22", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test23","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test23", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test24","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test24", 10000000,""});
      chain.push_transaction(trx);

      trx.scope = sort_names({"test25","currency"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"currency","active"} },
                         "transfer", types::transfer{"currency", "test25", 10000000,""});
      chain.push_transaction(trx);

      chain.produce_blocks(1);
   }

   {
      eos::chain::SignedTransaction trx;
      trx.scope = {"test1"};
      trx.messages.resize(1);
      trx.messages[0].code = config::EosContractName;
      trx.messages[0].authorization.emplace_back(types::AccountPermission{"test1","active"});
      transaction_set_message(trx, 0, "setcode", handler2);
      trx.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(trx, chain.head_block_id());
      chain.push_transaction(trx);
      chain.produce_blocks(1);
   }

   {
      handler2.account = "test5";
      eos::chain::SignedTransaction trx;
      trx.scope = {"test5"};
      trx.messages.resize(1);
      trx.messages[0].code = config::EosContractName;
      trx.messages[0].authorization.emplace_back(types::AccountPermission{"test5","active"});
      transaction_set_message(trx, 0, "setcode", handler2);
      trx.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(trx, chain.head_block_id());
      chain.push_transaction(trx);
      chain.produce_blocks(1);
   }

   eos::chain::SignedTransaction trx;
   trx.scope = sort_names({"test1","test2"});
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());

   eos::chain::SignedTransaction trx2;
   trx2.scope = sort_names({"test2","test3"});
   trx2.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx2, chain.head_block_id());

   eos::chain::SignedTransaction trx3;
   trx3.scope = sort_names({"test1","test4"});
   trx3.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx3, chain.head_block_id());

   for (uint32_t i = 0; i < 900; ++i)
   {
      trx.messages.clear();
      transaction_emplace_message(trx, "test1",
                         vector<types::AccountPermission>{ {"test1","active"},{"test2","active"} },
                         "transfer", types::transfer{"test1", "test2", i+1, ""});
      chain.push_transaction(trx);

      trx2.messages.clear();
      transaction_emplace_message(trx2, "test1",
                         vector<types::AccountPermission>{ {"test2","active"},{"test3","active"} },
                         "transfer", types::transfer{"test2", "test3", i+1, ""});
      chain.push_transaction(trx2);

      trx3.messages.clear();
      transaction_emplace_message(trx3, "test1",
                         vector<types::AccountPermission>{ {"test1","active"},{"test4","active"} },
                         "transfer", types::transfer{"test1", "test4", i+1, ""});
      chain.push_transaction(trx3);

   }
   // auth test1 - 1800 transaction messages
   // auth test2 - 1800 transaction messages
   // auth test3 - 900 transaction messages
   // auth test4 - 900 transaction messages
   // code test1 - 5400 transaction messages

   try
   {
      trx.scope = sort_names({"test1"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"test1","active"} },
                         "transfer", types::transfer{"test1", "test4", 1000,""});
      chain.push_transaction(trx);
      BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
   }
   catch (const tx_msgs_auth_exceeded& ex)
   {
   }

   try
   {
      trx2.scope = sort_names({"test2"});
      trx2.messages.clear();
      transaction_emplace_message(trx2, "currency",
                         vector<types::AccountPermission>{ {"test2","active"} },
                         "transfer", types::transfer{"test2", "test3", 1000,""});
      chain.push_transaction(trx2);
      BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
   }
   catch (const tx_msgs_auth_exceeded& ex)
   {
   }

   eos::chain::SignedTransaction trx4;
   trx4.scope = sort_names({"test3","test4"});
   trx4.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx4, chain.head_block_id());

   for (uint32_t i = 0; i < 900; ++i)
   {
      trx4.messages.clear();
      transaction_emplace_message(trx4, "test1",
                         vector<types::AccountPermission>{ {"test3","active"},{"test4","active"} },
                         "transfer", types::transfer{"test3", "test4", i+1, ""});
      chain.push_transaction(trx4);

   }
   // auth test1 - 1800 transaction messages
   // auth test2 - 1800 transaction messages
   // auth test3 - 1800 transaction messages
   // auth test4 - 1800 transaction messages
   // code test1 - 7200 transaction messages

   try
   {
      trx.scope = sort_names({"test1"});
      trx.messages.clear();
      transaction_emplace_message(trx, "currency",
                         vector<types::AccountPermission>{ {"test3","active"} },
                         "transfer", types::transfer{"test3", "test5", 1000,""});
      chain.push_transaction(trx);
      BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
   }
   catch (const tx_msgs_auth_exceeded& ex)
   {
   }

   try
   {
      trx2.scope = sort_names({"test4"});
      trx2.messages.clear();
      transaction_emplace_message(trx2, "currency",
                         vector<types::AccountPermission>{ {"test4","active"} },
                         "transfer", types::transfer{"test4", "test5", 1000,""});
      chain.push_transaction(trx2);
      BOOST_FAIL("Should have gotten tx_msgs_auth_exceeded exception.");
   }
   catch (const tx_msgs_auth_exceeded& ex)
   {
   }

   eos::chain::SignedTransaction trx11;
   trx11.scope = sort_names({"test11","test31"});
   trx11.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx11, chain.head_block_id());

   eos::chain::SignedTransaction trx12;
   trx12.scope = sort_names({"test12","test32"});
   trx12.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx12, chain.head_block_id());

   eos::chain::SignedTransaction trx13;
   trx13.scope = sort_names({"test13","test33"});
   trx13.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx13, chain.head_block_id());

   eos::chain::SignedTransaction trx14;
   trx14.scope = sort_names({"test14","test34"});
   trx14.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx14, chain.head_block_id());

   eos::chain::SignedTransaction trx15;
   trx15.scope = sort_names({"test15","test35"});
   trx15.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx15, chain.head_block_id());

   eos::chain::SignedTransaction trx21;
   trx21.scope = sort_names({"test21","test41"});
   trx21.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx21, chain.head_block_id());

   eos::chain::SignedTransaction trx22;
   trx22.scope = sort_names({"test22","test42"});
   trx22.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx22, chain.head_block_id());

   eos::chain::SignedTransaction trx23;
   trx23.scope = sort_names({"test23","test43"});
   trx23.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx23, chain.head_block_id());

   eos::chain::SignedTransaction trx24;
   trx24.scope = sort_names({"test24","test44"});
   trx24.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx24, chain.head_block_id());

   eos::chain::SignedTransaction trx25;
   trx25.scope = sort_names({"test25","test45"});
   trx25.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx25, chain.head_block_id());

   uint32_t i = 0;
   for (; i < 1440; ++i)
   {
      trx11.messages.clear();
      transaction_emplace_message(trx11, "test1",
                         vector<types::AccountPermission>{ {"test11","active"},{"test31","active"} },
                         "transfer", types::transfer{"test11", "test31", i+1, ""});
      chain.push_transaction(trx11);

      trx12.messages.clear();
      transaction_emplace_message(trx12, "test1",
                         vector<types::AccountPermission>{ {"test12","active"},{"test32","active"} },
                         "transfer", types::transfer{"test12", "test32", i+1, ""});
      chain.push_transaction(trx12);

      trx13.messages.clear();
      transaction_emplace_message(trx13, "test1",
                         vector<types::AccountPermission>{ {"test13","active"},{"test33","active"} },
                         "transfer", types::transfer{"test13", "test33", i+1, ""});
      chain.push_transaction(trx13);

      trx14.messages.clear();
      transaction_emplace_message(trx14, "test1",
                         vector<types::AccountPermission>{ {"test14","active"},{"test34","active"} },
                         "transfer", types::transfer{"test14", "test34", i+1, ""});
      chain.push_transaction(trx14);

      trx15.messages.clear();
      transaction_emplace_message(trx15, "test1",
                         vector<types::AccountPermission>{ {"test15","active"},{"test35","active"} },
                         "transfer", types::transfer{"test15", "test35", i+1, ""});
      chain.push_transaction(trx15);

      trx21.messages.clear();
      transaction_emplace_message(trx21, "test1",
                         vector<types::AccountPermission>{ {"test21","active"},{"test41","active"} },
                         "transfer", types::transfer{"test21", "test41", i+1, ""});
      chain.push_transaction(trx21);

      trx22.messages.clear();
      transaction_emplace_message(trx22, "test1",
                         vector<types::AccountPermission>{ {"test22","active"},{"test42","active"} },
                         "transfer", types::transfer{"test22", "test42", i+1, ""});
      chain.push_transaction(trx22);

      trx23.messages.clear();
      transaction_emplace_message(trx23, "test1",
                         vector<types::AccountPermission>{ {"test23","active"},{"test43","active"} },
                         "transfer", types::transfer{"test23", "test43", i+1, ""});
      chain.push_transaction(trx23);

      trx24.messages.clear();
      transaction_emplace_message(trx24, "test1",
                         vector<types::AccountPermission>{ {"test24","active"},{"test44","active"} },
                         "transfer", types::transfer{"test24", "test44", i+1, ""});
      chain.push_transaction(trx24);

      trx25.messages.clear();
      transaction_emplace_message(trx25, "test1",
                         vector<types::AccountPermission>{ {"test25","active"},{"test45","active"} },
                         "transfer", types::transfer{"test25", "test45", i+1, ""});
      chain.push_transaction(trx25);

   }
   // code test1 - 18000 transaction messages

   // reached rate limit
   try
   {
      trx11.messages.clear();
      transaction_emplace_message(trx11, "test1",
                         vector<types::AccountPermission>{ {"test11","active"},{"test21","active"} },
                         "transfer", types::transfer{"test11", "test21", i+1, ""});
      chain.push_transaction(trx11);
      BOOST_FAIL("Should have gotten tx_msgs_code_exceeded exception.");
   }
   catch (const tx_msgs_code_exceeded& ex)
   {
   }

   // verify that only test1 is blocked
   trx11.messages.clear();
   transaction_emplace_message(trx11, "test5",
                      vector<types::AccountPermission>{ {"test11","active"},{"test31","active"} },
                      "transfer", types::transfer{"test11", "test31", i+1, ""});
   chain.push_transaction(trx11);

   try
   {
      trx11.messages.clear();
      transaction_emplace_message(trx11, "test1",
                         vector<types::AccountPermission>{ {"test11","active"},{"test21","active"} },
                         "transfer", types::transfer{"test11", "test21", i+1, ""});
      chain.push_transaction(trx11);
      BOOST_FAIL("Should have gotten tx_msgs_code_exceeded exception.");
   }
   catch (const tx_msgs_code_exceeded& ex)
   {
   }

   chain.produce_blocks(1);
   trx11.messages.clear();
   transaction_emplace_message(trx11, "test1",
                      vector<types::AccountPermission>{ {"test11","active"},{"test21","active"} },
                      "transfer", types::transfer{"test11", "test21", i+1, ""});
   chain.push_transaction(trx11);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
