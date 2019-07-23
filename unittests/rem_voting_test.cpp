/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
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

struct rem_genesis_account {
   account_name name;
   uint64_t     initial_balance;
};

std::vector<rem_genesis_account> rem_test_genesis( {
  {N(b1),        100'000'000'0000ll}, // TODO investigate why `b1` should be at least this value?
  {N(whale1),     40'000'000'0000ll},
  {N(whale2),     30'000'000'0000ll},
  {N(whale3),     20'000'000'0000ll},
  {N(proda),         500'000'0000ll},
  {N(prodb),         500'000'0000ll},
  {N(prodc),         500'000'0000ll},
  {N(prodd),         500'000'0000ll},
  {N(prode),         500'000'0000ll},
  {N(prodf),         500'000'0000ll},
  {N(prodg),         500'000'0000ll},
  {N(prodh),         500'000'0000ll},
  {N(prodi),         500'000'0000ll},
  {N(prodj),         500'000'0000ll},
  {N(prodk),         500'000'0000ll},
  {N(prodl),         500'000'0000ll},
  {N(prodm),         500'000'0000ll},
  {N(prodn),         500'000'0000ll},
  {N(prodo),         500'000'0000ll},
  {N(prodp),         500'000'0000ll},
  {N(prodq),         500'000'0000ll},
  {N(prodr),         500'000'0000ll},
  {N(prods),         500'000'0000ll},
  {N(prodt),         500'000'0000ll},
  {N(produ),         500'000'0000ll},
  {N(runnerup1),     200'000'0000ll},
  {N(runnerup2),     150'000'0000ll},
  {N(runnerup3),     100'000'0000ll},
});

class voting_tester : public TESTER {
public:
   void deploy_contract( bool call_init = true ) {
      set_code( config::system_account_name, contracts::eosio_system_wasm() );
      set_abi( config::system_account_name, contracts::eosio_system_abi().data() );
      if( call_init ) {
         base_tester::push_action(config::system_account_name, N(init),
                                  config::system_account_name,  mutable_variant_object()
                                  ("version", 0)
                                  ("core", CORE_SYM_STR)
            );
      }
      const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);
   }

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

    auto claim_rewards( name owner ) {
       auto r = base_tester::push_action( config::system_account_name, N(claimrewards), owner, mvo()("owner",  owner ));
       produce_block();
       return r;
    }

    auto set_privileged( name account ) {
       auto r = base_tester::push_action(config::system_account_name, N(setpriv), config::system_account_name,  mvo()("account", account)("is_priv", 1));
       produce_block();
       return r;
    }

    auto register_producer(name producer) {
       auto r = base_tester::push_action(config::system_account_name, N(regproducer), producer, mvo()
                       ("producer",  name(producer))
                       ("producer_key", get_public_key( producer, "active" ) )
                       ("url", "" )
                       ("location", 0 )
                    );
       produce_block();
       return r;
    }

    asset get_balance( const account_name& act ) {
         return get_currency_balance(N(eosio.token), symbol(CORE_SYMBOL), act);
    }

    void set_code_abi(const account_name& account, const vector<uint8_t>& wasm, const char* abi, const private_key_type* signer = nullptr) {
       wdump((account));
        set_code(account, wasm, signer);
        set_abi(account, abi, signer);
        if (account == config::system_account_name) {
           const auto& accnt = control->db().get<account_object,by_name>( account );
           abi_def abi_definition;
           BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
           abi_ser.set_abi(abi_definition, abi_serializer_max_time);
        }
        produce_blocks();
    }

    fc::variant get_producer_info( const account_name& act ) {
       vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(producers), act );
       return abi_ser.binary_to_variant( "producer_info", data, abi_serializer_max_time );
    }
 
    fc::variant get_voter_info( const account_name& act ) {
       vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(voters), act );
       return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "voter_info", data, abi_serializer_max_time );
    }
 
    // Vote for producers
    void votepro( account_name voter, vector<account_name> producers ) {
       std::sort( producers.begin(), producers.end() );
       base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
                            ("voter", name(voter))
                            ("proxy", name(0) )
                            ("producers", producers)
                );
    };

    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(rem_voting_tests)
