/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/block_summary_object.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <proxy/proxy.wast.hpp>

using namespace eosio;
using namespace chain;


void Set_Proxy_Owner( testing_blockchain& chain, account_name proxy, account_name owner ) {
   eosio::chain::signed_transaction trx;
   trx.scope = sort_names({proxy,owner});
   transaction_emplace_message(trx, "proxy", 
                      vector<types::account_permission>({ }),
                      "setowner", owner);

   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());
   idump((trx));
   chain.push_transaction(trx);
}

// declared in slow_test.cpp
namespace slow_tests {
   void SetCode( testing_blockchain& chain, account_name account, const char* wast );
}

BOOST_AUTO_TEST_SUITE(deferred_tests)

BOOST_FIXTURE_TEST_CASE(opaque_proxy, testing_fixture)
{ try {
      Make_Blockchain(chain);
      chain.produce_blocks(1);
      Make_Account(chain, newguy);
      Make_Account(chain, proxy);
      chain.produce_blocks(1);

      slow_tests::SetCode(chain, "proxy", proxy_wast);
      //chain.produce_blocks(1);

      Set_Proxy_Owner(chain, "proxy", "newguy");
      chain.produce_blocks(7);
      
      Transfer_Asset(chain, inita, proxy, asset(100));
      chain.produce_blocks(1);
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("newguy"), asset(0));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), asset(100000-300));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("proxy"), asset(100));


      chain.produce_blocks(1);
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("newguy"), asset(100));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), asset(100000-300));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("proxy"), asset(0));
      
      BOOST_CHECK_EQUAL(chain.head_block_num(), 12);
      BOOST_CHECK(chain.fetch_block_by_number(12).valid());
      BOOST_CHECK(!chain.fetch_block_by_number(12)->cycles.empty());
      BOOST_CHECK(!chain.fetch_block_by_number(12)->cycles.front().empty());
      BOOST_CHECK_EQUAL(chain.fetch_block_by_number(12)->cycles.front().front().generated_input.size(), 1);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
