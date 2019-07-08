#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>
// These contracts are still under dev
#include <eosio.bios/eosio.bios.wast.hpp>
#include <eosio.bios/eosio.bios.abi.hpp>
#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>
#include <eosio.msig/eosio.msig.wast.hpp>
#include <eosio.msig/eosio.msig.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <cyberway/chain/cyberway_contract_types.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::config;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

class system_contract_tester : public TESTER {
public:

   fc::variant get_global_state() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer_max_time );

   }

    auto buyram( name payer, name receiver, asset ram ) {
       auto r = base_tester::push_action(config::system_account_name, N(buyram), payer, mvo()
                    ("payer", payer)
                    ("receiver", receiver)
                    ("quant", ram)
                    );
       produce_block();
       return r;
    }

    auto delegate_bandwidth( name from, name receiver, asset net, asset cpu, uint8_t transfer = 1) {
       auto r = base_tester::push_action(config::system_account_name, N(delegatebw), from, mvo()
                    ("from", from )
                    ("receiver", receiver)
                    ("stake_net_quantity", net)
                    ("stake_cpu_quantity", cpu)
                    ("transfer", transfer)
                    );
       produce_block();
       return r;
    }

    void create_currency( name contract, name manager, asset maxsupply, const private_key_type* signer = nullptr ) {
        auto act =  mutable_variant_object()
                ("issuer",       manager )
                ("maximum_supply", maxsupply );

        base_tester::push_action(contract, N(create), contract, act );
    }

    auto issue( name contract, name manager, name to, asset amount ) {
       auto r = base_tester::push_action( contract, N(issue), manager, mutable_variant_object()
                ("to",      to )
                ("quantity", amount )
                ("memo", "")
        );
        produce_block();
        return r;
    }

    asset get_balance( const account_name& act ) {
         return get_currency_balance(token_account_name, symbol(CORE_SYMBOL), act);
    }

    void set_code_abi(const account_name& account, const char* wast, const char* abi, const private_key_type* signer = nullptr) {
       wdump((account));
        set_code(account, wast, signer);
        set_abi(account, abi, signer);
        if (account == config::system_account_name) {
           const auto& accnt = control->chaindb().get<account_object>( account );
           abi_def abi_definition;
           BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
           abi_ser.set_abi(abi_definition, abi_serializer_max_time);
        }
        produce_blocks();
    }

    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(providebw_tests)

