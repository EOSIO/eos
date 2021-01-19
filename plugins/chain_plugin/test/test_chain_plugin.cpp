#define BOOST_TEST_MODULE chain_plugin
#include <eosio/chain/permission_object.hpp>
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include <fc/variant_object.hpp>

#include <contracts.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fc/log/logger.hpp>
#include <eosio/chain/exceptions.hpp>
#include <Runtime/Runtime.h>

//#include "../../../unittests/eosio_system_tester.hpp"
//#include "/Users/venu.kailasa/jira-issues/epe-584/eosio.contracts/tests/eosio.system_tester.hpp"

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

//using namespace eosio_system;

#if 0
struct _abi_hash {
   name owner;
   fc::sha256 hash;
};
FC_REFLECT( _abi_hash, (owner)(hash) );

struct connector {
   asset balance;
   double weight = .5;
};
FC_REFLECT( connector, (balance)(weight) );
#endif

inline fc::mutable_variant_object voter( account_name acct ) {
   return mutable_variant_object()
      ("owner", acct)
      ("proxy", name(0).to_string())
      ("producers", variants() )
      ("staked", int64_t(0))
      //("last_vote_weight", double(0))
      ("proxied_vote_weight", double(0))
      ("is_proxy", 0)
      ;
}
inline fc::mutable_variant_object voter( std::string_view acct ) {
   return voter( account_name(acct) );
}

inline fc::mutable_variant_object voter( account_name acct, const asset& vote_stake ) {
   return voter( acct )( "staked", vote_stake.get_amount() );
}
inline fc::mutable_variant_object voter( std::string_view acct, const asset& vote_stake ) {
   return voter( account_name(acct), vote_stake );
}

inline fc::mutable_variant_object voter( account_name acct, int64_t vote_stake ) {
   return voter( acct )( "staked", vote_stake );
}
inline fc::mutable_variant_object voter( std::string_view acct, int64_t vote_stake ) {
   return voter( account_name(acct), vote_stake );
}

class chain_plugin_tester : public TESTER {
public:
   void preactivate()
   {
         const auto& pfm = control->get_protocol_feature_manager();

         auto schedule_preactivate_protocol_feature = [&]() {
             auto preactivate_feature_digest = pfm.get_builtin_digest(builtin_protocol_feature_t::preactivate_feature);
             FC_ASSERT( preactivate_feature_digest, "PREACTIVATE_FEATURE not found" );
             schedule_protocol_features_wo_preactivation( { *preactivate_feature_digest } );
         };

#if 0
         schedule_preactivate_protocol_feature();
         produce_block();
         set_before_producer_authority_bios_contract();
         preactivate_all_builtin_protocol_features();
         produce_block();
         set_bios_contract();
#endif
   }

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
   action_result stake( std::string_view from, std::string_view to, const asset& net, const asset& cpu ) {
      return stake( account_name(from), account_name(to), net, cpu );
   }

   action_result stake( const account_name& acnt, const asset& net, const asset& cpu ) {
      return stake( acnt, acnt, net, cpu );
   }
   action_result stake( std::string_view acnt, const asset& net, const asset& cpu ) {
      return stake( account_name(acnt), net, cpu );
   }

   action_result stake_with_transfer( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action( name(from), "delegatebw"_n, mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
                          ("transfer", true )
      );
   }

   action_result stake_with_transfer( const account_name& acnt, const asset& net, const asset& cpu ) {
      return stake_with_transfer( acnt, acnt, net, cpu );
   }

