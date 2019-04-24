#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/pbft_database.hpp>
#include <eosio/chain/global_property_object.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio::chain;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE(pbft_tests)

    BOOST_AUTO_TEST_CASE(can_init) {
        tester tester;
        controller &ctrl = *tester.control.get();
        pbft_controller pbft_ctrl{ctrl};

        tester.produce_block();
        auto p = pbft_ctrl.pbft_db.should_prepared();
        BOOST_CHECK(!p);
    }

    BOOST_AUTO_TEST_CASE(can_advance_lib_in_old_version) {
        tester tester;
        controller &ctrl = *tester.control.get();
        pbft_controller pbft_ctrl{ctrl};

        auto privkey = tester::get_private_key( N(eosio), "active" );
        auto pubkey = tester::get_public_key( N(eosio), "active");
        auto sp = [privkey]( const eosio::chain::digest_type& digest ) {
            return privkey.sign(digest);
        };
        std::map<eosio::chain::public_key_type, signature_provider_type> msp;
        msp[pubkey]=sp;
        ctrl.set_my_signature_providers(msp);

        tester.produce_block();//produce block num 2
        BOOST_REQUIRE_EQUAL(ctrl.last_irreversible_block_num(), 0);
        BOOST_REQUIRE_EQUAL(ctrl.head_block_num(), 2);
        tester.produce_block();
        BOOST_REQUIRE_EQUAL(ctrl.last_irreversible_block_num(), 2);
        BOOST_REQUIRE_EQUAL(ctrl.head_block_num(), 3);
    }

BOOST_AUTO_TEST_CASE(can_advance_lib_after_upgrade) {
    tester tester;
    controller &ctrl = *tester.control.get();
    pbft_controller pbft_ctrl{ctrl};
    ctrl.set_upo(150);

    const auto& upo = ctrl.db().get<upgrade_property_object>();
    const auto upo_upgrade_target_block_num = upo.upgrade_target_block_num;
    BOOST_CHECK_EQUAL(upo_upgrade_target_block_num, 150);


    auto privkey = tester::get_private_key( N(eosio), "active" );
    auto pubkey = tester::get_public_key( N(eosio), "active");
    auto sp = [privkey]( const eosio::chain::digest_type& digest ) {
      return privkey.sign(digest);
    };
    std::map<eosio::chain::public_key_type, signature_provider_type> msp;
    msp[pubkey]=sp;
    ctrl.set_my_signature_providers(msp);

    auto is_upgraded = ctrl.is_upgraded();

    BOOST_CHECK_EQUAL(is_upgraded, false);

    tester.produce_block();//produce block num 2
    BOOST_CHECK_EQUAL(ctrl.last_irreversible_block_num(), 0);
    BOOST_CHECK_EQUAL(ctrl.head_block_num(), 2);
    tester.produce_blocks(150);
    BOOST_CHECK_EQUAL(ctrl.last_irreversible_block_num(), 150);
    BOOST_CHECK_EQUAL(ctrl.head_block_num(), 152);

    is_upgraded = ctrl.is_upgraded();
    BOOST_CHECK_EQUAL(is_upgraded, true);

    tester.produce_blocks(10);
    BOOST_CHECK_EQUAL(ctrl.pending_pbft_lib(), false);
    BOOST_CHECK_EQUAL(ctrl.last_irreversible_block_num(), 150);
    BOOST_CHECK_EQUAL(ctrl.head_block_num(), 162);

    pbft_ctrl.maybe_pbft_prepare();
    pbft_ctrl.maybe_pbft_commit();

    BOOST_CHECK_EQUAL(ctrl.pending_pbft_lib(), true);
    tester.produce_block(); //set lib using pending pbft lib

    BOOST_CHECK_EQUAL(ctrl.last_irreversible_block_num(), 162);
    BOOST_CHECK_EQUAL(ctrl.head_block_num(), 163);
}


BOOST_AUTO_TEST_SUITE_END()
