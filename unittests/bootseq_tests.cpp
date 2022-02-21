#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

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
  {"b1"_n,       100'000'000'0000ll},
  {"whale4"_n,    40'000'000'0000ll},
  {"whale3"_n,    30'000'000'0000ll},
  {"whale2"_n,    20'000'000'0000ll},
  {"proda"_n,      1'000'000'0000ll},
  {"prodb"_n,      1'000'000'0000ll},
  {"prodc"_n,      1'000'000'0000ll},
  {"prodd"_n,      1'000'000'0000ll},
  {"prode"_n,      1'000'000'0000ll},
  {"prodf"_n,      1'000'000'0000ll},
  {"prodg"_n,      1'000'000'0000ll},
  {"prodh"_n,      1'000'000'0000ll},
  {"prodi"_n,      1'000'000'0000ll},
  {"prodj"_n,      1'000'000'0000ll},
  {"prodk"_n,      1'000'000'0000ll},
  {"prodl"_n,      1'000'000'0000ll},
  {"prodm"_n,      1'000'000'0000ll},
  {"prodn"_n,      1'000'000'0000ll},
  {"prodo"_n,      1'000'000'0000ll},
  {"prodp"_n,      1'000'000'0000ll},
  {"prodq"_n,      1'000'000'0000ll},
  {"prodr"_n,      1'000'000'0000ll},
  {"prods"_n,      1'000'000'0000ll},
  {"prodt"_n,      1'000'000'0000ll},
  {"produ"_n,      1'000'000'0000ll},
  {"runnerup1"_n,  1'000'000'0000ll},
  {"runnerup2"_n,  1'000'000'0000ll},
  {"runnerup3"_n,  1'000'000'0000ll},
  {"minow1"_n,           100'0000ll},
  {"minow2"_n,             1'0000ll},
  {"minow3"_n,             1'0000ll},
  {"masses"_n,   800'000'000'0000ll}
});

class bootseq_tester : public TESTER {
public:
   void deploy_contract( bool call_init = true ) {
      set_code( config::system_account_name, contracts::eosio_system_wasm() );
      set_abi( config::system_account_name, contracts::eosio_system_abi().data() );
      if( call_init ) {
         base_tester::push_action(config::system_account_name, "init"_n,
                                  config::system_account_name,  mutable_variant_object()
                                  ("version", 0)
                                  ("core", CORE_SYM_STR)
            );
      }
      const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer::create_yield_function( abi_serializer_max_time ));
   }

   fc::variant get_global_state() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, "global"_n, "global"_n );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer::create_yield_function( abi_serializer_max_time ) );
   }

    auto buyram( name payer, name receiver, asset ram ) {
       auto r = base_tester::push_action(config::system_account_name, "buyram"_n, payer, mvo()
                    ("payer", payer)
                    ("receiver", receiver)
                    ("quant", ram)
                    );
       produce_block();
       return r;
    }

    auto delegate_bandwidth( name from, name receiver, asset net, asset cpu, uint8_t transfer = 1) {
       auto r = base_tester::push_action(config::system_account_name, "delegatebw"_n, from, mvo()
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

        base_tester::push_action(contract, "create"_n, contract, act );
    }

    auto issue( name contract, name manager, name to, asset amount ) {
       auto r = base_tester::push_action( contract, "issue"_n, manager, mutable_variant_object()
                ("to",      to )
                ("quantity", amount )
                ("memo", "")
        );
        produce_block();
        return r;
    }

    auto claim_rewards( name owner ) {
       auto r = base_tester::push_action( config::system_account_name, "claimrewards"_n, owner, mvo()("owner",  owner ));
       produce_block();
       return r;
    }

    auto set_privileged( name account ) {
       auto r = base_tester::push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  mvo()("account", account)("is_priv", 1));
       produce_block();
       return r;
    }

    auto register_producer(name producer) {
       auto r = base_tester::push_action(config::system_account_name, "regproducer"_n, producer, mvo()
                       ("producer",  name(producer))
                       ("producer_key", get_public_key( producer, "active" ) )
                       ("url", "" )
                       ("location", 0 )
                    );
       produce_block();
       return r;
    }


    auto undelegate_bandwidth( name from, name receiver, asset net, asset cpu ) {
       auto r = base_tester::push_action(config::system_account_name, "undelegatebw"_n, from, mvo()
                    ("from", from )
                    ("receiver", receiver)
                    ("unstake_net_quantity", net)
                    ("unstake_cpu_quantity", cpu)
                    );
       produce_block();
       return r;
    }

    asset get_balance( const account_name& act ) {
         return get_currency_balance("eosio.token"_n, symbol(CORE_SYMBOL), act);
    }

    void set_code_abi(const account_name& account, const vector<uint8_t>& wasm, const char* abi, const private_key_type* signer = nullptr) {
       wdump((account));
        set_code(account, wasm, signer);
        set_abi(account, abi, signer);
        if (account == config::system_account_name) {
           const auto& accnt = control->db().get<account_object,by_name>( account );
           abi_def abi_definition;
           BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
           abi_ser.set_abi(abi_definition, abi_serializer::create_yield_function( abi_serializer_max_time ));
        }
        produce_blocks();
    }


    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(bootseq_tests)

BOOST_FIXTURE_TEST_CASE( bootseq_test, bootseq_tester ) {
    try {

        // Create eosio.msig and eosio.token
        create_accounts({"eosio.msig"_n, "eosio.token"_n, "eosio.ram"_n, "eosio.ramfee"_n, "eosio.stake"_n, "eosio.vpay"_n, "eosio.bpay"_n, "eosio.saving"_n });
        // Set code for the following accounts:
        //  - eosio (code: eosio.bios) (already set by tester constructor)
        //  - eosio.msig (code: eosio.msig)
        //  - eosio.token (code: eosio.token)
        // set_code_abi("eosio.msig"_n, contracts::eosio_msig_wasm(), contracts::eosio_msig_abi().data());//, &eosio_active_pk);
        // set_code_abi("eosio.token"_n, contracts::eosio_token_wasm(), contracts::eosio_token_abi().data()); //, &eosio_active_pk);

        set_code_abi("eosio.msig"_n,
                     contracts::eosio_msig_wasm(),
                     contracts::eosio_msig_abi().data());//, &eosio_active_pk);
        set_code_abi("eosio.token"_n,
                     contracts::eosio_token_wasm(),
                     contracts::eosio_token_abi().data()); //, &eosio_active_pk);

        // Set privileged for eosio.msig and eosio.token
        set_privileged("eosio.msig"_n);
        set_privileged("eosio.token"_n);

        // Verify eosio.msig and eosio.token is privileged
        const auto& eosio_msig_acc = get<account_metadata_object, by_name>("eosio.msig"_n);
        BOOST_TEST(eosio_msig_acc.is_privileged() == true);
        const auto& eosio_token_acc = get<account_metadata_object, by_name>("eosio.token"_n);
        BOOST_TEST(eosio_token_acc.is_privileged() == true);


        // Create SYS tokens in eosio.token, set its manager as eosio
        auto max_supply = core_from_string("10000000000.0000"); /// 1x larger than 1B initial tokens
        auto initial_supply = core_from_string("1000000000.0000"); /// 1x larger than 1B initial tokens
        create_currency("eosio.token"_n, config::system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to eosio.system
        issue("eosio.token"_n, config::system_account_name, config::system_account_name, initial_supply);

        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

        // Create genesis accounts
        for( const auto& a : test_genesis ) {
           create_account( a.aname, config::system_account_name );
        }

        deploy_contract();

        // Buy ram and stake cpu and net for each genesis accounts
        for( const auto& a : test_genesis ) {
           auto ib = a.initial_balance;
           auto ram = 1000;
           auto net = (ib - ram) / 2;
           auto cpu = ib - net - ram;

           auto r = buyram(config::system_account_name, a.aname, asset(ram));
           BOOST_REQUIRE( !r->except_ptr );

           r = delegate_bandwidth("eosio.stake"_n, a.aname, asset(net), asset(cpu));
           BOOST_REQUIRE( !r->except_ptr );
        }

        auto producer_candidates = {
                "proda"_n, "prodb"_n, "prodc"_n, "prodd"_n, "prode"_n, "prodf"_n, "prodg"_n,
                "prodh"_n, "prodi"_n, "prodj"_n, "prodk"_n, "prodl"_n, "prodm"_n, "prodn"_n,
                "prodo"_n, "prodp"_n, "prodq"_n, "prodr"_n, "prods"_n, "prodt"_n, "produ"_n,
                "runnerup1"_n, "runnerup2"_n, "runnerup3"_n
        };

        // Register producers
        for( auto pro : producer_candidates ) {
           register_producer(pro);
        }

        // Vote for producers
        auto votepro = [&]( account_name voter, vector<account_name> producers ) {
          std::sort( producers.begin(), producers.end() );
          base_tester::push_action(config::system_account_name, "voteproducer"_n, voter, mvo()
                                ("voter",  name(voter))
                                ("proxy", name(0) )
                                ("producers", producers)
                     );
        };
        votepro( "b1"_n, { "proda"_n, "prodb"_n, "prodc"_n, "prodd"_n, "prode"_n, "prodf"_n, "prodg"_n,
                           "prodh"_n, "prodi"_n, "prodj"_n, "prodk"_n, "prodl"_n, "prodm"_n, "prodn"_n,
                           "prodo"_n, "prodp"_n, "prodq"_n, "prodr"_n, "prods"_n, "prodt"_n, "produ"_n} );
        votepro( "whale2"_n, {"runnerup1"_n, "runnerup2"_n, "runnerup3"_n} );
        votepro( "whale3"_n, {"proda"_n, "prodb"_n, "prodc"_n, "prodd"_n, "prode"_n} );

        // Total Stakes = b1 + whale2 + whale3 stake = (100,000,000 - 1,000) + (20,000,000 - 1,000) + (30,000,000 - 1,000)
        vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, "global"_n, "global"_n );

        BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 1499999997000);

        // No producers will be set, since the total activated stake is less than 150,000,000
        produce_blocks_for_n_rounds(2); // 2 rounds since new producer schedule is set when the first block of next round is irreversible
        auto active_schedule = control->head_block_state()->active_schedule;
        BOOST_TEST(active_schedule.producers.size() == 1u);
        BOOST_TEST(active_schedule.producers.front().producer_name == name("eosio"));

        // Spend some time so the producer pay pool is filled by the inflation rate
        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // 30 days
        // Since the total activated stake is less than 150,000,000, it shouldn't be possible to claim rewards
        BOOST_REQUIRE_THROW(claim_rewards("runnerup1"_n), eosio_assert_message_exception);

        // This will increase the total vote stake by (40,000,000 - 1,000)
        votepro( "whale4"_n, {"prodq"_n, "prodr"_n, "prods"_n, "prodt"_n, "produ"_n} );
        BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 1899999996000);

        // Since the total vote stake is more than 150,000,000, the new producer set will be set
        produce_blocks_for_n_rounds(2); // 2 rounds since new producer schedule is set when the first block of next round is irreversible
        active_schedule = control->head_block_state()->active_schedule;
        BOOST_REQUIRE(active_schedule.producers.size() == 21);
        BOOST_TEST(active_schedule.producers.at( 0).producer_name == name("proda"));
        BOOST_TEST(active_schedule.producers.at( 1).producer_name == name("prodb"));
        BOOST_TEST(active_schedule.producers.at( 2).producer_name == name("prodc"));
        BOOST_TEST(active_schedule.producers.at( 3).producer_name == name("prodd"));
        BOOST_TEST(active_schedule.producers.at( 4).producer_name == name("prode"));
        BOOST_TEST(active_schedule.producers.at( 5).producer_name == name("prodf"));
        BOOST_TEST(active_schedule.producers.at( 6).producer_name == name("prodg"));
        BOOST_TEST(active_schedule.producers.at( 7).producer_name == name("prodh"));
        BOOST_TEST(active_schedule.producers.at( 8).producer_name == name("prodi"));
        BOOST_TEST(active_schedule.producers.at( 9).producer_name == name("prodj"));
        BOOST_TEST(active_schedule.producers.at(10).producer_name == name("prodk"));
        BOOST_TEST(active_schedule.producers.at(11).producer_name == name("prodl"));
        BOOST_TEST(active_schedule.producers.at(12).producer_name == name("prodm"));
        BOOST_TEST(active_schedule.producers.at(13).producer_name == name("prodn"));
        BOOST_TEST(active_schedule.producers.at(14).producer_name == name("prodo"));
        BOOST_TEST(active_schedule.producers.at(15).producer_name == name("prodp"));
        BOOST_TEST(active_schedule.producers.at(16).producer_name == name("prodq"));
        BOOST_TEST(active_schedule.producers.at(17).producer_name == name("prodr"));
        BOOST_TEST(active_schedule.producers.at(18).producer_name == name("prods"));
        BOOST_TEST(active_schedule.producers.at(19).producer_name == name("prodt"));
        BOOST_TEST(active_schedule.producers.at(20).producer_name == name("produ"));

        // Spend some time so the producer pay pool is filled by the inflation rate
        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // 30 days
        // Since the total activated stake is larger than 150,000,000, pool should be filled reward should be bigger than zero
        claim_rewards("runnerup1"_n);
        BOOST_TEST(get_balance("runnerup1"_n).get_amount() > 0);

        const auto first_june_2018 = fc::seconds(1527811200); // 2018-06-01
        const auto first_june_2028 = fc::seconds(1843430400); // 2028-06-01
        // Ensure that now is yet 10 years after 2018-06-01 yet
        BOOST_REQUIRE(control->head_block_time().time_since_epoch() < first_june_2028);

        // This should thrown an error, since block one can only unstake all his stake after 10 years

        BOOST_REQUIRE_THROW(undelegate_bandwidth("b1"_n, "b1"_n, core_from_string("49999500.0000"), core_from_string("49999500.0000")), eosio_assert_message_exception);

        // Skip 10 years
        produce_block(first_june_2028 - control->head_block_time().time_since_epoch());

        // Block one should be able to unstake all his stake now
        undelegate_bandwidth("b1"_n, "b1"_n, core_from_string("49999500.0000"), core_from_string("49999500.0000"));

        return;
        produce_blocks(7000); /// produce blocks until virutal bandwidth can acomadate a small user
        wlog("minow" );
        votepro( "minow1"_n, {"p1"_n, "p2"_n} );


// TODO: Complete this test
    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