   fc::variant get_total_stake( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, act, "userres"_n, act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "user_resources", data, abi_serializer::create_yield_function( abi_serializer_max_time ) );
   }

   action_result vote( const account_name& voter, const std::vector<account_name>& producers, const account_name& proxy = name(0) ) {
      return push_action(voter, "voteproducer"_n, mvo()
                         ("voter",     voter)
                         ("proxy",     proxy)
                         ("producers", producers));
   }
   action_result vote( const account_name& voter, const std::vector<account_name>& producers, std::string_view proxy ) {
      return vote( voter, producers, account_name(proxy) );
   }

   fc::variant get_voter_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, "voters"_n, act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "voter_info", data, abi_serializer::create_yield_function(abi_serializer_max_time) );
   }
   fc::variant get_voter_info(  std::string_view act ) {
      return get_voter_info( account_name(act) );
   }

   asset get_balance( const account_name& act ) {
      vector<char> data = get_row_by_account( "eosio.token"_n, act, "accounts"_n, name(symbol(CORE_SYMBOL).to_symbol_code().value) );
      return data.empty() ? asset(0, symbol(CORE_SYMBOL)) : token_abi_ser.binary_to_variant("account", data, abi_serializer::create_yield_function( abi_serializer_max_time ))["balance"].as<asset>();
   }

   asset get_rex_balance( const account_name& act ) const {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, "rexbal"_n, act );
      return data.empty() ? asset(0, symbol(SY(4, REX))) : abi_ser.binary_to_variant("rex_balance", data, abi_serializer::create_yield_function(abi_serializer_max_time))["rex_balance"].as<asset>();
   }

   asset get_rex_fund( const account_name& act ) const {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, "rexfund"_n, act );
      return data.empty() ? asset(0, symbol{CORE_SYM}) : abi_ser.binary_to_variant("rex_fund", data, abi_serializer::create_yield_function(abi_serializer_max_time))["balance"].as<asset>();
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

