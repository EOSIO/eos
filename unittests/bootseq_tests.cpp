#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>

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

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

struct genesis_account {
   account_name aname;
   uint64_t     initial_balance;
};

std::vector<genesis_account> test_genesis( {
  {N(b1),    100'000'000'0000ll},
  {N(whale4), 40'000'000'0000ll},
  {N(whale3), 30'000'000'0000ll},
  {N(whale2), 20'000'000'0000ll},
  {N(p1),      1'000'000'0000ll},
  {N(p2),      1'000'000'0000ll},
  {N(p3),      1'000'000'0000ll},
  {N(p4),      1'000'000'0000ll},
  {N(p5),      1'000'000'0000ll},
  {N(minow1),        100'0000ll},
  {N(minow2),          1'0000ll},
  {N(minow3),          1'0000ll},
  {N(masses),800'000'000'0000ll}
});

class bootseq_tester : public TESTER
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
        const auto &accnt = control->db().get<account_object, by_name>(config::system_account_name);
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


    void create_currency( name contract, name manager, asset maxsupply, const private_key_type* signer = nullptr ) {
        auto act =  mutable_variant_object()
                ("issuer",       manager )
                ("maximum_supply", maxsupply )
                ("can_freeze", 0)
                ("can_recall", 0)
                ("can_whitelist", 0);

        base_tester::push_action(contract, N(create), contract, act );
    }

    void issue( name contract, name manager, name to, asset amount ) {
       base_tester::push_action( contract, N(issue), manager, mutable_variant_object()
                ("to",      to )
                ("quantity", amount )
                ("memo", "")
        );
    }


/*
    action_result stake(const account_name& from, const account_name& to, const string& net, const string& cpu, const string& ) {
        return push_action( name(from), name(from), N(delegatebw), mvo()
                ("from",     from)
                ("receiver", to)
                ("stake_net_quantity", net)
                ("stake_cpu_quantity", cpu)
                ("stake_storage_quantity", storage)
                ("transfer", storage)
        );
    }
    */
#if _READY
    fc::variant get_total_stake( const account_name& act )
    {
        vector<char> data = get_row_by_account( config::system_account_name, act, N(totalband), act );
        return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "total_resources", data );
    }
#endif

    /*
    action_result stake( const account_name& acnt, const string& net, const string& cpu, const string& storage ) {
        return stake( acnt, acnt, net, cpu, storage );
    }
    */

    asset get_balance( const account_name& act )
    {
         return get_currency_balance(N(eosio.token), symbol(SY(4,EOS)), act);
    }

    action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
        return push_action( acnt, acnt, N(regproducer), mvo()
                ("producer",  name(acnt).to_string() )
                ("producer_key", fc::raw::pack( get_public_key( acnt, "active" ) ) )
                ("prefs", producer_parameters_example( params_fixture ) )
        );
    }

    void set_code_abi(const account_name& account, const char* wast, const char* abi, const private_key_type* signer = nullptr)
    {
       wdump((account));
        set_code(account, wast, signer);
        set_abi(account, abi, signer);
        produce_blocks();
    }


    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(bootseq_tests)

BOOST_FIXTURE_TEST_CASE( bootseq_test, bootseq_tester ) {
    try {
        //set_code_abi(config::system_account_name, eosio_bios_wast, eosio_bios_abi);

        // Create the following accounts:
        //  eosio.msig
        //  eosio.token
        create_accounts({N(eosio.msig), N(eosio.token)});
        /*
        auto eosio_active = authority( 1, {}, {{{N(eosio),N(active)},1}} );
        auto eosio_active_pk = get_private_key( N(eosio), "active" );

        base_tester::push_action(  N(eosio), N(newaccount), vector<permission_level>{{N(eosio),config::active_name}},
                                   fc::variant(newaccount{
                                      .creator  = N(eosio),
                                      .name     = N(eosio.msig),
                                      .owner    = eosio_active,
                                      .active   = eosio_active,
                                      .recovery = eosio_active 
                                   }).get_object() );

        base_tester::push_action(  N(eosio), N(newaccount), vector<permission_level>{{N(eosio),config::active_name}},
                                   fc::variant(newaccount{
                                      .creator  = N(eosio),
                                      .name     = N(eosio.token),
                                      .owner    = eosio_active,
                                      .active   = eosio_active,
                                      .recovery = eosio_active 
                                   }).get_object() );
                                   */

        // Set code for the following accounts:
        //  eosio.system  (code: eosio.bios)
        //  eosio.msig (code: eosio.msig)
        //  eosio.token    (code: eosio.token)
        set_code_abi(N(eosio.msig), eosio_msig_wast, eosio_msig_abi);//, &eosio_active_pk);
        set_code_abi(N(eosio.token), eosio_token_wast, eosio_token_abi); //, &eosio_active_pk);

        ilog(".");
        // Set privileges for eosio.msig
        auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                              config::system_account_name,  mutable_variant_object()
                ("account", "eosio.msig")
                ("is_priv", 1)
        );

        ilog(".");
        // Todo : how to check the privilege is set? (use is_priv action)


        auto max_supply = asset::from_string("10000000000.0000 EOS"); /// 1x larger than 1B initial tokens
        auto initial_supply = asset::from_string("1000000000.0000 EOS"); /// 1x larger than 1B initial tokens

        // Create EOS tokens in eosio.token, set its manager as eosio.system
        create_currency(N(eosio.token), config::system_account_name, max_supply );


        // Issue the genesis supply of 1 billion EOS tokens to eosio.system
        // Issue the genesis supply of 1 billion EOS tokens to eosio.system
        issue(N(eosio.token), config::system_account_name, config::system_account_name, initial_supply);


        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

        // Create a few genesis accounts
        //std::vector<account_name> gen_accounts{N(inita), N(initb), N(initc)};
        //create_accounts(gen_accounts);

        for( const auto& a : test_genesis ) {
           create_account( a.aname, N(eosio) );
           /*
           base_tester::push_action(N(eosio.token), N(transfer), config::system_account_name, mutable_variant_object()
                    ("from", name(config::system_account_name))
                    ("to", name(a.aname))
                    ("quantity", asset(a.initial_balance))
                    ("memo", "" ) );
                    */
        }
        set_code_abi(N(eosio), eosio_system_wast, eosio_system_abi); //, &eosio_active_pk);

        for( const auto& a : test_genesis ) {
           auto ib = a.initial_balance;
           auto ram = 1000;
           auto net = (ib - ram) / 2;
           auto cpu = ib - net - ram;

           auto r =  base_tester::push_action(N(eosio), N(buyram), N(eosio), mutable_variant_object()
                    ("payer", "eosio")
                    ("receiver", name(a.aname))
                    ("quant", asset(ram)) 
                    );
           BOOST_REQUIRE( !r->except_ptr );

           r = base_tester::push_action(N(eosio), N(delegatebw), N(eosio), mutable_variant_object()
                    ("from", "eosio" )
                    ("receiver", name(a.aname))
                    ("stake_net_quantity", asset(net)) 
                    ("stake_cpu_quantity", asset(cpu)) 
                    ("transfer", 1) 
                    );

           BOOST_REQUIRE( !r->except_ptr );
        }

        produce_blocks(10000);

        for( auto pro : { N(p1), N(p2), N(p3), N(p4), N(p5) } ) {
           base_tester::push_action(N(eosio), N(regproducer), pro, mutable_variant_object()
                       ("producer",  name(pro))
                       ("producer_key", get_public_key( pro, "active" ) )
                       ("url", "" )
                    );
        }
        produce_blocks(10);

        auto votepro = [&]( account_name voter, vector<account_name> producers ) {
          std::sort( producers.begin(), producers.end() );
          base_tester::push_action(N(eosio), N(voteproducer), voter, mvo()
                                ("voter",  name(voter))
                                ("proxy", name(0) )
                                ("producers", producers) 
                     );
        };

        produce_blocks(100);

        votepro( N(b1), {N(p1), N(p2)} );
        votepro( N(whale2), {N(p2), N(p3)} );
        votepro( N(whale3), {N(p2), N(p4), N(p5)} );

        wlog("pb" );
        produce_blocks(10);
        wdump((control->head_block_state()->active_schedule));
        return;
        produce_blocks(7000); /// produce blocks until virutal bandwidth can acomadate a small user
        wlog("minow" );
        votepro( N(minow1), {N(p1), N(p2)} );



        // Transfer EOS to genesis accounts
        /*for (auto gen_acc : gen_accounts) {
            auto quantity = "10000.0000 EOS";
            auto stake_quantity = "5000.0000 EOS";

            ilog(".");
            auto trace = base_tester::push_action(N(eosio.token), N(transfer), config::system_account_name, mutable_variant_object()
                    ("from", name(config::system_account_name))
                    ("to", gen_acc)
                    ("quantity", quantity)
                    ("memo", gen_acc)
            );
            ilog( "." );

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
        */
        ilog(".");

#warning Complete this test
/*
        // Set code eosio.system from eosio.bios to eosio.system
        set_code_abi(config::system_account_name, eosio_system_wast, eosio_system_abi);


        ilog(".");
        // Register these genesis accts as producer account
        for (auto gen_acc : gen_accounts) {
        //    BOOST_REQUIRE_EQUAL(success(), regproducer(gen_acc));
        }
*/
    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
