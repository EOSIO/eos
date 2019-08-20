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
   voting_tester();

   void deploy_contract( bool call_init = true ) {
      set_code( config::system_account_name, contracts::rem_system_wasm() );
      set_abi( config::system_account_name, contracts::rem_system_abi().data() );
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

    auto delegate_bandwidth( name from, name receiver, asset stake_quantity, uint8_t transfer = 1) {
       auto r = base_tester::push_action(config::system_account_name, N(delegatebw), from, mvo()
                    ("from", from )
                    ("receiver", receiver)
                    ("stake_quantity", stake_quantity)
                    ("transfer", transfer)
                    );
       produce_block();
       return r;
    }

   auto undelegate_bandwidth( name from, name receiver, asset unstake_quantity ) {
       auto r = base_tester::push_action(config::system_account_name, N(undelegatebw), from, mvo()
                    ("from", from )
                    ("receiver", receiver)
                    ("unstake_quantity", unstake_quantity)
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

    auto transfer( name from, name to, asset quantity ) {
       auto r = push_action( N(rem.token), N(transfer), config::system_account_name, 
                             mutable_variant_object()
                             ("from", from)
                             ("to", to)
                             ("quantity", quantity)
                             ("memo", "")
       );
       produce_block();

       return r;
    }

    auto torewards( name caller, name payer, asset amount ) {
        auto r = base_tester::push_action(config::system_account_name, N(torewards), caller, mvo()
                ("payer", payer)
                ("amount", amount)
        );
        produce_block();
        return r;
    }

    void set_lock_period(uint64_t mature_period ) {
        base_tester::push_action(config::system_account_name, N(setlockperiod),config::system_account_name,  mvo()("period_in_days", mature_period));
        produce_block();
    }

    void set_unlock_period(uint64_t mature_period ) {
        base_tester::push_action(config::system_account_name, N(setunloperiod),config::system_account_name,  mvo()("period_in_days", mature_period));
        produce_block();
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
         return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
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
       produce_blocks();
    };

   auto unregister_producer(name producer) {
       auto r = base_tester::push_action(config::system_account_name, N(unregprod), producer, mvo()
               ("producer",  name(producer))
               ("producer_key", get_public_key( producer, "active" ) )
               ("url", "" )
               ("location", 0 )
       );
       produce_block();
       return r;
   }

    abi_serializer abi_ser;
};

voting_tester::voting_tester() {
   // Create rem.msig and rem.token
   create_accounts({N(rem.msig), N(rem.token), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.bpay), N(rem.spay), N(rem.vpay), N(rem.saving) });

   // Set code for the following accounts:
   //  - rem (code: rem.bios) (already set by tester constructor)
   //  - rem.msig (code: rem.msig)
   //  - rem.token (code: rem.token)
   set_code_abi(N(rem.msig),
               contracts::rem_msig_wasm(),
               contracts::rem_msig_abi().data());//, &rem_active_pk);
   set_code_abi(N(rem.token),
               contracts::rem_token_wasm(),
               contracts::rem_token_abi().data()); //, &rem_active_pk);

   // Set privileged for rem.msig and rem.token
   set_privileged(N(rem.msig));
   set_privileged(N(rem.token));

   // Verify rem.msig and rem.token is privileged
   const auto& rem_msig_acc = get<account_metadata_object, by_name>(N(rem.msig));
   BOOST_TEST(rem_msig_acc.is_privileged() == true);
   const auto& rem_token_acc = get<account_metadata_object, by_name>(N(rem.token));
   BOOST_TEST(rem_token_acc.is_privileged() == true);

   // Create SYS tokens in rem.token, set its manager as rem
   const auto max_supply     = core_from_string("1000000000.0000"); /// 10x larger than 1B initial tokens
   const auto initial_supply = core_from_string("100000000.0000");  /// 10x larger than 1B initial tokens

   create_currency(N(rem.token), config::system_account_name, max_supply);
   // Issue the genesis supply of 1 billion SYS tokens to rem.system
   issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

   auto actual = get_balance(config::system_account_name);
   BOOST_REQUIRE_EQUAL(initial_supply, actual);

   // Create genesis accounts
   for( const auto& account : rem_test_genesis ) {
      create_account( account.name, config::system_account_name );
   }

   deploy_contract();

   // Buy ram and stake cpu and net for each genesis accounts
   for( const auto& account : rem_test_genesis ) {
      const auto stake_quantity = account.initial_balance - 1000;

      const auto r = delegate_bandwidth(N(rem.stake), account.name, asset(stake_quantity));
      BOOST_REQUIRE( !r->except_ptr );
   }
}

BOOST_AUTO_TEST_SUITE(rem_voting_tests)
BOOST_FIXTURE_TEST_CASE( rem_voting_test, voting_tester ) {
    try {
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
           BOOST_REQUIRE_EXCEPTION(
                 register_producer(producer),
                 eosio_assert_message_exception,
                 fc_exception_message_starts_with("assertion failure with message: user should stake at least " + core_from_string("250000.0000").to_string() + " to become a producer"));
        }

        // Re-stake for runnerups
        for( const auto& producer : producer_runnerups ) {
           const auto stake = 200'000'0000ll;

           const auto r = delegate_bandwidth(N(rem.stake), producer, asset(stake));
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
        BOOST_TEST(active_schedule.producers.front().producer_name == "rem");

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
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( rem_vote_reassertion_test, voting_tester ) {
   try {
      // Register producers
      const auto producer_candidates = {
               N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
               N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
               N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
      };
      for( const auto& producer : producer_candidates ) {
         register_producer(producer);
      }

      // register whales as producers
      const auto whales_as_producers = { N(b1), N(whale1), N(whale2), N(whale3) };
      for( const auto& producer : whales_as_producers ) {
         register_producer(producer);
      }

      // This will increase the total vote stake by (1'000'000'000'000 - 1,000)
      votepro( N(b1), {N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                       N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                       N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );

      votepro( N(whale1), {N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                           N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                           N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );

      votepro( N(whale2), {N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                           N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                           N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );

      votepro( N(whale3), {N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                           N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                           N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );

      votepro( N(proda), {N(proda)} );

      BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 1904999995000);

      // Since the total vote stake is more than 150,000,000, the new producer set will be set
      produce_blocks_for_n_rounds(2); // 2 rounds since new producer schedule is set when the first block of next round is irreversible
      const auto active_schedule = control->head_block_state()->active_schedule;
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

      // Skip 180 Days so vote gain 100% power
      produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(180 * 24 * 3600));

      votepro( N(proda), {N(proda)} );
      produce_blocks_for_n_rounds(2);
      claim_rewards(N(proda));

      // Day 0
      // We just claimed rewards so unpaid blocks == 0
      {
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST( 0 == prod["unpaid_blocks"].as_int64() );
      }

      // Day 3
      {
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(3 * 24 * 3600)); // +3 days
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST( 0 <= prod["unpaid_blocks"].as_int64() );
      }

      // Day 7
      // Vote was not re-asserted for 7 days so we will not get paid anymore
      {
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(4 * 24 * 3600)); // +4 days
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST( 0 <= prod["unpaid_blocks"].as_int64() );

         claim_rewards(N(proda));
      }

      // Day 10
      // We claimed rewards and vote was not re-asserted, so unpaid_blocks == 0
      {
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(3 * 24 * 3600)); // +3 days
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST( 0 == prod["unpaid_blocks"].as_int64() );
      }

      // Day 11
      // Vote is re-asserted so we should have unpaid blocks again
      {
         votepro( N(proda), {N(proda)} );
         produce_blocks_for_n_rounds(2);

         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(1 * 24 * 3600)); // +1 days
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST( 0 <= prod["unpaid_blocks"].as_int64() );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( rem_vote_weight_test, voting_tester ) {
   try {
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

      // Day 0
      {
         const auto voter = get_voter_info( "whale1" );
         BOOST_TEST_REQUIRE( 0.0 == voter["last_vote_weight"].as_double() );

         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 0.0 == prod["total_votes"].as_double() );
      }

      {
         votepro( N(whale1), { N(proda) } );
         // vote gains full power at:     1593388805500000
         // voteproducer was done at:     1577836844500000
         // 180 days in microseconds is:  15552000000000

         // eos weight: ~20.057692;
         // rem weight: ~0.000002;
         // staked:     399999999000
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 1031774.2926451379 == prod["total_votes"].as_double() );
      }

      // Day 30
      {
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // +30 days
         votepro( N(whale1), { N(proda) } );

         // eos weight: ~20.134615;
         // rem weight: ~0.170384;
         // staked:     399999999000
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 1372237214636.8337 == prod["total_votes"].as_double() );
      }

      // Day 180
      {
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(150 * 24 * 3600)); // +150 days
         votepro( N(whale1), { N(proda) } );

         // eos weight: 20.557692;
         // rem weight: 1.000000;
         // staked:     399999999000
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 8223076902519.2305 == prod["total_votes"].as_double() );
      }

      // Day 180 (0)
      // re-staking vote power 100%
      // staked 40KK 
      // re-staked 20KK 
      {
         const auto r = delegate_bandwidth(N(rem.stake), N(whale1), asset(20'000'000'0000LL));
         BOOST_REQUIRE( !r->except_ptr );

         votepro( N(whale1), { N(proda) } );

         // adjusted:   now + 0 Days * 40 / 60 + 180 Days * 20 / 60 => 60 Days
         // eos weight: 20.557692;
         // rem weight: 0.666667;
         // staked:     599999999000
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 8223077702492.6377 == prod["total_votes"].as_double() );
      }

      // Day 210 (30)
      // re-staking vote power 83%
      // staked 60KK
      // re-staked 20KK 
      {
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // +30 days
         
         const auto r = delegate_bandwidth(N(rem.stake), N(whale1), asset(20'000'000'0000LL));
         BOOST_REQUIRE( !r->except_ptr );

         votepro( N(whale1), { N(proda) } );

         // adjusted:   now + 30 Days * 60 / 80 + 180 Days * 20 / 80 => 67.5 Days
         // eos weight: 20.634615;
         // rem weight: 0.625;
         // staked:     799999999000
         const auto prod = get_producer_info( "proda" );
         BOOST_TEST_REQUIRE( 10363317352113.912 == prod["total_votes"].as_double() );
      }
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( undelegate_after_resignation_test, voting_tester ) {
   try {
      const auto producers = { N(b1), N(whale1), N(whale2), N(whale3) };
      for( const auto& producer : producers ) {
         register_producer(producer);
         votepro( producer, { N(b1) } );
      }

      transfer( name(config::system_account_name), name(N(proda)), core_from_string("10000.0000") );
      delegate_bandwidth( N(proda), N(runnerup1), core_from_string("1000.0000"), 0 );

      register_producer( N(proda) );

      BOOST_REQUIRE_THROW( undelegate_bandwidth( N(proda), N(proda), core_from_string("1.0000") ), eosio_assert_message_exception );
      BOOST_REQUIRE( undelegate_bandwidth( N(proda), N(runnerup1), core_from_string("500.0000") ) );
      BOOST_REQUIRE( delegate_bandwidth( N(proda), N(runnerup1), core_from_string("500.0000"), 0 ) );

      produce_min_num_of_blocks_to_spend_time_wo_inactive_prod( fc::seconds(180 * 24 * 3600) );
      unregister_producer( N(proda) );

      BOOST_REQUIRE_THROW( undelegate_bandwidth( N(proda), N(proda), core_from_string("1.0000") ), eosio_assert_message_exception );
      BOOST_REQUIRE( undelegate_bandwidth( N(proda), N(runnerup1), core_from_string("1000.0000") ) );
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( resignation_test, voting_tester ) {
   try {

      const auto producers = { N(b1), N(proda), N(whale1), N(whale2), N(whale3) };
      for( const auto& producer : producers ) {
         register_producer(producer);
      }

      for( const auto& producer : producers ) {
         votepro( producer, { N(proda) } );
      }

      // Day 0
      {
         // Should throw because producer stake is locked for 180 days
         BOOST_REQUIRE_THROW( unregister_producer( N(proda) ), eosio_assert_message_exception );
      }

      // Day 180 so stake is unlocked
      {
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(180 * 24 * 3600)); // +150 days

         const auto prod = get_producer_info( "proda" );
         BOOST_TEST( 0 < prod["unpaid_blocks"].as_int64() );

         claim_rewards( N(proda) );
         // Claim rewards is called from `unregprod` and allowed only once per day
         BOOST_REQUIRE_THROW( unregister_producer( N(proda) ), eosio_assert_message_exception );
      }

      // Day 181
      {
         // Re-assert vote so we will participate in block rewards
         votepro( N(proda), { N(proda) } );
         torewards(config::system_account_name, config::system_account_name, core_from_string("20000.0000"));

         BOOST_TEST( 0 == get_producer_info( "proda" )["unpaid_blocks"].as_int64() );
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(1 * 24 * 3600)); // +1 day
         BOOST_TEST( 0 < get_producer_info( "proda" )["unpaid_blocks"].as_int64() );

         const auto balance_before_unreg = get_balance(N(proda)).get_amount();
         BOOST_TEST( 0 == balance_before_unreg );

         unregister_producer( N(proda) );
         BOOST_TEST( balance_before_unreg < get_balance(N(proda)).get_amount() );

         BOOST_TEST( 0 == get_producer_info( "proda" )["unpaid_blocks"].as_int64() );
      }
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( stake_lock_period_test, voting_tester ) {
   try {

      const auto producers = { N(b1), N(proda), N(whale1), N(whale2), N(whale3) };
      for( const auto& producer : producers ) {
         register_producer(producer);
      }

      for( const auto& producer : producers ) {
         votepro( producer, { N(proda) } );
      }

      // Should throw because producer stake is locked for 180 days
      BOOST_REQUIRE_THROW( unregister_producer( N(proda) ), eosio_assert_message_exception );

      delegate_bandwidth(N(rem.stake), N(proda), asset(2'000'000'000));


      set_lock_period(90);
      produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(90 * 24 * 3600));

      delegate_bandwidth(N(rem.stake), N(proda), asset(1'000'000'000));

      produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(90 * 24 * 3600));

      unregister_producer( N(proda) );
   } FC_LOG_AND_RETHROW()
}
BOOST_AUTO_TEST_SUITE_END()