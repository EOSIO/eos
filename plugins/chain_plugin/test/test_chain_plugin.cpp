#define BOOST_TEST_MODULE chain_plugin
#include <eosio/chain/permission_object.hpp>
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <fc/variant_object.hpp>
#include <contracts.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fc/log/logger.hpp>
#include <eosio/chain/exceptions.hpp>
#include <Runtime/Runtime.h>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain_apis;
using namespace eosio::testing;
using namespace eosio::chain_apis;
using namespace fc;

using mvo = fc::mutable_variant_object;

class chain_plugin_tester : public TESTER {
public:

    action_result push_action( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = config::system_account_name;
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data, abi_serializer::create_yield_function(abi_serializer_max_time) );

         return base_tester::push_action( std::move(act), (auth ? signer : signer == "bob111111111"_n ? "alice1111111"_n : "bob111111111"_n).to_uint64_t() );
    }

    action_result deposit( const account_name& owner, const asset& amount ) {
      return push_action( name(owner), "deposit"_n, mvo()
                          ("owner",  owner)
                          ("amount", amount)
                          );
    }

    void transfer( name from, name to, const asset& amount, name manager = config::system_account_name ) {
      base_tester::push_action( "eosio.token"_n, "transfer"_n, manager, mutable_variant_object()
                                ("from",    from)
                                ("to",      to )
                                ("quantity", amount)
                                ("memo", "")
                                );
    }

    action_result stake( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action( name(from), "delegatebw"_n, mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
                          ("transfer", 0 )
      );
    }

    action_result stake( const account_name& acnt, const asset& net, const asset& cpu ) {
      return stake( acnt, acnt, net, cpu );
    }

    asset get_balance( const account_name& act ) {
      vector<char> data = get_row_by_account( "eosio.token"_n, act, "accounts"_n, name(symbol(CORE_SYMBOL).to_symbol_code().value) );
      return data.empty() ? asset(0, symbol(CORE_SYMBOL)) : token_abi_ser.binary_to_variant("account", data, abi_serializer::create_yield_function( abi_serializer_max_time ))["balance"].as<asset>();
    }

    transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, uint32_t ram_bytes = 8000 ) {
        signed_transaction trx;
        set_transaction_headers(trx);

        authority owner_auth;
        owner_auth =  authority( get_public_key( a, "owner" ) );

        trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                  newaccount{
                                          .creator  = creator,
                                          .name     = a,
                                          .owner    = owner_auth,
                                          .active   = authority( get_public_key( a, "active" ) )
                                  });

        trx.actions.emplace_back( get_action( config::system_account_name, "buyrambytes"_n, vector<permission_level>{{creator,config::active_name}},
                                              mvo()
                                                      ("payer", creator)
                                                      ("receiver", a)
                                                      ("bytes", ram_bytes) )
        );
        trx.actions.emplace_back( get_action( config::system_account_name, "delegatebw"_n, vector<permission_level>{{creator,config::active_name}},
                                              mvo()
                                                      ("from", creator)
                                                      ("receiver", a)
                                                      ("stake_net_quantity", core_from_string("10.0000") )
                                                      ("stake_cpu_quantity", core_from_string("10.0000") )
                                                      ("transfer", 0 )
                                  )
        );

        set_transaction_headers(trx);
        trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
        return push_transaction( trx );
    }

    transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, asset ramfunds, bool multisig,
                                                        asset net = core_from_string("10.0000"), asset cpu = core_from_string("10.0000") ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      if (multisig) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth =  authority( get_public_key( a, "owner" ) );
      }

      authority active_auth( get_public_key( a, "active" ) );

      auto sort_permissions = []( authority& auth ) {
         std::sort( auth.accounts.begin(), auth.accounts.end(),
                    []( const permission_level_weight& lhs, const permission_level_weight& rhs ) {
                        return lhs.permission < rhs.permission;
                    }
                  );
      };

      {
         FC_ASSERT( owner_auth.threshold <= std::numeric_limits<weight_type>::max(), "threshold is too high" );
         FC_ASSERT( active_auth.threshold <= std::numeric_limits<weight_type>::max(), "threshold is too high" );
         owner_auth.accounts.push_back( permission_level_weight{ {a, config::eosio_code_name},
                                                                 static_cast<weight_type>(owner_auth.threshold) } );
         sort_permissions(owner_auth);
         active_auth.accounts.push_back( permission_level_weight{ {a, config::eosio_code_name},
                                                                  static_cast<weight_type>(active_auth.threshold) } );
         sort_permissions(active_auth);
      }

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });


      trx.actions.emplace_back( get_action( config::system_account_name, "buyram"_n, vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("quant", ramfunds) )
                              );

      trx.actions.emplace_back( get_action( config::system_account_name, "delegatebw"_n, vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", net )
                                            ("stake_cpu_quantity", cpu )
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
    }

    void create_currency( name contract, name manager, asset maxsupply ) {
        auto act =  mutable_variant_object()
                ("issuer",       manager )
                ("maximum_supply", maxsupply );

        base_tester::push_action(contract, "create"_n, contract, act );
    }

    void issue( name to, const asset& amount, name manager = config::system_account_name ) {
        base_tester::push_action( "eosio.token"_n, "issue"_n, manager, mutable_variant_object()
                ("to",      to )
                ("quantity", amount )
                ("memo", "")
        );
    }
    void setup_system_accounts(){
       create_accounts({ "eosio.token"_n, "eosio.ram"_n, "eosio.ramfee"_n, "eosio.stake"_n,
                         "eosio.bpay"_n, "eosio.vpay"_n, "eosio.saving"_n, "eosio.names"_n, "eosio.rex"_n });

       set_code( "eosio.token"_n, contracts::eosio_token_wasm() );
       set_abi( "eosio.token"_n, contracts::eosio_token_abi().data() );

       {
           const auto& accnt = control->db().get<account_object,by_name>( "eosio.token"_n );
           abi_def abi;
           BOOST_CHECK_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
           token_abi_ser.set_abi(abi, abi_serializer::create_yield_function( abi_serializer_max_time ));
       }

       create_currency( "eosio.token"_n, config::system_account_name, core_from_string("10000000000.0000") );
       issue(config::system_account_name,      core_from_string("1000000000.0000"));
       BOOST_CHECK_EQUAL( core_from_string("1000000000.0000"), get_balance( name("eosio") ) );

       set_code( config::system_account_name, contracts::eosio_system_wasm() );
       set_abi( config::system_account_name, contracts::eosio_system_abi().data() );

       base_tester::push_action(config::system_account_name, "init"_n,
                                config::system_account_name,  mutable_variant_object()
                                        ("version", 0)
                                        ("core", CORE_SYM_STR));

       {
           const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
           abi_def abi;
           BOOST_CHECK_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
           abi_ser.set_abi(abi, abi_serializer::create_yield_function( abi_serializer_max_time ));
       }

    }

    read_only::get_account_results get_account_info(const account_name acct){
       auto account_object = control->get_account(acct);
       read_only::get_account_params params = { account_object.name };
       chain_apis::read_only plugin(*(control.get()), {}, fc::microseconds::maximum());
       return plugin.get_account(params);
    }

    transaction_trace_ptr setup_producer_accounts( const std::vector<account_name>& accounts ) {
        account_name creator(config::system_account_name);
        signed_transaction trx;
        set_transaction_headers(trx);
        asset cpu = core_from_string("80.0000");
        asset net = core_from_string("80.0000");
        asset ram = core_from_string("1.0000");

        for (const auto& a: accounts) {
            authority owner_auth( get_public_key( a, "owner" ) );
            trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                      newaccount{
                                              .creator  = creator,
                                              .name     = a,
                                              .owner    = owner_auth,
                                              .active   = authority( get_public_key( a, "active" ) )
                                      });

            trx.actions.emplace_back( get_action( config::system_account_name, "buyram"_n, vector<permission_level>{ {creator, config::active_name} },
                                                  mvo()
                                                          ("payer", creator)
                                                          ("receiver", a)
                                                          ("quant", ram) )
            );

            trx.actions.emplace_back( get_action( config::system_account_name, "delegatebw"_n, vector<permission_level>{ {creator, config::active_name} },
                                                  mvo()
                                                          ("from", creator)
                                                          ("receiver", a)
                                                          ("stake_net_quantity", net)
                                                          ("stake_cpu_quantity", cpu )
                                                          ("transfer", 0 )
                                      )
            );
        }

        set_transaction_headers(trx);
        trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
        return push_transaction( trx );
    }

    action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
        action_result r = push_action( acnt, "regproducer"_n, mvo()
                ("producer",  acnt )
                ("producer_key", get_public_key( acnt, "active" ) )
                ("url", "" )
                ("location", 0 )
        );
        BOOST_CHECK_EQUAL( success(), r);
        return r;
    }

    action_result unstake(const account_name& from, const account_name& to, const asset& net, const asset& cpu){
       return push_action(name(from), "undelegatebw"_n, mvo()
               ("from",  from)
               ("receiver", to)
               ("unstake_net_quantity", net )
               ("unstake_cpu_quantity", cpu )
       );
    }

    action_result buyram( const account_name& payer, account_name receiver, const asset& eosin ) {
        return push_action( payer, "buyram"_n, mvo()( "payer",payer)("receiver",receiver)("quant",eosin) );
    }

    vector<name> active_and_vote_producers() {
        //stake more than 15% of total EOS supply to activate chain
        transfer( name("eosio"), name("alice1111111"), core_from_string("650000000.0000"), name("eosio") );
        BOOST_CHECK_EQUAL( success(), stake( name("alice1111111"), name("alice1111111"), core_from_string("300000000.0000"), core_from_string("300000000.0000") ) );

        // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
        std::vector<account_name> producer_names;
        {
            producer_names.reserve('z' - 'a' + 1);
            const std::string root("defproducer");
            for ( char c = 'a'; c < 'a'+21; ++c ) {
                producer_names.emplace_back(root + std::string(1, c));
            }
            setup_producer_accounts(producer_names);
            for (const auto& p: producer_names) {

                BOOST_CHECK_EQUAL( success(), regproducer(p) );
            }
        }
        produce_blocks( 250);

        auto trace_auth = TESTER::push_action(config::system_account_name, updateauth::get_name(), config::system_account_name, mvo()
                ("account", name(config::system_account_name).to_string())
                ("permission", name(config::active_name).to_string())
                ("parent", name(config::owner_name).to_string())
                ("auth",  authority(1, {key_weight{get_public_key( config::system_account_name, "active" ), 1}}, {
                                            permission_level_weight{{config::system_account_name, config::eosio_code_name}, 1},
                                            permission_level_weight{{config::producers_account_name,  config::active_name}, 1}
                                    }
                ))
        );
        BOOST_CHECK_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

        //vote for producers
        {
            transfer( config::system_account_name, name("alice1111111"), core_from_string("100000000.0000"), config::system_account_name );
            BOOST_CHECK_EQUAL(success(), stake( name("alice1111111"), core_from_string("30000000.0000"), core_from_string("30000000.0000") ) );
            BOOST_CHECK_EQUAL(success(), buyram( name("alice1111111"), name("alice1111111"), core_from_string("30000000.0000") ) );
            BOOST_CHECK_EQUAL(success(), push_action("alice1111111"_n, "voteproducer"_n, mvo()
                    ("voter",  "alice1111111")
                    ("proxy", name(0).to_string())
                    ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
            )
            );
        }
        produce_blocks( 250 );

        auto producer_keys = control->head_block_state()->active_schedule.producers;
        BOOST_CHECK_EQUAL( 21, producer_keys.size() );
        BOOST_CHECK_EQUAL( name("defproducera"), producer_keys[0].producer_name );

        return producer_names;
    }

    action_result buyrex(const name& from, const asset& amount){
       return push_action(name(from), "buyrex"_n, mvo()
               ("from", from)
               ("amount", amount)
       );
   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
};

