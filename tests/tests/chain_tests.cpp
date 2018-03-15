/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain/types.hpp>
#include <currency/currency.abi.hpp>
#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(chain_tests)

// Test transaction signature chain_controller::get_required_keys
BOOST_AUTO_TEST_CASE(get_required_keys)
{
    try
    {
        tester test;
        signed_transaction trx;
        action transfer_act;
        transfer_act.account = N(currency);
        transfer_act.name = N(transfer);
        transfer_act.authorization = vector<permission_level>{{N(inita), config::active_name}};

        abi_serializer abi_ser(json::from_string(currency_abi).as<abi_def>());
        transfer_act.data = abi_ser.variant_to_binary("transfer", mutable_variant_object()
                ("from", "inita")
                ("to",   "initb")
                ("quantity", "100.0000 CUR")
                ("memo", "inita to initb")
        );

        trx.actions.emplace_back(std::move(transfer_act));
        // trx.expiration = fc::time_point_sec(test.control->head_block_time().sec_since_epoch()) + 100;
        test.set_tapos(trx);
        //BOOST_REQUIRE_THROW(test.push_transaction(trx), tx_missing_sigs);

        trx.sign(test.get_private_key(N(currency), "active"), chain_id_type());

        test.push_transaction(trx);
        test.produce_block();

        auto inita_bal = test.get_currency_balance(N(currency), symbol(SY(4,CUR)), N(inita));
        auto initb_bal = test.get_currency_balance(N(currency), symbol(SY(4,CUR)), N(initb));
        BOOST_CHECK_EQUAL(inita_bal, asset(100000 - 100));
        BOOST_CHECK_EQUAL(initb_bal, asset(100000 + 100));
        BOOST_CHECK(trx.get_signature_keys(chain_id_type()).size() > 0);
} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