#if 0
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
#endif

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   void setup_rex_accounts( const std::vector<account_name>& accounts,
                            const asset& init_balance,
                            const asset& net = core_sym::from_string("80.0000"),
                            const asset& cpu = core_sym::from_string("80.0000"),
                            bool deposit_into_rex_fund = true ) {
      const asset nstake = core_sym::from_string("10.0000");
      const asset cstake = core_sym::from_string("10.0000");
      create_account_with_resources( "proxyaccount"_n, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
//      BOOST_REQUIRE_EQUAL( success(), push_action( "proxyaccount"_n, "regproxy"_n, mvo()("proxy", "proxyaccount")("isproxy", true) ) );
      for (const auto& a: accounts) {
         create_account_with_resources( a, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
#if 0
         transfer( config::system_account_name, a, init_balance + nstake + cstake, config::system_account_name );
         BOOST_REQUIRE_EQUAL( success(),                        stake( a, a, nstake, cstake) );
         BOOST_REQUIRE_EQUAL( success(),                        vote( a, { }, "proxyaccount"_n ) );
         BOOST_REQUIRE_EQUAL( init_balance,                     get_balance(a) );
         BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 REX"), get_rex_balance(a) );
         if (deposit_into_rex_fund) {
            BOOST_REQUIRE_EQUAL( success(),    deposit( a, init_balance ) );
            BOOST_REQUIRE_EQUAL( init_balance, get_rex_fund( a ) );
            BOOST_REQUIRE_EQUAL( 0,            get_balance( a ).get_amount() );
         }
#endif
      }
   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
};

BOOST_AUTO_TEST_SUITE(chain_plugin_tests)

BOOST_FIXTURE_TEST_CASE(account_results_total_resources_test, chain_plugin_tester) { try {
   preactivate();

   const account_name account = { "userres.test"_n };
   create_account(account);

   create_account("eosio.token"_n);
   set_code("eosio.token"_n, contracts::eosio_token_wasm() );
   set_abi("eosio.token"_n, contracts::eosio_token_abi().data() );

   produce_blocks( 10 );
   produce_block( fc::hours(3*24) );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( account ) );
   transfer( config::system_account_name, account, core_sym::from_string("1000.0000"), config::system_account_name );
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), get_balance( account ) );

   const asset nstake = core_sym::from_string("200.0000");
   const asset cstake = core_sym::from_string("100.0000");
   BOOST_REQUIRE_EQUAL( success(), stake( config::system_account_name, account, nstake, cstake) );

   auto total = get_total_stake(account);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("10.0000"), total["cpu_weight"].as<asset>());

    auto account_object = control->get_account(account);
    read_only::get_account_params params = { account_object.name };
    chain_apis::read_only plugin(*(control.get()), {}, fc::microseconds::maximum());
    read_only::get_account_results results = plugin.get_account(params);

	BOOST_TEST_REQUIRE(results.total_resources.get_type() != fc::variant::type_id::null_type);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(account_results_self_delegated_bandwidth_test, chain_plugin_tester) { try {
   const account_name account = { "bw.test"_n };
   create_account(account);

   create_account("eosio.token"_n);
   set_code("eosio.token"_n, contracts::eosio_token_wasm() );
   set_abi("eosio.token"_n, contracts::eosio_token_abi().data() );

    auto account_object = control->get_account(account);
    read_only::get_account_params params = { account_object.name };
    chain_apis::read_only plugin(*(control.get()), {}, fc::microseconds::maximum());
    read_only::get_account_results results = plugin.get_account(params);

	BOOST_TEST_REQUIRE(results.self_delegated_bandwidth.get_type() != fc::variant::type_id::null_type);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(account_results_refund_request_test, chain_plugin_tester) { try {
   const account_name account = { "refund.test"_n };
   create_account(account);

   create_account("eosio.token"_n);
   set_code("eosio.token"_n, contracts::eosio_token_wasm() );
   set_abi("eosio.token"_n, contracts::eosio_token_abi().data() );

    auto account_object = control->get_account(account);
    read_only::get_account_params params = { account_object.name };
    chain_apis::read_only plugin(*(control.get()), {}, fc::microseconds::maximum());
    read_only::get_account_results results = plugin.get_account(params);

	BOOST_TEST_REQUIRE(results.refund_request.get_type() != fc::variant::type_id::null_type);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(account_results_voter_info_test, chain_plugin_tester) { try {
   const account_name account_to = { "voter.test.1"_n };
   const account_name account_from = { "voter.test.2"_n };
   create_account(account_to);
   create_account(account_from);

   create_account("eosio.token"_n);
   set_code("eosio.token"_n, contracts::eosio_token_wasm() );
   set_abi("eosio.token"_n, contracts::eosio_token_abi().data() );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), get_balance( account_to ) );

   //eosio stakes with transfer flag

   transfer( config::system_account_name, account_from, core_sym::from_string("1000.0000"), config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake_with_transfer( account_from, account_to, core_sym::from_string("200.0000"), core_sym::from_string("100.0000") ) );

   //check that account_to has both bandwidth and voting power
   auto total = get_total_stake(account_to);
   BOOST_REQUIRE_EQUAL( core_sym::from_string("210.0000"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( core_sym::from_string("110.0000"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( account_to, core_sym::from_string("300.0000")), get_voter_info( account_to ) );

    auto account_object = control->get_account(account_to);
    read_only::get_account_params params = { account_object.name };
    chain_apis::read_only plugin(*(control.get()), {}, fc::microseconds::maximum());
    read_only::get_account_results results = plugin.get_account(params);

	BOOST_TEST_REQUIRE(results.voter_info.get_type() != fc::variant::type_id::null_type);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(account_results_rex_info_test, chain_plugin_tester) { //try {
   const int64_t ratio        = 10000;
   const asset   init_rent    = core_sym::from_string("20000.0000");
   const asset   init_balance = core_sym::from_string("1000.0000");
   const std::vector<account_name> accounts = { "rexinfo.test"_n };
   setup_rex_accounts( accounts, init_balance );

   create_account("eosio.token"_n);
   set_code("eosio.token"_n, contracts::eosio_token_wasm() );
   set_abi("eosio.token"_n, contracts::eosio_token_abi().data() );

   produce_blocks(2);
   produce_block(fc::days(5));

    auto account_object = control->get_account(accounts[0]);
    read_only::get_account_params params = { account_object.name };
    chain_apis::read_only plugin(*(control.get()), {}, fc::microseconds::maximum());
    read_only::get_account_results results = plugin.get_account(params);

	BOOST_TEST_REQUIRE(results.rex_info.get_type() != fc::variant::type_id::null_type);

#if 0
    {
    tester   chain;
    chain.create_account("account"_n);
    chain.setup_rex_accounts("account"_n);
    auto account_object = chain.control->get_account("account"_n);
    read_only::get_account_params params = { account_object.name };
    chain_apis::read_only plugin(*(chain.control.get()), {}, fc::microseconds::maximum());
    read_only::get_account_results results = plugin.get_account(params);
	}
#endif

} //FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