BOOST_AUTO_TEST_SUITE(chain_plugin_tests)

BOOST_FIXTURE_TEST_CASE(account_results_total_resources_test, chain_plugin_tester) { try {

    produce_blocks(10);
    setup_system_accounts();
    produce_blocks();
    create_account_with_resources("alice1111111"_n, config::system_account_name);
    //stake more than 15% of total EOS supply to activate chain
    transfer( name("eosio"), name("alice1111111"), core_from_string("650000000.0000"), name("eosio") );

    read_only::get_account_results results = get_account_info(name("alice1111111"));
    BOOST_CHECK(results.total_resources.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK_EQUAL(core_from_string("10.0000"), results.total_resources["net_weight"].as<asset>());
    BOOST_CHECK_EQUAL(core_from_string("10.0000"), results.total_resources["cpu_weight"].as<asset>());
    BOOST_CHECK_EQUAL(results.total_resources["ram_bytes"].as_int64() > 0, true);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(account_results_self_delegated_bandwidth_test, chain_plugin_tester) { try {

    produce_blocks(10);
    setup_system_accounts();
    produce_blocks();
    const asset nstake = core_from_string("1.0000");
    const asset cstake = core_from_string("2.0000");
    create_account_with_resources("alice1111111"_n, config::system_account_name, core_from_string("1.0000"), false);
    BOOST_CHECK_EQUAL(success(), stake(config::system_account_name, name("alice1111111"), nstake, cstake));

    read_only::get_account_results results = get_account_info(name("alice1111111"));
    BOOST_CHECK(results.total_resources.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK_EQUAL(core_from_string("11.0000"), results.total_resources["net_weight"].as<asset>());
    BOOST_CHECK_EQUAL(core_from_string("12.0000"), results.total_resources["cpu_weight"].as<asset>());

    //self delegate bandwidth
    transfer( name("eosio"), name("alice1111111"), core_from_string("650000000.0000"), name("eosio") );
    BOOST_CHECK_EQUAL(success(), stake(name("alice1111111"), name("alice1111111"), nstake, cstake));

    results = get_account_info(name("alice1111111"));
    BOOST_CHECK(results.self_delegated_bandwidth.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK_EQUAL(core_from_string("1.0000"), results.self_delegated_bandwidth["net_weight"].as<asset>());
    BOOST_CHECK_EQUAL(core_from_string("2.0000"), results.self_delegated_bandwidth["cpu_weight"].as<asset>());

    BOOST_CHECK(results.total_resources.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK_EQUAL(core_from_string("12.0000"), results.total_resources["net_weight"].as<asset>());
    BOOST_CHECK_EQUAL(core_from_string("14.0000"), results.total_resources["cpu_weight"].as<asset>());

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(account_results_refund_request_test, chain_plugin_tester) { try {

    produce_blocks(10);
    setup_system_accounts();
    produce_blocks();

    setup_producer_accounts({"producer1111"_n});
    regproducer("producer1111"_n);

    read_only::get_account_results results = get_account_info(name("producer1111"));
    BOOST_CHECK(results.total_resources.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK_EQUAL(core_from_string("80.0000"), results.total_resources["net_weight"].as<asset>());

    //cross 15 percent threshold
    {
        signed_transaction trx;
        set_transaction_headers(trx);

        trx.actions.emplace_back( get_action( config::system_account_name, "delegatebw"_n,
                                              vector<permission_level>{{config::system_account_name, config::active_name}},
                                              mvo()
                                                      ("from", name{config::system_account_name})
                                                      ("receiver", "producer1111")
                                                      ("stake_net_quantity", core_from_string("150000000.0000") )
                                                      ("stake_cpu_quantity", core_from_string("0.0000") )
                                                      ("transfer", 1 )
                                  )
        );
        trx.actions.emplace_back( get_action( config::system_account_name, "voteproducer"_n,
                                              vector<permission_level>{{"producer1111"_n, config::active_name}},
                                              mvo()
                                                      ("voter", "producer1111")
                                                      ("proxy", name(0).to_string())
                                                      ("producers", vector<account_name>(1, "producer1111"_n))
                                  )
        );

        set_transaction_headers(trx);
        trx.sign( get_private_key( config::system_account_name, "active" ), control->get_chain_id()  );
        trx.sign( get_private_key( "producer1111"_n, "active" ), control->get_chain_id()  );
        push_transaction( trx );
    }

    results = get_account_info(name("producer1111"));
    BOOST_CHECK_EQUAL(core_from_string("150000080.0000"), results.total_resources["net_weight"].as<asset>());

    BOOST_CHECK_EQUAL(success(), unstake(name("producer1111"),name("producer1111"), core_from_string("150000000.0000"), core_from_string("0.0000")));
    results = get_account_info(name("producer1111"));
    BOOST_CHECK(results.total_resources.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK(results.refund_request.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK_EQUAL(core_from_string("150000000.0000"), results.refund_request["net_amount"].as<asset>());
    BOOST_CHECK_EQUAL(core_from_string("80.0000"), results.total_resources["net_weight"].as<asset>());


} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(account_results_voter_info_test, chain_plugin_tester) { try {

    produce_blocks(10);
    setup_system_accounts();

    create_account_with_resources("alice1111111"_n, config::system_account_name, core_from_string("1.0000"), false);

    active_and_vote_producers();
    read_only::get_account_results results = get_account_info(name("alice1111111"));

    BOOST_CHECK(results.voter_info.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK_EQUAL(21, results.voter_info["producers"].size());

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(account_results_rex_info_test, chain_plugin_tester) { try {

    produce_blocks(10);
    setup_system_accounts();

    create_account_with_resources("alice1111111"_n, config::system_account_name, core_from_string("1.0000"), false);

    //stake more than 15% of total EOS supply to activate chain
    transfer( name("eosio"), name("alice1111111"), core_from_string("650000000.0000"), name("eosio") );
    deposit(name("alice1111111"), core_from_string("1000.0000"));
    BOOST_CHECK_EQUAL( success(), buyrex(name("alice1111111"), core_from_string("100.0000")) );

    read_only::get_account_results results = get_account_info(name("alice1111111"));
    BOOST_CHECK(results.rex_info.get_type() != fc::variant::type_id::null_type);
    BOOST_CHECK_EQUAL(core_from_string("100.0000"), results.rex_info["vote_stake"].as<asset>());
    //BOOST_CHECK_EQUAL(0, results.rex_info["matured_rex"]);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()