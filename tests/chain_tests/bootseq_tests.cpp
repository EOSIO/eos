#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>
// These contracts are still under dev
#if _READY
#include <eosio.bios/eosio.bios.wast.hpp>
#include <eosio.bios/eosio.bios.abi,hpp>
#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>
#endif

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

class bootseq_tester : public tester
{
public:

    static fc::variant_object producer_parameters_example( int n ) {
        return mutable_variant_object()
                ("target_block_size", 1024 * 1024 + n)
                ("max_block_size", 10 * 1024 + n)
                ("target_block_acts_per_scope", 1000 + n)
                ("max_block_acts_per_scope", 10000 + n)
                ("target_block_acts", 1100 + n)
                ("max_block_acts", 11000 + n)
                ("max_storage_size", 2000 + n)
                ("max_transaction_lifetime", 3600 + n)
                ("max_transaction_exec_time", 9900 + n)
                ("max_authority_depth", 6 + n)
                ("max_inline_depth", 4 + n)
                ("max_inline_action_size", 4096 + n)
                ("max_generated_transaction_size", 64*1024 + n)
                ("percent_of_max_inflation_rate", 50 + n)
                ("storage_reserve_ratio", 100 + n);
    }

    bootseq_tester() {
        const auto &accnt = control->get_database().get<account_object, by_name>(config::system_account_name);
        abi_def abi;
        BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
        abi_ser.set_abi(abi);
    }

    action_result push_action( const account_name& account, const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
        string action_type_name = abi_ser.get_action_type(name);

        action act;
        act.account = account;
        act.name = name;
        act.data = abi_ser.variant_to_binary( action_type_name, data );

        return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : 0 );
    }


    void create_currency( name contract, name manager, asset maxsupply ) {
        push_action(contract, manager, N(create), mutable_variant_object()
                ("issuer",       manager )
                ("maximum_supply", maxsupply )
                ("can_freeze", 0)
                ("can_recall", 0)
                ("can_whitelist", 0)
        );
    }

    void issue( name contract, name manager, name to, asset amount ) {
        push_action( contract, manager, N(issue), mutable_variant_object()
                ("to",      to )
                ("quantity", amount )
                ("memo", "")
        );
    }



    action_result stake(const account_name& from, const account_name& to, const string& net, const string& cpu, const string& storage ) {
        return push_action( name(from), name(from), N(delegatebw), mvo()
                ("from",     from)
                ("receiver", to)
                ("stake_net", net)
                ("stake_cpu", cpu)
                ("stake_storage", storage)
        );
    }
#if _READY
    fc::variant get_total_stake( const account_name& act )
    {
        vector<char> data = get_row_by_account( config::system_account_name, act, N(totalband), act );
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "total_resources", data );
    }
#endif

    action_result stake( const account_name& acnt, const string& net, const string& cpu, const string& storage ) {
        return stake( acnt, acnt, net, cpu, storage );
    }

    asset get_balance( const account_name& act )
    {
        //return get_currency_balance( config::system_account_name, symbol(SY(4,EOS)), act );
        //temporary code. current get_currency_balancy uses table name N(accounts) from currency.h
        //generic_currency table name is N(account).
        const auto& db  = control->get_database();
        const auto* tbl = db.find<contracts::table_id_object, contracts::by_code_scope_table>(boost::make_tuple(config::system_account_name, act, N(account)));
        share_type result = 0;

        // the balance is implied to be 0 if either the table or row does not exist
        if (tbl) {
            const auto *obj = db.find<contracts::key_value_object, contracts::by_scope_primary>(boost::make_tuple(tbl->id, symbol(SY(4,EOS)).value()));
            if (obj) {
                //balance is the second field after symbol, so skip the symbol
                fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
                fc::raw::unpack(ds, result);
            }
        }
        return asset( result, symbol(SY(4,EOS)) );
    }

    action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
        return push_action( acnt, acnt, N(regproducer), mvo()
                ("producer",  name(acnt).to_string() )
                ("producer_key", fc::raw::pack( get_public_key( acnt, "active" ) ) )
                ("prefs", producer_parameters_example( params_fixture ) )
        );
    }

    void set_code_abi(const account_name& account, const char* wast, const char* abi)
    {
        set_code(account, wast);
        set_abi(account, abi);
        produce_blocks();
    }


    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(bootseq_tests)

BOOST_FIXTURE_TEST_CASE( bootseq_test, bootseq_tester ) {
    try {

        // Create the following accounts:
        //  eosio.msig
        //  eosio.token
        create_accounts({N(eosio.msig), N(eosio.token)});

        // Set code for the following accounts:
        //  eosio.system  (code: eosio.bios)
        //  eosio.msig (code: eosio.msig)
        //  eosio.token    (code: eosio.token)
// These contracts are still under dev
#if _READY
        set_code_abi(config::system_account_name, eosio_bios_wast, eosio_bios_abi);
        set_code_abi(N(eosio.msig), eosio_msig_wast, eosio_msig_abi);
        set_code_abi(N(eosio.token), eosio_token_wast, eosio_token_abi);
#endif

        // Set privileges for eosio.msig
        auto trace = push_action(N(eosio.bios), N(eosio.bios), N(setprivil), mutable_variant_object()
                ("account", "eosio.token")
                ("is_priv", 1)
        );
        // Todo : how to check the privilege is set? (use is_priv action)


        auto expected = asset::from_string("1000000000.0000 EOS");
        // Create EOS tokens in eosio.token, set its manager as eosio.system
        create_currency(N(eosio.token), config::system_account_name, expected);
        // Issue the genesis supply of 1 billion EOS tokens to eosio.system
        // Issue the genesis supply of 1 billion EOS tokens to eosio.system
        issue(N(eosio.token), config::system_account_name, config::system_account_name, expected); 
        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(expected, actual);

        // Create a few genesis accounts
        std::vector<account_name> gen_accounts{N(inita), N(initb), N(initc)};
        create_accounts(gen_accounts);
        // Transfer EOS to genesis accounts
        for (auto gen_acc : gen_accounts) {
            auto quantity = "10000.0000 EOS";
            auto stake_quantity = "5000.0000 EOS";

            auto trace = push_action(N(eosio.token), config::system_account_name, N(transfer), mutable_variant_object()
                    ("from", config::system_account_name)
                    ("to", gen_acc)
                    ("quantity", quantity)
                    ("memo", gen_acc)
            );

            auto balance = get_balance(gen_acc);
            BOOST_REQUIRE_EQUAL(asset::from_string(quantity), balance);
#if _READY
            // Stake 50% of balance to CPU and other 50% to bandwidth
            BOOST_REQUIRE_EQUAL(success(),
                                stake(config::system_account_name, gen_acc, stake_quantity, stake_quantity, ""));
            auto total = get_total_stake(gen_acc);
            BOOST_REQUIRE_EQUAL(asset::from_string(stake_quantity).amount, total["net_weight"].as_uint64());
            BOOST_REQUIRE_EQUAL(asset::from_string(stake_quantity).amount, total["cpu_weight"].as_uint64());
#endif
        }

        // Set code eosio.system from eosio.bios to eosio.system
        set_code_abi(config::system_account_name, eosio_system_wast, eosio_system_abi);

        // Register these genesis accts as producer account
        for (auto gen_acc : gen_accounts) {
            BOOST_REQUIRE_EQUAL(success(), regproducer(gen_acc));
        }

    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
