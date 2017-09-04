#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/permission_link_object.hpp>
#include <eos/chain/authority_checker.hpp>

#include <eos/native_contract/producer_objects.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/permutation.hpp>

#include "../common/database_fixture.hpp"

using namespace eos;
using namespace chain;


BOOST_AUTO_TEST_SUITE(chain_tests)

// Test transaction signature chain_controller::get_required_keys
BOOST_FIXTURE_TEST_CASE(get_required_keys, testing_fixture)
{ try {
      Make_Blockchain(chain)

      chain.set_auto_sign_transactions(false);
      chain.set_skip_transaction_signature_checking(false);

      SignedTransaction trx;
      trx.messages.resize(1);
      transaction_set_reference_block(trx, chain.head_block_id());
      trx.expiration = chain.head_block_time() + 100;
      trx.scope = sort_names( {"inita", "initb"} );
      types::transfer trans = { "inita", "initb", (100), "" };

      trx.messages[0].type = "transfer";
      trx.messages[0].authorization = {{"inita", "active"}};
      trx.messages[0].code = config::EosContractName;
      transaction_set_message(trx, 0, "transfer", trans);
      BOOST_REQUIRE_THROW(chain.push_transaction(trx), tx_missing_sigs);

      auto required_keys = chain.get_required_keys(trx, available_keys());
      BOOST_CHECK(required_keys.size() < available_keys().size()); // otherwise not a very good test
      chain.sign_transaction(trx); // uses get_required_keys
      chain.push_transaction(trx);

      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000 - 100));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("initb"), Asset(100000 + 100));

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