BOOST_FIXTURE_TEST_CASE( providebw_test, system_contract_tester ) {
    try {
        // Create eosio.msig and eosio.token
        create_accounts({token_account_name, ram_account_name, ramfee_account_name,
            vpay_account_name, bpay_account_name, saving_account_name});

        // Set code for the following accounts:
        //  - eosio (code: eosio.bios) (already set by tester constructor)
        //  - eosio.msig (code: eosio.msig)
        //  - eosio.token (code: eosio.token)
        set_code_abi(msig_account_name, eosio_msig_wast, eosio_msig_abi);//, &eosio_active_pk);
        set_code_abi(token_account_name, eosio_token_wast, eosio_token_abi); //, &eosio_active_pk);

        // Set privileged for eosio.msig and eosio.token
      //   set_privileged(msig_account_name);
      //   set_privileged(token_account_name);

        // Verify eosio.msig and eosio.token is privileged
        const auto& eosio_msig_acc = get<account_object>(msig_account_name);
        BOOST_TEST(eosio_msig_acc.privileged == true);
        const auto& eosio_token_acc = get<account_object>(token_account_name);
      //   BOOST_TEST(eosio_token_acc.privileged == true);


        // Create SYS tokens in eosio.token, set its manager as eosio
        auto max_supply = core_from_string("10000000000.0000"); /// 1x larger than 1B initial tokens
        auto initial_supply = core_from_string("1000000000.0000"); /// 1x larger than 1B initial tokens
        create_currency(token_account_name, system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to eosio.system
        issue(token_account_name, system_account_name, system_account_name, initial_supply);

        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

        create_accounts({N(provider), N(user), N(user2)});

        // Set eosio.system to eosio
        set_code_abi(config::system_account_name, eosio_system_wast, eosio_system_abi);

        {
            auto r = buyram(config::system_account_name, N(provider), asset(1000));
            BOOST_REQUIRE( !r->except_ptr );

            r = delegate_bandwidth(stake_account_name, N(provider), asset(1000000), asset(100000));
            BOOST_REQUIRE( !r->except_ptr );

            r = buyram(config::system_account_name, N(user), asset(1000));
            BOOST_REQUIRE( !r->except_ptr );

            r = buyram(config::system_account_name, N(user2), asset(1000));
        }
        
        auto block_time = control->head_block_time().time_since_epoch().to_seconds();
        auto& rlm = control->get_mutable_resource_limits_manager();
        auto provider_stake = rlm.get_account_balance(control->head_block_time(), N(provider), rlm.get_pricelist(), false);
        auto provider_used = rlm.get_account_usage(N(provider));

        BOOST_CHECK_GT(provider_stake, 0);
        
        auto user_stake = rlm.get_account_balance(control->head_block_time(), N(user), rlm.get_pricelist(), false);
        auto user_used = rlm.get_account_usage(N(user));

        BOOST_CHECK_EQUAL(user_stake, 0);

        BOOST_CHECK_EQUAL(rlm.get_account_balance(control->head_block_time(), N(user2), rlm.get_pricelist(), false), 0);

        // Check that user can't send transaction due missing bandwidth
        signed_transaction trx;
        trx.actions.push_back(get_action(
               name(config::system_account_name), N(reqauth),
               vector<permission_level>{{N(user),name(config::active_name)}},
               mvo()("from","user")));
        set_transaction_headers(trx);
        trx.sign( get_private_key("user", "active"), control->get_chain_id() );
        BOOST_REQUIRE_EXCEPTION( push_transaction(trx), tx_usage_exceeded, [](auto&){return true;});

        trx.actions.emplace_back( vector<permission_level>{{"provider", config::active_name}},
                                  cyberway::chain::providebw(N(provider), N(user)));
        set_transaction_headers(trx);

        // Check that user can publish transaction using provider bandwidth
        BOOST_TEST_MESSAGE("user public key: " <<  get_public_key("user", "active"));
        BOOST_TEST_MESSAGE("provider public key: " <<  get_public_key("provider", "active"));
        trx.signatures.clear();
        trx.sign( get_private_key("user", "active"), control->get_chain_id() );
        trx.sign( get_private_key("provider", "active"), control->get_chain_id() );
        auto r = push_transaction( trx );

        BOOST_REQUIRE( !r->except_ptr );

        auto provider_used2 = rlm.get_account_usage(N(provider));
        auto user_used2 = rlm.get_account_usage(N(user));
        BOOST_CHECK_EQUAL(user_used2[resource_limits::CPU], user_used[resource_limits::CPU]);
        BOOST_CHECK_EQUAL(user_used2[resource_limits::NET], user_used[resource_limits::NET]);
        
        BOOST_CHECK_GT(provider_used2[resource_limits::CPU], provider_used[resource_limits::CPU]);
        BOOST_CHECK_GT(provider_used2[resource_limits::NET], provider_used[resource_limits::NET]);
        
        // Check that another actor need bandwidth to publish transaction
        trx.actions.push_back(get_action(name(config::system_account_name), N(reqauth),
               vector<permission_level>{{N(user2),name(config::active_name)}},
               mvo()("from","user2")));
        set_transaction_headers(trx);
        trx.signatures.clear();
        trx.sign( get_private_key("user", "active"), control->get_chain_id() );
        trx.sign( get_private_key("user2", "active"), control->get_chain_id() );
        trx.sign( get_private_key("provider", "active"), control->get_chain_id() );
        BOOST_REQUIRE_EXCEPTION(push_transaction( trx ), tx_usage_exceeded, [](auto&){return true;});

        // Check operation success with 2 providebw
        trx.actions.emplace_back( vector<permission_level>{{"provider", config::active_name}},
                                  cyberway::chain::providebw(N(provider), N(user2)));
        set_transaction_headers(trx);
        trx.signatures.clear();
        trx.sign( get_private_key("user", "active"), control->get_chain_id() );
        trx.sign( get_private_key("user2", "active"), control->get_chain_id() );
        trx.sign( get_private_key("provider", "active"), control->get_chain_id() );
        r = push_transaction( trx );
        BOOST_REQUIRE( !r->except_ptr );

    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