BOOST_FIXTURE_TEST_CASE( rem_voting_test, voting_tester ) {
    try {
        // Create eosio.msig and eosio.token
        create_accounts({N(eosio.msig), N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake), N(eosio.vpay), N(eosio.bpay), N(eosio.saving) });

        // Set code for the following accounts:
        //  - eosio (code: eosio.bios) (already set by tester constructor)
        //  - eosio.msig (code: eosio.msig)
        //  - eosio.token (code: eosio.token)
        set_code_abi(N(eosio.msig),
                     contracts::eosio_msig_wasm(),
                     contracts::eosio_msig_abi().data());//, &eosio_active_pk);
        set_code_abi(N(eosio.token),
                     contracts::eosio_token_wasm(),
                     contracts::eosio_token_abi().data()); //, &eosio_active_pk);

        // Set privileged for eosio.msig and eosio.token
        set_privileged(N(eosio.msig));
        set_privileged(N(eosio.token));

        // Verify eosio.msig and eosio.token is privileged
        const auto& eosio_msig_acc = get<account_metadata_object, by_name>(N(eosio.msig));
        BOOST_TEST(eosio_msig_acc.is_privileged() == true);
        const auto& eosio_token_acc = get<account_metadata_object, by_name>(N(eosio.token));
        BOOST_TEST(eosio_token_acc.is_privileged() == true);


        // Create SYS tokens in eosio.token, set its manager as eosio
        const auto max_supply     = core_from_string("1000000000.0000"); /// 10x larger than 1B initial tokens
        const auto initial_supply = core_from_string("100000000.0000");  /// 10x larger than 1B initial tokens

        create_currency(N(eosio.token), config::system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to eosio.system
        issue(N(eosio.token), config::system_account_name, config::system_account_name, initial_supply);

        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

        // Create genesis accounts
        for( const auto& a : rem_test_genesis ) {
           create_account( a.name, config::system_account_name );
        }

        deploy_contract();

        // Buy ram and stake cpu and net for each genesis accounts
        for( const auto& acc : rem_test_genesis ) {
           const auto ib = acc.initial_balance;
           const auto ram = 1000;
           const auto net = (ib - ram) / 2;
           const auto cpu = ib - net - ram;

           auto r = buyram(config::system_account_name, acc.name, asset(ram));
           BOOST_REQUIRE( !r->except_ptr );

           r = delegate_bandwidth(N(eosio.stake), acc.name, asset(net), asset(cpu));
           BOOST_REQUIRE( !r->except_ptr );
        }

        // Register producers
        const auto producer_candidates = {
                N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
        };
        for( const auto& producer : producer_candidates ) {
           register_producer(producer);
        }



        // Runners-up should not be able to register as producer because their stakes are less then producer threshold
        const auto producer_runnerups = {
                N(runnerup1), N(runnerup2), N(runnerup3)
        };
        for( const auto& producer : producer_runnerups ) {
           BOOST_REQUIRE_THROW( register_producer(producer), eosio_assert_message_exception );
        }

        // Re-stake for runnerups
        for( const auto& producer : producer_runnerups ) {
           const auto net = 100'000'0000ll;
           const auto cpu = 100'000'0000ll;

           const auto r = delegate_bandwidth(N(eosio.stake), producer, asset(net), asset(cpu));
           BOOST_REQUIRE( !r->except_ptr );
        }

        // Now runnerups have enough stake to become producers
        for( const auto& producer : producer_runnerups ) {
           register_producer(producer);
        }

        // whale3 is not a producer so can't vote
        BOOST_REQUIRE_THROW( votepro( N(whale3), {N(runnerup1), N(runnerup2), N(runnerup3)} ), eosio_assert_message_exception );

        // register whales as producers
        const auto whales_as_producers = { N(b1), N(whale1), N(whale2) };
        for( const auto& producer : whales_as_producers ) {
           register_producer(producer);
        }

        votepro( N(whale1), {N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                             N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                             N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        votepro( N(whale2), {N(proda), N(prodb), N(prodc), N(prodd), N(prode)} );

        vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );

        // Total Stakes = whale1 + whale2 stakes = (40'000'000'0000 - 1,000) + (30'000'000'0000 - 1,000) = 69'999'999.8000
        BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 699999998000);

        // No producers will be set, since the total activated stake is less than 150,000,000
        produce_blocks_for_n_rounds(2); // 2 rounds since new producer schedule is set when the first block of next round is irreversible
        auto active_schedule = control->head_block_state()->active_schedule;
        BOOST_TEST(active_schedule.producers.size() == 1u);
        BOOST_TEST(active_schedule.producers.front().producer_name == "eosio");

        // This will increase the total vote stake by (1'000'000'000'000 - 1,000)
        votepro( N(b1), {N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                         N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                         N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 1699999997000); // 169'999'999.7000


        // Since the total vote stake is more than 150,000,000, the new producer set will be set
        produce_blocks_for_n_rounds(2); // 2 rounds since new producer schedule is set when the first block of next round is irreversible
        active_schedule = control->head_block_state()->active_schedule;
        BOOST_REQUIRE(active_schedule.producers.size() == 21);
        BOOST_TEST(active_schedule.producers.at(0).producer_name == "proda");
        BOOST_TEST(active_schedule.producers.at(1).producer_name == "prodb");
        BOOST_TEST(active_schedule.producers.at(2).producer_name == "prodc");
        BOOST_TEST(active_schedule.producers.at(3).producer_name == "prodd");
        BOOST_TEST(active_schedule.producers.at(4).producer_name == "prode");
        BOOST_TEST(active_schedule.producers.at(5).producer_name == "prodf");
        BOOST_TEST(active_schedule.producers.at(6).producer_name == "prodg");
        BOOST_TEST(active_schedule.producers.at(7).producer_name == "prodh");
        BOOST_TEST(active_schedule.producers.at(8).producer_name == "prodi");
        BOOST_TEST(active_schedule.producers.at(9).producer_name == "prodj");
        BOOST_TEST(active_schedule.producers.at(10).producer_name == "prodk");
        BOOST_TEST(active_schedule.producers.at(11).producer_name == "prodl");
        BOOST_TEST(active_schedule.producers.at(12).producer_name == "prodm");
        BOOST_TEST(active_schedule.producers.at(13).producer_name == "prodn");
        BOOST_TEST(active_schedule.producers.at(14).producer_name == "prodo");
        BOOST_TEST(active_schedule.producers.at(15).producer_name == "prodp");
        BOOST_TEST(active_schedule.producers.at(16).producer_name == "prodq");
        BOOST_TEST(active_schedule.producers.at(17).producer_name == "prodr");
        BOOST_TEST(active_schedule.producers.at(18).producer_name == "prods");
        BOOST_TEST(active_schedule.producers.at(19).producer_name == "prodt");
        BOOST_TEST(active_schedule.producers.at(20).producer_name == "produ");
        return;
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( rem_vote_weight_test, voting_tester ) {
   try {
      // Create eosio.msig and eosio.token
      create_accounts({N(eosio.msig), N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake), N(eosio.vpay), N(eosio.bpay), N(eosio.saving) });

      // Set code for the following accounts:
      //  - eosio (code: eosio.bios) (already set by tester constructor)
      //  - eosio.msig (code: eosio.msig)
      //  - eosio.token (code: eosio.token)
      set_code_abi(N(eosio.msig),
                  contracts::eosio_msig_wasm(),
                  contracts::eosio_msig_abi().data());//, &eosio_active_pk);
      set_code_abi(N(eosio.token),
                  contracts::eosio_token_wasm(),
                  contracts::eosio_token_abi().data()); //, &eosio_active_pk);

      // Set privileged for eosio.msig and eosio.token
      set_privileged(N(eosio.msig));
      set_privileged(N(eosio.token));

      // Verify eosio.msig and eosio.token is privileged
      const auto& eosio_msig_acc = get<account_metadata_object, by_name>(N(eosio.msig));
      BOOST_TEST(eosio_msig_acc.is_privileged() == true);
      const auto& eosio_token_acc = get<account_metadata_object, by_name>(N(eosio.token));
      BOOST_TEST(eosio_token_acc.is_privileged() == true);


      // Create SYS tokens in eosio.token, set its manager as eosio
      const auto max_supply     = core_from_string("1000000000.0000"); /// 10x larger than 1B initial tokens
      const auto initial_supply = core_from_string("100000000.0000");  /// 10x larger than 1B initial tokens

      create_currency(N(eosio.token), config::system_account_name, max_supply);
      // Issue the genesis supply of 1 billion SYS tokens to eosio.system
      issue(N(eosio.token), config::system_account_name, config::system_account_name, initial_supply);

      auto actual = get_balance(config::system_account_name);
      BOOST_REQUIRE_EQUAL(initial_supply, actual);

      // Create genesis accounts
      for( const auto& a : rem_test_genesis ) {
         create_account( a.name, config::system_account_name );
      }

      deploy_contract();

      // Buy ram and stake cpu and net for each genesis accounts
      for( const auto& acc : rem_test_genesis ) {
         const auto ib = acc.initial_balance;
         const auto ram = 1000;
         const auto net = (ib - ram) / 2;
         const auto cpu = ib - net - ram;

         auto r = buyram(config::system_account_name, acc.name, asset(ram));
         BOOST_REQUIRE( !r->except_ptr );

         r = delegate_bandwidth(N(eosio.stake), acc.name, asset(net), asset(cpu));
         BOOST_REQUIRE( !r->except_ptr );
      }

      // Register producers
      const auto producer_candidates = {
               N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
               N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
               N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
      };
      for( const auto& producer : producer_candidates ) {
         register_producer(producer);
      }

      // Register whales as producers
      const auto whales_as_producers = { N(b1), N(whale1), N(whale2), N(whale2) };
      for( const auto& producer : whales_as_producers ) {
         register_producer(producer);
      }

      // 0 days
      {
         votepro( N(whale1), { N(proda) } );

         // vote gains full power at:     1593388805500000
         // voteproducer was done at:     1577836844500000
         // 180 days in microseconds is:  15552000000000
         // rem vote weight:             ~0.0000025
         // eos weight:                  ~1091357.4775723184
         // staked = 399999999000
         // 1091357.4775723184 * 0.00000250771 * 399999999000 ~= 1.09472322 × 10^12
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 1094725862107.0167 == prod["total_votes"].as_double() );
      }

      // 30 days
      {
         const auto voter = get_voter_info( "whale1" );
         const auto last_vote_weight = voter["last_vote_weight"].as_double();

         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // +30 days
         votepro( N(whale1), { N(proda) } );

         // rem vote weight: 0.16666917
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 78454001807635136 == prod["total_votes"].as_double() );
         BOOST_TEST_REQUIRE( last_vote_weight < prod["total_votes"].as_double() );
      }

      // 180 days
      {
         const auto voter = get_voter_info( "whale1" );
         const auto last_vote_weight = voter["last_vote_weight"].as_double();

         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(150 * 24 * 3600)); // +150 days
         votepro( N(whale1), { N(proda) } );

         // rem vote weight: 1.0
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 617365016928612860 == prod["total_votes"].as_double() );
         BOOST_TEST_REQUIRE( last_vote_weight < prod["total_votes"].as_double() );
      }

      // re-delegating vote power 100%
      {
         const auto voter = get_voter_info( "whale1" );
         const auto last_vote_weight = voter["last_vote_weight"].as_double();

         const auto r = delegate_bandwidth(N(eosio.stake), N(whale1), asset(10'000'000'0000LL), asset(10'000'000'0000LL));
         BOOST_REQUIRE( !r->except_ptr );

         votepro( N(whale1), { N(proda) } );

         // staked 40KK + 20KK re-staked
         // vote mature adjusted time: 0 + 180 * 20 / (40 + 20)
         // rem vote weight:           0.66
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 617365047215702140 == prod["total_votes"].as_double() );
         BOOST_TEST_REQUIRE( last_vote_weight < prod["total_votes"].as_double() );
      }

      // TODO validate adjusted voter.vote_mature_time
      // re-delegating vote power 66%
      {
         const auto voter = get_voter_info( "whale1" );
         const auto last_vote_weight = voter["last_vote_weight"].as_double();

         const auto r = delegate_bandwidth(N(eosio.stake), N(whale1), asset(10'000'000'0000LL), asset(10'000'000'0000LL));
         BOOST_REQUIRE( !r->except_ptr );

         votepro( N(whale1), { N(proda) } );

         // staked 60KK + 20KK re-staked
         // vote mature adjusted time: 60 + 180 * 20 / (60 + 20)
         // rem vote weight:           0.41
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 514470927477248640 == prod["total_votes"].as_double() );
         BOOST_TEST_REQUIRE( last_vote_weight > prod["total_votes"].as_double() );
      }
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()