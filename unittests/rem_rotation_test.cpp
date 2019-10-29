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

struct genesis_account {
   account_name aname;
   uint64_t     initial_balance;
};

static std::vector<genesis_account> test_genesis( {
    {N(b1),       100'000'000'0000ll},
    {N(whale1),    70'000'000'0000ll},
    {N(whale2),    40'000'000'0000ll},
    {N(whale3),    20'000'000'0000ll},
    {N(proda),      2'000'000'0000ll},
    {N(prodb),      2'000'000'0000ll},
    {N(prodc),      2'000'000'0000ll},
    {N(prodd),      2'000'000'0000ll},
    {N(prode),      2'000'000'0000ll},
    {N(prodf),      2'000'000'0000ll},
    {N(prodg),      2'000'000'0000ll},
    {N(prodh),      2'000'000'0000ll},
    {N(prodi),      2'000'000'0000ll},
    {N(prodj),      2'000'000'0000ll},
    {N(prodk),      2'000'000'0000ll},
    {N(prodl),      2'000'000'0000ll},
    {N(prodm),      2'000'000'0000ll},
    {N(prodn),      2'000'000'0000ll},
    {N(prodo),      2'000'000'0000ll},
    {N(prodp),      2'000'000'0000ll},
    {N(prodq),      2'000'000'0000ll},
    {N(prodr),      2'000'000'0000ll},
    {N(prods),      2'000'000'0000ll},
    {N(prodt),      2'000'000'0000ll},
    {N(produ),      2'000'000'0000ll},
    {N(runnerup1),  1'000'000'0000ll},
    {N(runnerup2),  1'000'000'0000ll},
    {N(runnerup3),  1'000'000'0000ll},
    {N(runnerup4),  1'000'000'0000ll},
    {N(runnerup5),  1'000'000'0000ll}
} );

class rotation_tester : public TESTER {
public:
   rotation_tester();

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

   void votepro( account_name voter, vector<account_name> producers ) {
      std::sort( producers.begin(), producers.end() );
      base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
                           ("voter", name(voter))
                           ("proxy", name(0) )
                           ("producers", producers)
               );
      produce_blocks();
   };

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

   uint32_t produce_blocks_until_schedule_is_changed(const uint32_t max_blocks) {
       const auto current_version = control->active_producers().version;
       uint32_t blocks_produced = 0;
       while (control->active_producers().version == current_version && blocks_produced < max_blocks) {
           produce_block();
           blocks_produced++;
       }
       return blocks_produced;
   }

   abi_serializer abi_ser;
};

rotation_tester::rotation_tester() {
    // Create rem.msig and rem.token
    create_accounts({N(rem.msig), N(rem.token), N(rem.rex), N(rem.ram),
                     N(rem.ramfee), N(rem.stake), N(rem.bpay),
                     N(rem.spay), N(rem.vpay), N(rem.saving)});

    // Set code for the following accounts:
    //  - rem (code: rem.bios) (already set by tester constructor)
    //  - rem.msig (code: rem.msig)
    //  - rem.token (code: rem.token)
    set_code_abi(N(rem.msig),
                 contracts::rem_msig_wasm(),
                 contracts::rem_msig_abi().data()); //, &rem_active_pk);
    set_code_abi(N(rem.token),
                 contracts::rem_token_wasm(),
                 contracts::rem_token_abi().data()); //, &rem_active_pk);

    // Set privileged for rem.msig and rem.token
    set_privileged(N(rem.msig));
    set_privileged(N(rem.token));

    // Verify rem.msig and rem.token is privileged
    const auto &rem_msig_acc = get<account_metadata_object, by_name>(N(rem.msig));
    BOOST_TEST(rem_msig_acc.is_privileged() == true);
    const auto &rem_token_acc = get<account_metadata_object, by_name>(N(rem.token));
    BOOST_TEST(rem_token_acc.is_privileged() == true);

    // Create SYS tokens in rem.token, set its manager as rem
    const auto max_supply = core_from_string("1000000000.0000");
    const auto initial_supply = core_from_string("900000000.0000");

    create_currency(N(rem.token), config::system_account_name, max_supply);
    // Issue the genesis supply of 1 billion SYS tokens to rem.system
    issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

    // Create genesis accounts
    for (const auto &account : test_genesis)
    {
        create_account(account.aname, config::system_account_name);
   }

   deploy_contract();

   // Buy ram and stake cpu and net for each genesis accounts
   for( const auto& account : test_genesis ) {
      const auto stake_quantity = account.initial_balance - 1000;

      const auto r = delegate_bandwidth(N(rem.stake), account.aname, asset(stake_quantity));
      BOOST_REQUIRE( !r->except_ptr );
   }

    // register whales as producers
    const auto whales_as_producers = { N(b1), N(whale1), N(whale2), N(whale3) };
    for( const auto& producer : whales_as_producers ) {
        register_producer(producer);
    }
}

BOOST_AUTO_TEST_SUITE(rem_rotation_tests)

// Expected schedule versions:
// V1: top21[proda - prodt, produ], top25[], rotation[]
// V2: top21[proda - prodt, produ], top25[], rotation[]
// V3: top21[proda - prodt, produ], top25[], rotation[]
// ...
BOOST_FIXTURE_TEST_CASE( no_rotation_test, rotation_tester ) {
    try {
        const auto producer_candidates = std::vector< account_name >{
            N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
            N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
            N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
        };

        // Register producers
        for( auto pro : producer_candidates ) {
            register_producer(pro);
        }

        votepro( N(b1), producer_candidates );
        votepro( N(whale1), producer_candidates );
        votepro( N(whale2), producer_candidates );
        votepro( N(whale3), producer_candidates );

        // Initial producers setup
        {
            produce_blocks(2);
            const auto active_schedule = control->head_block_state()->active_schedule;
            BOOST_REQUIRE( 
                std::equal( std::begin( producer_candidates ), std::end( producer_candidates ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        const auto rotation_period = fc::hours(4);

        // Next round
        {
            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( producer_candidates ), std::end( producer_candidates ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        // Next round
        {
            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( producer_candidates ), std::end( producer_candidates ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }
    } FC_LOG_AND_RETHROW()
}

// Expected schedule versions:
// V1: top21[proda - prodt, produ],     top25[runnerup1-runnerup4], rotation[runnerup1, runnerup2, runnerup3, runnerup4, produ]
// V2: top21[proda - prodt, runnerup1], top25[runnerup1-runnerup4], rotation[runnerup2, runnerup3, runnerup4, produ, runnerup1]
// V3: top21[proda - prodt, runnerup2], top25[runnerup1-runnerup4], rotation[runnerup3, runnerup4, produ, runnerup1, runnerup2]
// V4: top21[proda - prodt, runnerup3], top25[runnerup1-runnerup4], rotation[runnerup4, produ, runnerup1, runnerup2, runnerup3]
// V5: top21[proda - prodt, runnerup4], top25[runnerup1-runnerup4], rotation[produ, runnerup1, runnerup2, runnerup3, runnerup4]
// V6: top21[proda - prodt, produ],     top25[runnerup1-runnerup4], rotation[runnerup1, runnerup2, runnerup3, runnerup4, produ]
// ...
BOOST_FIXTURE_TEST_CASE( rotation_with_stable_top25, rotation_tester ) {
    try {
        auto producer_candidates = std::vector< account_name >{
            N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
            N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
            N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
        };

        // Register producers
        for( auto pro : producer_candidates ) {
            register_producer(pro);
        }

        auto runnerups = std::vector< account_name >{
            N(runnerup1), N(runnerup2), N(runnerup3),  N(runnerup4)
        };

        // Register producers
        for( auto pro : runnerups ) {
            register_producer(pro);
        }

        votepro( N(b1), producer_candidates );
        votepro( N(whale1), producer_candidates );
        votepro( N(whale2), producer_candidates );
        votepro( N(whale3), runnerups );

        // After this voting producers table looks like this:
        // Top21:
        //  proda-produ: (100'000'000'0000 + 70'000'000'0000 + 40'000'000'0000) / 21 = 10'000'000'0000
        // Standby (22-24):
        //  runnerup1-runnerup3: 20'000'000'0000 / 3 = 6'600'000'0000
        // 
        // So the first schedule should be proda-produ
        {
            produce_blocks(2);
            const auto active_schedule = control->head_block_state()->active_schedule;
            BOOST_REQUIRE( 
                std::equal( std::begin( producer_candidates ), std::end( producer_candidates ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        const auto rotation_period = fc::hours(4);

        // Next round should be included runnerup1 instead of produ
        {
            auto rota = producer_candidates;
            rota.back() = N(runnerup1);

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        // Next round should be included runnerup2 instead of runnerup1
        {
            auto rota = producer_candidates;
            rota.back() = N(runnerup2);

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        // Next round should be included runnerup3 instead of runnerup2
        {
            auto rota = producer_candidates;
            rota.back() = N(runnerup3);

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        // Next round should be included runnerup4 instead of runnerup3
        {
            auto rota = producer_candidates;
            rota.back() = N(runnerup4);

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        // Next round should be included produ instead of runnerup4
        {
            auto rota = producer_candidates;
            rota.back() = N(produ);

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        // Next round should be included runnerup1 instead of produ
        {
            auto rota = producer_candidates;
            rota.back() = N(runnerup1);

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }
    } FC_LOG_AND_RETHROW()
}

// Expected schedule versions:
// V1: top21[proda - produ],                   top25[runnerup1-runnerup4],                    rotation[runnerup1, runnerup2, runnerup3, runnerup4, produ]
// V2: top21[produ, proda - prods, runnerup1], top25[runnerup1-runnerup4],                    rotation[runnerup2, runnerup3, runnerup4, prodt, runnerup1]
// V3: top21[produ, proda - prods, runnerup2], top25[runnerup1-runnerup4],                    rotation[runnerup3, runnerup4, prodt, runnerup1, runnerup2]
// V4: top21[proda - prods, produ, runnerup3], top25[runnerup3, prodt, runnerup1, runnerup2], rotation[runnerup4, prodt, runnerup1, runnerup2, runnerup3]
BOOST_FIXTURE_TEST_CASE( top_25_reordered_test, rotation_tester ) {
    try {
        auto producer_candidates = std::vector< account_name >{
            N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
            N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
            N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
            N(runnerup1), N(runnerup2), N(runnerup3),  N(runnerup4), N(runnerup5)
        };

        // Register producers
        for( auto pro : producer_candidates ) {
            register_producer(pro);
        }

        votepro( N(b1), producer_candidates );
        votepro( N(whale1), producer_candidates);
        votepro( N(whale2), producer_candidates );
        votepro( N(whale3), producer_candidates );

        // Initial schedule should be proda-produ
        {
            produce_blocks(2);
            const auto active_schedule = control->head_block_state()->active_schedule;
            BOOST_REQUIRE( 
                std::equal( std::begin( producer_candidates ), std::begin( producer_candidates ) + 21,
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        const auto rotation_period = fc::hours(4);

        // produ votes for himself reaching top1
        // top21: produ, proda-prods
        // top25: prodt, runnerup1-runnerup4
        // prodt was in top25 of previous schedule so it will be rotated
        {
            // active schedule is sorted by name acutally
            auto rota = std::vector< account_name >{
                N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(produ), N(runnerup1)
            };

            votepro( N(produ), { N(produ) } );

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        // Next round
        {
            // active schedule is sorted by name acutally
            auto rota = std::vector< account_name >{
                N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(produ), N(runnerup2)
            };

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        // proda-prods, produ, runnerup4 votes for themselves
        // top21: proda-prods, produ, runnerup4 
        // top25: prodt, runnerup1-runnerup3
        // runnerup4 was in top25 of previous schedule so it will be rotated
        {
            // active schedule is sorted by name acutally
            auto rota = std::vector< account_name >{
                N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(produ), N(runnerup4)
            };

            for( auto pro: rota ) {
                votepro( pro, { pro } );
            }

            rota.back() = N(runnerup3);


            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }
    } FC_LOG_AND_RETHROW()
}

// Expected schedule versions:
// V1: top21[proda - prodt, produ],     top25[runnerup1-runnerup4],        rotation[runnerup1, runnerup2, runnerup3, runnerup4, produ]
// V2: top21[proda - prodt, runnerup5], top25[produ, runnerup1-runnerup3], rotation[runnerup5, runnerup1, runnerup2, runnerup3, produ]
// V3: top21[proda - prodt, runnerup5], top25[produ, runnerup1-runnerup3], rotation[runnerup1, runnerup2, runnerup3, produ, runnerup5]
BOOST_FIXTURE_TEST_CASE( new_top_21_test, rotation_tester ) {
    try {
        auto producer_candidates = std::vector< account_name >{
            N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
            N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
            N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
            N(runnerup1), N(runnerup2), N(runnerup3),  N(runnerup4), N(runnerup5)
        };

        // Register producers
        for( auto pro : producer_candidates ) {
            register_producer(pro);
        }

        votepro( N(b1), producer_candidates );
        votepro( N(whale1), producer_candidates);
        votepro( N(whale2), producer_candidates );
        votepro( N(whale3), producer_candidates );

        // Initial schedule should be proda-produ
        {
            produce_blocks(2);
            const auto active_schedule = control->head_block_state()->active_schedule;
            BOOST_REQUIRE( 
                std::equal( std::begin( producer_candidates ), std::begin( producer_candidates ) + 21,
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }


        const auto rotation_period = fc::hours(4);

        // produ-prodt voted for themselves getting additional 2'000'000'0000 votes
        // produ,runnerup1-runnerup4 did not voted for themselves
        // runnerup5 voted for himselve getting additional 1'000'000'0000 votes
        // top21: proda-prodt, runnerup5
        // top25: produ, runnerup1-runnerup3
        {
            auto rota = std::vector< account_name >{
                N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(runnerup5)
            };

            for (auto pro : rota) {
                votepro(pro, { pro });
            }

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }

        {
            auto rota = std::vector< account_name >{
                N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(runnerup1)
            };

            produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(rotation_period); // skip 4 hours (default rotation time) 
            produce_blocks_until_schedule_is_changed(2000); // produce some blocks until new schedule (prev wait can leave as in a middle of schedule)
            produce_blocks(2); // wait until schedule is accepted
            const auto active_schedule = control->head_block_state()->active_schedule;

            // std::cout << "expected: " << std::endl;
            // std::copy( std::begin( rota ), std::end( rota ), std::ostream_iterator< account_name >( std::cout, ", " ) );

            // std::cout << "\nactual: " << std::endl;
            // std::transform( std::begin( active_schedule.producers ), std::end( active_schedule.producers ), std::ostream_iterator< account_name >( std::cout, ", "), []( const auto& prod_key ){ return prod_key.producer_name; } );

            BOOST_REQUIRE( 
                std::equal( std::begin( rota ), std::end( rota ),
                            std::begin( active_schedule.producers ), std::end( active_schedule.producers ),
                            []( const account_name& rhs, const producer_authority& lhs ) {
                                return rhs == lhs.producer_name;
                            }
                )
            );
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()