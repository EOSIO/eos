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

std::vector<genesis_account> test_genesis( {
  {N(b1),       100'000'000'0000ll},
  {N(whale4),    40'000'000'0000ll},
  {N(whale3),    30'000'000'0000ll},
  {N(whale2),    20'000'000'0000ll},
  {N(proda),      1'000'000'0000ll},
  {N(prodb),      1'000'000'0000ll},
  {N(prodc),      1'000'000'0000ll},
  {N(prodd),      1'000'000'0000ll},
  {N(prode),      1'000'000'0000ll},
  {N(prodf),      1'000'000'0000ll},
  {N(prodg),      1'000'000'0000ll},
  {N(prodh),      1'000'000'0000ll},
  {N(prodi),      1'000'000'0000ll},
  {N(prodj),      1'000'000'0000ll},
  {N(prodk),      1'000'000'0000ll},
  {N(prodl),      1'000'000'0000ll},
  {N(prodm),      1'000'000'0000ll},
  {N(prodn),      1'000'000'0000ll},
  {N(prodo),      1'000'000'0000ll},
  {N(prodp),      1'000'000'0000ll},
  {N(prodq),      1'000'000'0000ll},
  {N(prodr),      1'000'000'0000ll},
  {N(prods),      1'000'000'0000ll},
  {N(prodt),      1'000'000'0000ll},
  {N(produ),      1'000'000'0000ll},
  {N(runnerup1),  1'000'000'0000ll},
  {N(runnerup2),  1'000'000'0000ll},
  {N(runnerup3),  1'000'000'0000ll},
  {N(minow1),         1'100'0000ll},
  {N(minow2),         1'050'0000ll},
  {N(minow3),         1'050'0000ll},
  {N(user1),          1'050'0000ll},
  {N(masses),   500'000'000'0000ll}
});

class bootseq_tester : public TESTER {
public:
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


    auto undelegate_bandwidth( name from, name receiver, asset unstake_quantity ) {
       auto r = base_tester::push_action(config::system_account_name, N(undelegatebw), from, mvo()
                    ("from", from )
                    ("receiver", receiver)
                    ("unstake_quantity", unstake_quantity)
                    );
       produce_block();
       return r;
    }

    asset get_balance( const account_name& act ) {
         return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
    }

    int64_t get_pending_pervote_reward( const account_name& prod ) {
       auto data = get_row_by_account(config::system_account_name, config::system_account_name, N(producers), prod);
       if (data.empty()) {
          return 0;
       }

       fc::variant v = abi_ser.binary_to_variant( "producer_info", data, abi_serializer_max_time );
       int64_t pending_pervote_reward = 0;
       fc::from_variant(v["pending_pervote_reward"], pending_pervote_reward);
       return pending_pervote_reward;
    }

    uint32_t get_expected_produced_blocks( const account_name& prod ) {
        auto data = get_row_by_account(config::system_account_name, config::system_account_name, N(producers), prod);
        if (data.empty()) {
            return 0;
        }

        fc::variant v = abi_ser.binary_to_variant( "producer_info", data, abi_serializer_max_time );
        uint32_t expected_produced_blocks = 0;
        fc::from_variant(v["expected_produced_blocks"], expected_produced_blocks);
        return expected_produced_blocks;
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

    uint32_t produce_blocks_until_schedule_is_changed(const uint32_t max_blocks) {
        const auto current_version = control->active_producers().version;
        uint32_t blocks_produced = 0;
        while (control->active_producers().version == current_version && blocks_produced < max_blocks) {
            produce_block();
            blocks_produced++;
        }
        return blocks_produced;
    }


    double get_total_votepay_shares() {
        double total = 0.0;
        auto state = get_global_state();
        auto arr = state["last_schedule"].get_array();
        for (auto& elem: arr) {
           total += elem["second"].as_double();
        }
        return total;
    }


    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(bootseq_tests)

BOOST_FIXTURE_TEST_CASE( bootseq_test, bootseq_tester ) {
    try {

        // Create rem.msig and rem.token
        create_accounts({N(rem.msig), N(rem.token), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.vpay), N(rem.spay), N(rem.saving) });
        // Set code for the following accounts:
        //  - rem (code: rem.bios) (already set by tester constructor)
        //  - rem.msig (code: rem.msig)
        //  - rem.token (code: rem.token)
        // set_code_abi(N(rem.msig), contracts::rem_msig_wasm(), contracts::rem_msig_abi().data());//, &rem_active_pk);
        // set_code_abi(N(rem.token), contracts::rem_token_wasm(), contracts::rem_token_abi().data()); //, &rem_active_pk);

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
        auto max_supply = core_from_string("1000000000.0000");
        auto initial_supply = core_from_string("900000000.0000");
        create_currency(N(rem.token), config::system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to rem.system
        issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

        // Create genesis accounts
        for( const auto& a : test_genesis ) {
           create_account( a.aname, config::system_account_name );
        }

        deploy_contract();

        // Buy ram and stake cpu and net for each genesis accounts
        for( const auto& a : test_genesis ) {
           auto stake_quantity = a.initial_balance - 1000;

           auto r = delegate_bandwidth(N(rem.stake), a.aname, asset(stake_quantity));
           BOOST_REQUIRE( !r->except_ptr );
        }

         push_action(N(rem.token), N(transfer), config::system_account_name, mutable_variant_object()
            ("from", name(config::system_account_name))
            ("to",   name(N(user1)))
            ("quantity", core_from_string("1050.0000"))
            ("memo", "")
         );
         produce_block();


        // register whales as producers
        const auto whales_as_producers = { N(b1), N(whale4), N(whale3), N(whale2) };
        for( const auto& producer : whales_as_producers ) {
           register_producer(producer);
        }

        auto producer_candidates = {
                N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
                N(runnerup1), N(runnerup2), N(runnerup3)
        };

        // Register producers
        for( auto pro : producer_candidates ) {
           register_producer(pro);
        }

        // Vote for producers
        auto votepro = [&]( account_name voter, vector<account_name> producers ) {
          std::sort( producers.begin(), producers.end() );
          base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
                                ("voter",  name(voter))
                                ("proxy", name(0) )
                                ("producers", producers)
                     );
        };
        votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                           N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                           N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        votepro( N(whale2), {N(runnerup1), N(runnerup2), N(runnerup3)} );
        votepro( N(whale3), {N(proda), N(prodb), N(prodc), N(prodd), N(prode)} );

        // Total Stakes = b1 + whale2 + whale3 stake = (100,000,000 - 1,000) + (20,000,000 - 1,000) + (30,000,000 - 1,000)
        vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );

        BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 1499999997000);

        // No producers will be set, since the total activated stake is less than 150,000,000
        produce_blocks_for_n_rounds(2); // 2 rounds since new producer schedule is set when the first block of next round is irreversible
        auto active_schedule = control->head_block_state()->active_schedule;
        BOOST_TEST(active_schedule.producers.size() == 1u);
        BOOST_TEST(active_schedule.producers.front().producer_name == "rem");

        // Spend 1 day so we can use claimrewards
        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(1 * 24 * 3600)); // 1 day
        // Since the total activated stake is less than 150,000,000, it shouldn't be possible to claim rewards
        BOOST_REQUIRE_THROW(claim_rewards(N(runnerup1)), eosio_assert_message_exception);

        // This will increase the total vote stake by (40,000,000 - 1,000)
        votepro( N(whale4), {N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 1899999996000);

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
        BOOST_REQUIRE(get_total_votepay_shares() > 0.999 && get_total_votepay_shares() <= 1.0);
        //Rounds in this schedule are started by prodp
        //Counters of expected_produced_blocks
        //proda-prodd - 12
        //prode       - 7
        //prodf-prodo - 0
        //prodp       - 9
        //prodq-produ - 12

        BOOST_REQUIRE_THROW(torewards(N(b1), config::system_account_name, core_from_string("20000.0000")), missing_auth_exception);
        torewards(config::system_account_name, config::system_account_name, core_from_string("20000.0000"));
        //Counters of expected_produced_blocks
        //prode - 8
        const auto saving_balance = get_balance(N(rem.saving)).get_amount();
        const auto spay_balance = get_balance(N(rem.spay)).get_amount();
        const auto vpay_balance = get_balance(N(rem.vpay)).get_amount();
        BOOST_REQUIRE(saving_balance >= 2000'0000);
        BOOST_REQUIRE_EQUAL(spay_balance, 14000'0000);
        BOOST_REQUIRE(vpay_balance <= 4000'0000);
        BOOST_REQUIRE_EQUAL(saving_balance + spay_balance + vpay_balance, 20000'0000);

        for (auto prod: active_schedule.producers) {
           BOOST_TEST(get_pending_pervote_reward(prod.producer_name) > 0);
        }

        // make changes to top 21 producer list
        votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                           N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                           N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
                           N(runnerup1)} );
        produce_block();
        BOOST_REQUIRE(get_total_votepay_shares() > 0.999 && get_total_votepay_shares() <= 1.0);
        produce_blocks_until_schedule_is_changed(2000);
        produce_blocks(2);
        //Counters of expected_produced_blocks
        //proda-prodf - 60
        //prodg       - 49
        //prodh-prodo - 48
        //prodp       - 57
        //prodq-produ - 60
        for (auto prod: { N(proda), N(prodb), N(prodc) }) {
           BOOST_TEST(get_expected_produced_blocks(prod) == 60);
        }
        BOOST_TEST(get_expected_produced_blocks(N(prodd)) == 49); //last producer in round
        for (auto prod: { N(prode), N(prodf), N(prodg), N(prodh), N(prodi), N(prodj), N(prodk), N(prodl) }) {
           BOOST_TEST(get_expected_produced_blocks(prod) == 48);
        }
        BOOST_TEST(get_expected_produced_blocks(N(prodm)) == 57); //first producer in round
        for (auto prod: { N(prodn), N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ) }) {
           BOOST_TEST(get_expected_produced_blocks(prod) == 60);
        }

        //update active schedule so we can check pending rewards after next schedule becomes active
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
        BOOST_TEST(active_schedule.producers.at(15).producer_name == "prodq");
        BOOST_TEST(active_schedule.producers.at(16).producer_name == "prodr");
        BOOST_TEST(active_schedule.producers.at(17).producer_name == "prods");
        BOOST_TEST(active_schedule.producers.at(18).producer_name == "prodt");
        BOOST_TEST(active_schedule.producers.at(19).producer_name == "produ");
        BOOST_TEST(active_schedule.producers.at(20).producer_name == "runnerup1");
        BOOST_TEST(get_pending_pervote_reward(N(runnerup1)) == 0);
        BOOST_REQUIRE(get_total_votepay_shares() > 0.999 && get_total_votepay_shares() <= 1.0);
        // make changes to top 21 producer list
        votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                           N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                           N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
                           N(runnerup1), N(runnerup2)} );
        produce_block();
        BOOST_REQUIRE(get_total_votepay_shares() > 0.999 && get_total_votepay_shares() <= 1.0);
        produce_blocks_until_schedule_is_changed(2000);
        produce_blocks(2);
        BOOST_REQUIRE(get_total_votepay_shares() > 0.999 && get_total_votepay_shares() <= 1.0);

        BOOST_TEST(get_expected_produced_blocks(N(prodp)) == 60); //wasn`t in second schedule, so expected_produced_blocks haven`t changed
        BOOST_TEST(get_expected_produced_blocks(N(proda)) == 85); //last producer in round
        for (auto prod: { N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg), N(prodh), N(prodi), N(prodj), N(prodk), N(prodl) }) {
           BOOST_TEST(get_expected_produced_blocks(prod) == 84);
        }
        BOOST_TEST(get_expected_produced_blocks(N(prodm)) == 93);
        for (auto prod: { N(prodn), N(prodo), N(prodq), N(prodr), N(prods), N(prodt), N(produ) }) {
           BOOST_TEST(get_expected_produced_blocks(prod) == 96);
        }
        BOOST_TEST(get_expected_produced_blocks(N(runnerup1)) == 36);

        BOOST_TEST(get_balance(N(runnerup1)).get_amount() == 0);
        BOOST_TEST(get_balance(N(proda)).get_amount() == 0);
        //runnerup should not get any pervote rewards because he wasn`t on schedule when torewards was called
        BOOST_REQUIRE_EQUAL(get_balance(N(rem.vpay)).get_amount(), vpay_balance);
        {
            const auto user1_balance = get_balance(N(user1)).get_amount();
            torewards(N(user1), N(user1), core_from_string("0.2000"));
            torewards(N(user1), N(user1), core_from_string("2.0000"));
            torewards(N(user1), N(user1), core_from_string("1.0000"));
            torewards(N(user1), N(user1), core_from_string("0.3000"));
            torewards(N(user1), N(user1), core_from_string("2.1000"));
            torewards(N(user1), N(user1), core_from_string("0.1000"));
            torewards(N(user1), N(user1), core_from_string("0.0300"));
            BOOST_REQUIRE(get_balance(N(user1)).get_amount() < user1_balance);
        }
        claim_rewards(N(runnerup1));
        BOOST_TEST(get_balance(N(runnerup1)).get_amount() > 0);
        claim_rewards(N(proda));
        BOOST_TEST(get_balance(N(proda)).get_amount() > 0);

        const auto first_june_2018 = fc::seconds(1527811200); // 2018-06-01
        const auto first_june_2028 = fc::seconds(1843430400); // 2028-06-01
        // Ensure that now is yet 10 years after 2018-06-01 yet
        BOOST_REQUIRE(control->head_block_time().time_since_epoch() < first_june_2028);

        // This should thrown an error, since block one can only unstake all his stake after 10 years

        BOOST_REQUIRE_THROW(undelegate_bandwidth(N(b1), N(b1), core_from_string("49999500.0000")), eosio_assert_message_exception);

        // Skip 10 years
        produce_block(first_june_2028 - control->head_block_time().time_since_epoch());

        // Block one should be able to unstake all his stake now
        undelegate_bandwidth(N(b1), N(b1), core_from_string("49999500.0000"));

        return;
        produce_blocks(7000); /// produce blocks until virutal bandwidth can acomadate a small user
        wlog("minow" );
        votepro( N(minow1), {N(p1), N(p2)} );


// TODO: Complete this test
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( stake_lock_test, bootseq_tester ) {
    try{
        // Create rem.msig and rem.token
        create_accounts({N(rem.msig), N(rem.token), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.vpay), N(rem.bpay), N(rem.saving) });

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
        auto max_supply = core_from_string("100.0000");
        auto initial_supply = core_from_string("100.0000");
        create_currency(N(rem.token), config::system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokenbs to rem.system
        issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

         // Create genesis accounts
        for( const auto& a : test_genesis ) {
            create_account( a.aname, config::system_account_name );
        }

        deploy_contract();

        // Buy ram and stake cpu and net for each genesis accounts
        for( const auto& a : test_genesis ) {
            auto stake_quantity = a.initial_balance - 1000;


            auto r = delegate_bandwidth(N(rem.stake), a.aname, asset(stake_quantity));
            BOOST_REQUIRE( !r->except_ptr );
        }

        const auto whales_as_producers = { N(b1), N(whale4), N(whale3), N(whale2) };
        for( const auto& producer : whales_as_producers ) {
            register_producer(producer);
        }

        auto producer_candidates = {N(proda)};

        register_producer( N(proda));

        // Vote for producers
        auto votepro = [&]( account_name voter, vector<account_name> producers ) {
            std::sort( producers.begin(), producers.end() );
            base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
                    ("voter",  name(voter))
                    ("proxy", name(0) )
                    ("producers", producers)
            );
        };

        votepro( N(b1), { N(proda)} );
        votepro( N(whale2), {N(proda)} );
        votepro( N(whale3), {N(proda)} );
        votepro( N(whale4), {N(proda)} );
        produce_block();

        // Spend some time so the producer pay pool is filled by the inflation rate
        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // 30 days

        // Spend some time so the producer pay pool is filled by the inflation rate
        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // 30 days
        const auto second_july_2020 = fc::seconds(1593648000); // 2020-07-02
        // Ensure that now is yet 6 month after 2020-01-01 yet
        BOOST_REQUIRE(control->head_block_time().time_since_epoch() < second_july_2020);

        //Try to unstake tokens from produceir should throw error, because unstaked condition does not met ( producer can unstake only after six month )
        BOOST_REQUIRE_THROW(undelegate_bandwidth(N(proda), N(proda), core_from_string("1.0000")), eosio_assert_message_exception);

        // Skip 6 month
        produce_block(second_july_2020 - control->head_block_time().time_since_epoch());

        // Block one should be able to unstake all his stake now
        BOOST_REQUIRE(undelegate_bandwidth(N(proda), N(proda), core_from_string("1.0000")));

    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
