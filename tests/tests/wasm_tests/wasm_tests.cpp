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
         BOOST_FAIL("Should have gotten tx_msgs_exceeded exception.");
      }
      catch (const tx_msgs_exceeded& ex)
      {
      }

      try
      {
         transaction_emplace_message(trx2, "currency",
                            vector<types::AccountPermission>{ {"inita","active"} },
                            "transfer", types::transfer{"inita", "test2", 10,""});
         chain.push_transaction(trx2);
         BOOST_FAIL("Should have gotten tx_msgs_exceeded exception.");
      }
      catch (const tx_msgs_exceeded& ex)
      {
      }

} FC_LOG_AND_RETHROW() }

/*
//Test rate limiting with multiple authorities
BOOST_FIXTURE_TEST_CASE(rate_limit_multi_authority_test, testing_fixture)
{
   struct Auths2
   {
      AccountName       acct1;
      AccountName       acct2;
   };

   struct Auths3
   {
      AccountName       acct1;
      AccountName       acct2;
      AccountName       acct3;
   };

   const char* struct_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name": "Auths2",
          "fields": {
            "acct1": "UInt64",
            "acct2": "UInt64"
          }
       },{
          "name": "Auths23",
          "fields": {
            "acct1": "UInt64",
            "acct2": "UInt64",
            "acct3": "UInt64"
          }
       }],
       "actions": [],
       "tables": []
   }
   )=====";

   Make_Blockchain(chain);
   chain.produce_blocks(10);
   Make_Account(chain, currency);
   Make_Account(chain, test1);
   Make_Account(chain, test2);
   Make_Account(chain, test3);
   Make_Account(chain, test4);
   chain.produce_blocks(1);


   types::setcode handler;
   handler.account = "currency";

   auto wasm = testing_blockchain::assemble_wast( currency_wast );
   handler.code.resize(wasm.size());
   memcpy( handler.code.data(), wasm.data(), wasm.size() );

   types::setcode handler2;
   handler2.account = "test1";
   handler2.abi = fc::json::from_string(struct_abi).as<types::Abi>();

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
                         "transfer", types::transfer{"currency", "test2", 10000000,""});
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


   eos::chain::SignedTransaction trx;
   trx.scope = sort_names({"test1","test2"});
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());

   eos::chain::SignedTransaction trx2;
   trx2.scope = sort_names({"test1","test2","test3"});
   trx2.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx2, chain.head_block_id());

   eos::chain::SignedTransaction trx3;
   trx3.scope = sort_names({"test3","test4"});
   trx3.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx3, chain.head_block_id());

   for (uint32_t i = 0; i < 1; ++i)
   {
      trx.messages.clear();
      transaction_emplace_message(trx, "test1",
                         vector<types::AccountPermission>{ {"test1","active"},{"test2","active"} },
                         "auths2", Auths2{"test1", "test2"});
      chain.push_transaction(trx);

      trx2.messages.clear();
      transaction_emplace_message(trx2, "currency",
                         vector<types::AccountPermission>{ {"test1","active"},{"test2","active"},{"test3","active"} },
                         "auths3", Auths3{"test1", "test2", "test3"});
      chain.push_transaction(trx2);
   }
}
*/

BOOST_AUTO_TEST_SUITE_END()
