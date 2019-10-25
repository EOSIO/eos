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

namespace
{
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

class rewards_tester : public TESTER {
public:
   rewards_tester();

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

   auto refund_to_stake( const name& to ) {
      auto r = base_tester::push_action(
         config::system_account_name, N(refundtostake), to,
         mvo()("owner", to)
      );

      produce_block();
      return r;
   }

    fc::microseconds microseconds_since_epoch_of_iso_string( const fc::variant& v ) {
        return time_point::from_iso_string( v.as_string() ).time_since_epoch();
    }

    abi_serializer abi_ser;
};

rewards_tester::rewards_tester() {
   // Create rem.msig and rem.token
   create_accounts({N(rem.msig), N(rem.token), N(rem.rex), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.bpay), N(rem.spay), N(rem.vpay), N(rem.saving) });

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

BOOST_AUTO_TEST_SUITE(rem_rewards_tests)
BOOST_FIXTURE_TEST_CASE( perstake_rewards_test, rewards_tester ) {
    try {
        const auto producers = std::vector< name >{ N(proda), N(prodb), N(prodc) };
        for( const auto& producer : producers ) {
            register_producer(producer);
            votepro( producer, {producer} );
        }
        
        // vote for prods to activate 15% of stake
        const auto whales = std::vector< name >{ N(b1), N(whale1), N(whale2) };
        for( const auto& whale : whales ) {
            votepro( whale, producers );
        }
        

        // wait until schedule is accepted so torewards could split amount between producers
        produce_blocks_for_n_rounds(2);
        {
            const auto active_schedule = control->head_block_state()->active_schedule;
            BOOST_TEST_REQUIRE(active_schedule.producers.size() == 3u);
            BOOST_TEST_REQUIRE(active_schedule.producers.at(0).producer_name == name{"proda"} );
            BOOST_TEST_REQUIRE(active_schedule.producers.at(1).producer_name == name{"prodb"} );
            BOOST_TEST_REQUIRE(active_schedule.producers.at(2).producer_name == name{"prodc"} );
        }

        {
            torewards( config::system_account_name, config::system_account_name, asset{ 100'0000 } );
            // 100'000 * 0.6 perstake share
            BOOST_TEST_REQUIRE( get_global_state()["perstake_bucket"].as_int64() == 59'9998 );
            // 100'000 * 0.3 percote share
            BOOST_TEST_REQUIRE( get_global_state()["pervote_bucket"].as_int64() == 29'9997 );

            // b1 + whale1-whale2 + proda-prodc = 171'499'999'4000 delegated stake
            // prodd-produ - didn't voted so they don't counts as Guardians
            // runnerup1-3 have less then 250'000'0000 so their stakes do not update `total_guardians_stake`
            BOOST_TEST_REQUIRE( get_global_state()["total_guardians_stake"].as_int64() == 171'499'999'4000 );

            // b1 staked: 99'999'999'9000; total_staked: 171'499'999'4000; share ~0.583 * 60'0000
            BOOST_TEST_REQUIRE( get_voter_info( N(b1) )["pending_perstake_reward"].as_int64() == 34'9854 );

            // proda-prodc have the same total_votes so their pervote shares are equal ~0.33 * 29'9997
            BOOST_TEST_REQUIRE( get_producer_info( N(prodb) )["pending_pervote_reward"].as_int64() == 9'9999 );
            BOOST_TEST_REQUIRE( get_producer_info( N(prodb) )["pending_pervote_reward"].as_int64() == get_producer_info( N(prodc) )["pending_pervote_reward"].as_int64() );

            // each of b1, whale1-whale2, proda-prodc has stakes more then 250'000'0000 and voted so all of them participate in perstake rewards
            // prodb staked: 499'999'9000; total_staked: 171'499'999'4000; share ~0.002 * 60'0000 and should be thesame as prodc
            BOOST_TEST_REQUIRE( get_voter_info( N(prodb) )["pending_perstake_reward"].as_int64() == 1749 );
            BOOST_TEST_REQUIRE( get_voter_info( N(prodb) )["pending_perstake_reward"].as_int64() == get_voter_info( N(prodc) )["pending_perstake_reward"].as_int64() );
        }

        {
            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod( fc::days( 1 ) );
            claim_rewards( N(proda) );

            BOOST_TEST_REQUIRE( get_producer_info( N(proda) )["pending_pervote_reward"].as_int64() == 0 );
            BOOST_TEST_REQUIRE( get_voter_info( N(proda) )["pending_perstake_reward"].as_int64() == 0 );

            // 59'9998 - 1749
            BOOST_TEST_REQUIRE( get_global_state()["perstake_bucket"].as_int64() == 59'8249 );
        }

        // b1 can claim his pending_perstake_reward even after loosing Guardian status
        {
            claim_rewards( N(b1) );
            BOOST_TEST_REQUIRE( get_voter_info( N(b1) )["pending_perstake_reward"].as_int64() == 0 );

            claim_rewards( N(prodb) );
            BOOST_TEST_REQUIRE( get_voter_info( N(prodb) )["pending_perstake_reward"].as_int64() == 0 );

            // 59'8249 - 34'9854 - 1749
            BOOST_TEST_REQUIRE( get_global_state()["perstake_bucket"].as_int64() == 24'6646 );
        }

        // after 7 days all Guardians loose their status if vote is not re-asserted
        // so all perstake rewards goes to Remme Savings
        {
            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod( fc::days( 7 ) );
            torewards( config::system_account_name, config::system_account_name, asset{ 100'0000 } );

            BOOST_TEST_REQUIRE( get_global_state()["total_guardians_stake"].as_int64() == 0 );
            
            BOOST_TEST_REQUIRE( get_voter_info( N(prodc) )["pending_perstake_reward"].as_int64() == 1749 );

            BOOST_TEST_REQUIRE( get_global_state()["perstake_bucket"].as_int64() == 24'6646 );
        }

        // after prodc re-asserted vote he is the only one challenger on `perstake_bucket`
        {
            votepro( N(prodc), { N(prodc) } );
            BOOST_TEST_REQUIRE( get_voter_info( N(prodc) )["pending_perstake_reward"].as_int64() == 1749 );

            torewards( config::system_account_name, config::system_account_name, asset{ 100'0000 } );
            // 24'6646 + 60'0000
            BOOST_TEST_REQUIRE( get_global_state()["perstake_bucket"].as_int64() == 84'6646 );

            BOOST_TEST_REQUIRE( get_voter_info( N(prodc) )["pending_perstake_reward"].as_int64() == 60'1749 );
        }

        // after everyone claimed their rewards perstake_bucket should be 0
        {
            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod( fc::days( 1 ) );

            const auto claimers = std::vector< name >{ N(b1), N(whale1), N(whale2), N(proda), N(prodb), N(prodc) };
            for( const auto& claimer : claimers ) {
                claim_rewards( claimer );
            }
        
            BOOST_TEST_REQUIRE( get_global_state()["perstake_bucket"].as_int64() == 0 );
            for( const auto& claimer : claimers ) {
                BOOST_TEST_REQUIRE( get_voter_info( claimer )["pending_perstake_reward"].as_int64() == 0 );
            }
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace anonymous