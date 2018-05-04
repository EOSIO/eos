#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/global_property_object.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

class eosio_system_tester : public TESTER {
public:

   eosio_system_tester() {
      produce_blocks( 2 );

      create_accounts( { N(eosio.token) } );
      produce_blocks( 100 );

      set_code( N(eosio.token), eosio_token_wast );
      set_abi( N(eosio.token), eosio_token_abi );

      create_currency( N(eosio.token), config::system_account_name, asset::from_string("10000000000.0000 EOS") );
      issue(config::system_account_name,      "1000000000.0000 EOS");
      BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );

      set_code( config::system_account_name, eosio_system_wast );
      set_abi( config::system_account_name, eosio_system_abi );

      produce_blocks();

      create_account_with_resources( N(alice), N(eosio), asset::from_string("1.0000 EOS"), false );//{ N(alice), N(bob), N(carol) } );
      create_account_with_resources( N(bob), N(eosio), asset::from_string("0.4500 EOS"), false );//{ N(alice), N(bob), N(carol) } );
      create_account_with_resources( N(carol), N(eosio), asset::from_string("1.0000 EOS"), false );//{ N(alice), N(bob), N(carol) } );
      BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );
      // eosio pays it self for these...
      //BOOST_REQUIRE_EQUAL( asset::from_string("999999998.5000 EOS"), get_balance( "eosio" ) );

      produce_blocks();

      const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi);
      /*
      const global_property_object &gpo = control->get_global_properties();
      FC_ASSERT(0 < gpo.active_producers.producers.size(), "No producers");
      producer_name = (string)gpo.active_producers.producers.front().producer_name;
      */
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, asset ramfunds, bool multisig ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      if (multisig) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth =  authority( get_public_key( a, "owner" ) );
      }

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) ),
                                   .recovery = authority( get_public_key( a, "recovery" ) ),
                                });

      trx.actions.emplace_back( get_action( N(eosio), N(buyram), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("quant", ramfunds) )
                              );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), chain_id_type()  );
      return push_transaction( trx );
   }

   action_result buyram( const account_name& payer, account_name receiver, string eosin ) {
      return push_action( payer, N(buyram), mvo()( "payer",payer)("receiver",receiver)("quant",eosin) );
   }
   action_result buyrambytes( const account_name& payer, account_name receiver, uint32_t numbytes ) {
      return push_action( payer, N(buyrambytes), mvo()( "payer",payer)("receiver",receiver)("bytes",numbytes) );
   }

   action_result sellram( const account_name& account, uint32_t numbytes ) {
      return push_action( account, N(sellram), mvo()( "account", account)("bytes",numbytes) );
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = config::system_account_name;
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob) ? N(alice) : N(bob) );
   }

   action_result stake( const account_name& from, const account_name& to, const string& net, const string& cpu ) {
      return push_action( name(from), N(delegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
      );
   }

   action_result stake( const account_name& acnt, const string& net, const string& cpu ) {
      return stake( acnt, acnt, net, cpu );
   }

   action_result unstake( const account_name& from, const account_name& to, const string& net, const string& cpu ) {
      return push_action( name(from), N(undelegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("unstake_net_quantity", net)
                          ("unstake_cpu_quantity", cpu)
      );
   }

   action_result unstake( const account_name& acnt, const string& net, const string& cpu ) {
      return unstake( acnt, acnt, net, cpu );
   }

   static fc::variant_object producer_parameters_example( int n ) {
      return mutable_variant_object()
         ("max_block_net_usage", 10000000 + n )
         ("target_block_net_usage_pct", 10 + n )
         ("max_transaction_net_usage", 1000000 + n )
         ("base_per_transaction_net_usage", 100 + n)
         ("net_usage_leeway", 500 + n )
         ("context_free_discount_net_usage_num", 1 + n )
         ("context_free_discount_net_usage_den", 100 + n )
         ("max_block_cpu_usage", 10000000 + n )
         ("target_block_cpu_usage_pct", 10 + n )
         ("max_transaction_cpu_usage", 1000000 + n )
         ("base_per_transaction_cpu_usage", 100 + n)
         ("base_per_action_cpu_usage", 100 + n)
         ("base_setcode_cpu_usage", 100 + n)
         ("per_signature_cpu_usage", 100 + n)
         ("cpu_usage_leeway", 2048 + n )
         ("context_free_discount_cpu_usage_num", 1 + n )
         ("context_free_discount_cpu_usage_den", 100 + n )
         ("max_transaction_lifetime", 3600 + n)
         ("deferred_trx_expiration_window", 600 + n)
         ("max_transaction_delay", 10*86400+n)
         ("max_inline_action_size", 4096 + n)
         ("max_inline_action_depth", 4 + n)
         ("max_authority_depth", 6 + n)
         ("max_generated_transaction_count", 10 + n)
         ("max_storage_size", (n % 10 + 1) * 1024 * 1024)
         ("percent_of_max_inflation_rate", 50 + n)
         ("storage_reserve_ratio", 100 + n);
   }

   action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
      return push_action( acnt, N(regproducer), mvo()
                          ("producer",  acnt )
                          ("producer_key", fc::raw::pack( get_public_key( acnt, "active" ) ) )
                          ("url", "" )
      );
   }

   uint32_t last_block_time() const {
      return time_point_sec( control->head_block_time() ).sec_since_epoch();
   }

   asset get_balance( const account_name& act ) {
      //return get_currency_balance( config::system_account_name, symbol(SY(4,EOS)), act );
      //temporary code. current get_currency_balancy uses table name N(accounts) from currency.h
      //generic_currency table name is N(account).
      const auto& db  = control->db();
      const auto* tbl = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(eosio.token), act, N(accounts)));
      share_type result = 0;

      // the balance is implied to be 0 if either the table or row does not exist
      if (tbl) {
         const auto *obj = db.find<key_value_object, by_scope_primary>(boost::make_tuple(tbl->id, symbol(SY(4,EOS)).to_symbol_code()));
         if (obj) {
            // balance is the first field in the serialization
            fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
            fc::raw::unpack(ds, result);
         }
      }
      return asset( result, symbol(SY(4,EOS)) );
   }

   fc::variant get_total_stake( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, act, N(userres), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "total_resources", data );
   }

   fc::variant get_voter_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(voters), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "voter_info", data );
   }

   fc::variant get_producer_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(producers), act );
      return abi_ser.binary_to_variant( "producer_info", data );
   }

   void create_currency( name contract, name manager, asset maxsupply ) {
      auto act =  mutable_variant_object()
         ("issuer",       manager )
         ("maximum_supply", maxsupply )
         ("can_freeze", 0)
         ("can_recall", 0)
         ("can_whitelist", 0);

      base_tester::push_action(contract, N(create), contract, act );
   }

   void issue( name to, const string& amount, name manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(issue), manager, mutable_variant_object()
                                ("to",      to )
                                ("quantity", asset::from_string(amount) )
                                ("memo", "")
                                );
   }
   void transfer( name from, name to, const string& amount, name manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(transfer), manager, mutable_variant_object()
                                ("from",    from)
                                ("to",      to )
                                ("quantity", asset::from_string(amount) )
                                ("memo", "")
                                );
   }


   abi_serializer abi_ser;
};

fc::mutable_variant_object voter( account_name acct ) {
   return mutable_variant_object()
      ("owner", acct)
      ("proxy", name(0).to_string())
      ("is_proxy", 0)
      ("staked", asset())
      ("unstaking", asset())
      ("unstake_per_week", asset())
      ("proxied_votes", 0)
      ("producers", variants() )
      ("deferred_trx_id", 0)
      ("last_unstake", 0);
}

fc::mutable_variant_object voter( account_name acct, const string& vote_stake ) {
   return voter( acct )( "staked", asset::from_string( vote_stake ) );
}

fc::mutable_variant_object proxy( account_name acct ) {
   return voter( acct )( "is_proxy", 1 );
}

inline uint64_t M( const string& eos_str ) {
   return asset::from_string( eos_str ).amount;
}

BOOST_AUTO_TEST_SUITE(eosio_system_tests)

BOOST_FIXTURE_TEST_CASE( stake_unstake, eosio_system_tester ) try {
   //issue( "eosio", "1000.0000 EOS", config::system_account_name );

   BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS"), get_balance( "alice" ) );
   transfer( "eosio", "alice", "1000.0000 EOS", "eosio" );
   BOOST_REQUIRE_EQUAL( asset::from_string("999999000.0000 EOS"), get_balance( "eosio" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice" ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice", "200.0000 EOS", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "alice", "200.0000 EOS", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice" ) );
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "alice", "200.0000 EOS", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice" ) );

   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice" ) );
   //after 3 days funds should be released
   produce_block( fc::hours(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice" ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "200.0000 EOS", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice", "bob", "200.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyrambytes( "alice", "bob", 100 ) );
   BOOST_REQUIRE_EQUAL( success(), sellram( "bob", 100 ) );
   BOOST_REQUIRE_EQUAL( success(), buyrambytes( "alice", "bob", 10000 ) );



   auto total = get_total_stake( "alice" );
   idump((total));
   return;

   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS").amount, total["net_weight"].as<asset>().amount );
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS").amount, total["cpu_weight"].as<asset>().amount );

   REQUIRE_MATCHING_OBJECT( voter( "alice", "300.0000 EOS"), get_voter_info( "alice" ) );

   auto bytes = total["storage_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );

   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS"), get_balance( "alice" ) );

   //unstake
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "200.0000 EOS", "100.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS").amount, total["net_weight"].as<asset>().amount);
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS").amount, total["cpu_weight"].as<asset>().amount);


   REQUIRE_MATCHING_OBJECT( voter( "alice", "0.0000 EOS" ), get_voter_info( "alice" ) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS"), get_balance( "alice" ) );

   //after 2 days balance should not be available yet
   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS"), get_balance( "alice" ) );
   //after 3 days funds should be released
   produce_block( fc::hours(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice" ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( fail_without_auth, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice", "2000.0000 EOS", "1000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "10.0000 EOS", "10.0000 EOS" ) );

   BOOST_REQUIRE_EQUAL( error("missing authority of alice"),
                        push_action( N(alice), N(delegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "bob")
                                    ("stake_net_quantity", "10.0000 EOS")
                                    ("stake_cpu_quantity", "10.0000 EOS")
                                    ("stake_storage_quantity", "10.0000 EOS"),
                                    false
                        )
   );

   BOOST_REQUIRE_EQUAL( error("missing authority of alice"),
                        push_action(N(alice), N(undelegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "bob")
                                    ("unstake_net_quantity", "200.0000 EOS")
                                    ("unstake_cpu_quantity", "100.0000 EOS")
                                    ,false
                        )
   );
   //REQUIRE_MATCHING_OBJECT( , get_voter_info( "alice" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( stake_negative, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice", "-0.0001 EOS", "0.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice", "0.0000 EOS", "-0.0001 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice", "00.0000 EOS", "00.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice", "0.0000 EOS", "00.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( true, get_voter_info( "alice" ).is_null() );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unstake_negative, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "200.0001 EOS", "100.0001 EOS" ) );

   auto total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0001 EOS"), total["net_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice", "300.0002 EOS" ), get_voter_info( "alice" ) );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice", "bob", "-1.0000 EOS", "0.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice", "bob", "0.0000 EOS", "-1.0000 EOS" )
   );

   //unstake all zeros
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice", "bob", "0.0000 EOS", "0.0000 EOS" )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unstake_more_than_at_stake, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "200.0000 EOS", "100.0000 EOS" ) );

   auto total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS"), total["cpu_weight"].as<asset>());

   BOOST_REQUIRE_EQUAL( asset::from_string("300.0000 EOS"), get_balance( "alice" ) );

   //trying to unstake more net bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        unstake( "alice", "200.0001 EOS", "0.0000 EOS", 0 )
   );

   //trying to unstake more cpu bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        unstake( "alice", "000.0000 EOS", "100.0001 EOS", 0 )
   );

   //check that nothing has changed
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("300.0000 EOS"), get_balance( "alice" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( delegate_to_another_user, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake ( "alice", "bob", "200.0000 EOS", "100.0000 EOS" ) );

   auto total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("620.0000 EOS"), get_balance( "alice" ) );
   //all voting power goes to alice
   REQUIRE_MATCHING_OBJECT( voter( "alice", "300.0000 EOS" ), get_voter_info( "alice" ) );
   //but not to bob
   BOOST_REQUIRE_EQUAL( true, get_voter_info( "bob" ).is_null() );

   //bob should not be able to unstake what was staked by alice
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: unable to find key"),
                        unstake( "bob", "0.0000 EOS", "10.0000 EOS" )
   );

   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "bob", "20.0000 EOS", "10.0000 EOS" ) );
   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("220.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("962.0000 EOS"), get_balance( "carol" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol", "30.0000 EOS" ), get_voter_info( "carol" ) );

   //alice should not be able to unstake money staked by carol
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        unstake( "alice", "bob", "2001.0000 EOS", "1.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        unstake( "alice", "bob", "1.0000 EOS", "101.0000 EOS" )
   );

   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("220.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());
   //balance should not change after unsuccessfull attempts to unstake
   BOOST_REQUIRE_EQUAL( asset::from_string("620.0000 EOS"), get_balance( "alice" ) );
   //voting power too
   REQUIRE_MATCHING_OBJECT( voter( "alice", "300.0000 EOS" ), get_voter_info( "alice" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol", "30.0000 EOS" ), get_voter_info( "carol" ) );
   BOOST_REQUIRE_EQUAL( true, get_voter_info( "bob" ).is_null() );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( stake_unstake_separate, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice" ) );

   //everything at once
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "10.0000 EOS", "20.0000 EOS" ) );
   auto total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["cpu_weight"].as<asset>());

   //cpu
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "100.0000 EOS", "0.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["cpu_weight"].as<asset>());

   //net
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "0.0000 EOS", "200.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("220.0000 EOS"), total["cpu_weight"].as<asset>());

   //unstake cpu
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "100.0000 EOS", "0.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("220.0000 EOS"), total["cpu_weight"].as<asset>());

   //unstake net
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "0.0000 EOS", "200.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["cpu_weight"].as<asset>());
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( adding_stake_partial_unstake, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "200.0000 EOS", "100.0000 EOS" ) );

   auto total = get_total_stake( "bob" );
   REQUIRE_MATCHING_OBJECT( voter( "alice", "300.0000 EOS" ), get_voter_info( "alice" ) );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "100.0000 EOS", "50.0000 EOS" ) );

   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("300.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("150.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice", "450.0000 EOS" ), get_voter_info( "alice" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("430.0000 EOS"), get_balance( "alice" ) );

   //unstake a share
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "bob", "150.0000 EOS", "75.0000 EOS" ) );

   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("150.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("75.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice", "225.0000 EOS" ), get_voter_info( "alice" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "bob", "50.0000 EOS", "25.0000 EOS" ) );
   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("50.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice", "150.0000 EOS" ), get_voter_info( "alice" ) );

   //combined amount should be available only in 3 days
   produce_block( fc::days(2) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("430.0000 EOS"), get_balance( "alice" ) );
   produce_block( fc::days(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("790.0000 EOS"), get_balance( "alice" ) );

} FC_LOG_AND_RETHROW()


// Tests for voting
BOOST_FIXTURE_TEST_CASE( producer_register_unregister, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( fc::crypto::public_key( std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV") ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );

   auto info = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( "alice", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_uint64() );
   REQUIRE_MATCHING_OBJECT( params, info["prefs"] );
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(info["packed_key"]) );


   //call regproducer again to change parameters
   fc::variant params2 = producer_parameters_example(2);

   vector<char> key2 = fc::raw::pack( fc::crypto::public_key( std::string("EOSR16EPHFSKVYHBjQgxVGQPrwCxTg7BbZ69H9i4gztN9deKTEXYne4") ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key2 )
                                               ("prefs", params2)
                        )
   );

   info = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( "alice", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_uint64() );
   REQUIRE_MATCHING_OBJECT( params2, info["prefs"] );
   BOOST_REQUIRE_EQUAL( string(key2.begin(), key2.end()), to_string(info["packed_key"]) );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(unregprod), mvo()
                                               ("producer",  "alice")
                        )
   );
   info = get_producer_info( "alice" );
   //key should be empty
   BOOST_REQUIRE_EQUAL( true, to_string(info["packed_key"]).empty() );
   //everything else should stay the same
   BOOST_REQUIRE_EQUAL( "alice", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_uint64() );
   REQUIRE_MATCHING_OBJECT( params2, info["prefs"] );

   //unregister bob who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer not found" ),
                        push_action( N(bob), N(unregprod), mvo()
                                     ("producer",  "bob")
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_producer, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( N(alice), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );
   auto prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( "alice", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, prod["total_votes"].as_uint64() );
   REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(prod["packed_key"]) );

   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   issue( "carol", "3000.0000 EOS",  config::system_account_name );

   //bob makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "11.0000 EOS", "0.1111 EOS", "0.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("1988.8889 EOS"), get_balance( "bob" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob", "11.1111 EOS" ), get_voter_info( "bob" ) );

   //bob votes for alice
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob), N(voteproducer), mvo()
                                               ("voter",  "bob")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice) } )
                        )
   );

   //check that producer parameters stay the same after voting
   prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 111111, prod["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "alice", prod["owner"].as_string() );
   REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(prod["packed_key"]) );

   //carol makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "22.0000 EOS", "0.2222 EOS", "0.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol", "22.2222 EOS" ), get_voter_info( "carol" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("2977.7778 EOS"), get_balance( "carol" ) );
   //carol votes for alice
   BOOST_REQUIRE_EQUAL( success(), push_action(N(carol), N(voteproducer), mvo()
                                               ("voter",  "carol")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice) } )
                        )
   );
   //new stake votes be added to alice's total_votes
   prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 333333, prod["total_votes"].as_uint64() );

   //bob increases his stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "55.0000 EOS", "0.5555 EOS", "0.0000 EOS" ) );
   //should increase alice's total_votes
   prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 888888, prod["total_votes"].as_uint64() );

   //carol unstakes part of the stake
   BOOST_REQUIRE_EQUAL( success(), unstake( "carol", "2.0000 EOS", "0.0002 EOS", 0 ) );
   //should decrease alice's total_votes
   prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 868886, prod["total_votes"].as_uint64() );

   //bob revokes his vote
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(voteproducer), mvo()
                                               ("voter",  "bob")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>() )
                        )
   );
   //should decrease alice's total_votes
   prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 202220, prod["total_votes"].as_uint64() );
   //but eos should still be at stake
   BOOST_REQUIRE_EQUAL( asset::from_string("1933.3334 EOS"), get_balance( "bob" ) );

   //carol unstakes rest of eos
   BOOST_REQUIRE_EQUAL( success(), unstake( "carol", "20.0000 EOS", "0.2220 EOS", 0 ) );
   //should decrease alice's total_votes to zero
   prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 0, prod["total_votes"].as_uint64() );
   //check that the producer parameters stay the same after all
   BOOST_REQUIRE_EQUAL( "alice", prod["owner"].as_string() );
   REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(prod["packed_key"]) );
   //carol should receive funds in 3 days
   produce_block( fc::days(3) );
   produce_block();
   BOOST_REQUIRE_EQUAL( asset::from_string("3000.0000 EOS"), get_balance( "carol" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unregistered_producer_voting, eosio_system_tester ) try {
   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "13.0000 EOS", "0.5791 EOS", "0.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob", "13.5791 EOS" ), get_voter_info( "bob" ) );

   //bob should not be able to vote for alice who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer is not registered" ),
                        push_action( N(bob), N(voteproducer), mvo()
                                    ("voter",  "bob")
                                    ("proxy", name(0).to_string() )
                                    ("producers", vector<account_name>{ N(alice) } )
                        )
   );

   //alice registers as a producer
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( N(alice), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );
   //and then unregisters
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(unregprod), mvo()
                                               ("producer",  "alice")
                        )
   );
   //key should be empty
   auto prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( true, to_string(prod["packed_key"]).empty() );

   //bob should not be able to vote for alice who is an unregistered producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer is not currently registered" ),
                        push_action( N(bob), N(voteproducer), mvo()
                                    ("voter",  "bob")
                                    ("proxy", name(0).to_string() )
                                    ("producers", vector<account_name>{ N(alice) } )
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( more_than_30_producer_voting, eosio_system_tester ) try {
   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "13.0000 EOS", "0.5791 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob", "13.5791 EOS" ), get_voter_info( "bob" ) );

   //bob should not be able to vote for alice who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: attempt to vote for too many producers" ),
                        push_action( N(bob), N(voteproducer), mvo()
                                     ("voter",  "bob")
                                     ("proxy", name(0).to_string() )
                                     ("producers", vector<account_name>(31, N(alice)) )
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_same_producer_30_times, eosio_system_tester ) try {
   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "50.0000 EOS", "50.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob", "100.0000 EOS" ), get_voter_info( "bob" ) );

   //alice becomes a producer
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( N(alice), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );

   //bob should not be able to vote for alice who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer votes must be unique and sorted" ),
                        push_action( N(bob), N(voteproducer), mvo()
                                     ("voter",  "bob")
                                     ("proxy", name(0).to_string() )
                                     ("producers", vector<account_name>(30, N(alice)) )
                        )
   );

   auto prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 0, prod["total_votes"].as_uint64() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( producer_keep_votes, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( N(alice), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );

   //bob makes stake
   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "13.0000 EOS", "0.5791 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob", "13.5791 EOS" ), get_voter_info( "bob" ) );

   //bob votes for alice
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob), N(voteproducer), mvo()
                                               ("voter",  "bob")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice) } )
                        )
   );

   auto prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 135791, prod["total_votes"].as_uint64() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(unregprod), mvo()
                                               ("producer",  "alice")
                        )
   );
   prod = get_producer_info( "alice" );
   //key should be empty
   BOOST_REQUIRE_EQUAL( true, to_string(prod["packed_key"]).empty() );
   //check parameters just in case
   REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   //votes should stay the same
   BOOST_REQUIRE_EQUAL( 135791, prod["total_votes"].as_uint64() );

   //regtister the same producer again
   params = producer_parameters_example(2);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );
   prod = get_producer_info( "alice" );
   //votes should stay the same
   BOOST_REQUIRE_EQUAL( 135791, prod["total_votes"].as_uint64() );

   //change parameters
   params = producer_parameters_example(3);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );
   prod = get_producer_info( "alice" );
   //votes should stay the same
   BOOST_REQUIRE_EQUAL( 135791, prod["total_votes"].as_uint64() );
   //check parameters just in case
   REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(prod["packed_key"]) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_two_producers, eosio_system_tester ) try {
   //alice becomes a producer
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( N(alice), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );
   //bob becomes a producer
   params = producer_parameters_example(2);
   key = fc::raw::pack( get_public_key( N(bob), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(regproducer), mvo()
                                               ("producer",  "bob")
                                               ("producer_key", key )
                                               ("prefs", params)
                        )
   );

   //carol votes for alice and bob
   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "15.0005 EOS", "5.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(carol), N(voteproducer), mvo()
                                               ("voter",  "carol")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice), N(bob) } )
                        )
   );

   auto alice_info = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 200005, alice_info["total_votes"].as_uint64() );
   auto bob_info = get_producer_info( "bob" );
   BOOST_REQUIRE_EQUAL( 200005, bob_info["total_votes"].as_uint64() );

   //carol votes for alice (but revokes vote for bob)
   BOOST_REQUIRE_EQUAL( success(), push_action(N(carol), N(voteproducer), mvo()
                                               ("voter",  "carol")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice) } )
                        )
   );

   alice_info = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 200005, alice_info["total_votes"].as_uint64() );
   bob_info = get_producer_info( "bob" );
   BOOST_REQUIRE_EQUAL( 0, bob_info["total_votes"].as_uint64() );

   //alice votes for herself and bob
   issue( "alice", "2.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "1.0000 EOS", "1.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(voteproducer), mvo()
                                               ("voter",  "alice")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice), N(bob) } )
                        )
   );

   alice_info = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( 220005, alice_info["total_votes"].as_uint64() );
   bob_info = get_producer_info( "bob" );
   BOOST_REQUIRE_EQUAL( 20000, bob_info["total_votes"].as_uint64() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_register_unregister_keeps_stake, eosio_system_tester ) try {
   //register proxy by first action for this user ever
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(regproxy), mvo()
                                               ("proxy",  "alice")
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" ), get_voter_info( "alice" ) );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(unregproxy), mvo()
                                               ("proxy",  "alice")
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice" ), get_voter_info( "alice" ) );

   //stake and then register as a proxy
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "200.0002 EOS", "100.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(regproxy), mvo()
                                               ("proxy",  "bob")
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "bob" )( "staked", "300.0003 EOS" ), get_voter_info( "bob" ) );
   //unrgister and check that stake is still in place
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(unregproxy), mvo()
                                               ("proxy",  "bob")
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "bob", "300.0003 EOS" ), get_voter_info( "bob" ) );

   //register as a proxy and then stake
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(regproxy), mvo()
                                               ("proxy",  "carol")
                        )
   );
   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "246.0002 EOS", "531.0001 EOS" ) );
   //check that both proxy flag and stake a correct
   REQUIRE_MATCHING_OBJECT( proxy( "carol" )( "staked", "777.0003 EOS" ), get_voter_info( "carol" ) );

   //unregister
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(unregproxy), mvo()
                                                ("proxy",  "carol")
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "carol", "777.0003 EOS" ), get_voter_info( "carol" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_stake_unstake_keeps_proxy_flag, eosio_system_tester ) try {
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                               ("proxy",  "alice")
                        )
   );
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" ), get_voter_info( "alice" ) );

   //stake
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "100.0000 EOS", "50.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "staked", "150.0000 EOS" ), get_voter_info( "alice" ) );

   //stake more
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "30.0000 EOS", "20.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )("staked", "200.0000 EOS" ), get_voter_info( "alice" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "65.0000 EOS", "35.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )("staked", "100.0000 EOS" ), get_voter_info( "alice" ) );

   //unstake the rest
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "65.0000 EOS", "35.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "staked", "0.0000 EOS" ), get_voter_info( "alice" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_actions_affect_producers, eosio_system_tester ) try {
   create_accounts( {  N(producer1), N(producer2), N(producer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer3", 3) );

   //register as a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                        )
   );

   //accumulate proxied votes
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob), N(voteproducer), mvo()
                                               ("voter",  "bob")
                                               ("proxy", "alice" )
                                               ("producers", vector<account_name>() )
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "proxied_votes", 1500003 ), get_voter_info( "alice" ) );

   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(voteproducer), mvo()
                                               ("voter",  "alice")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer1), N(producer2) } )
                        )
   );
   BOOST_REQUIRE_EQUAL( 1500003, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 1500003, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //vote for another producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(voteproducer), mvo()
                                               ("voter",  "alice")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer1), N(producer3) } )
                        )
   );
   BOOST_REQUIRE_EQUAL( 1500003, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 1500003, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(unregproxy), mvo()
                                                ("proxy",  "alice")
                        )
   );
   //REQUIRE_MATCHING_OBJECT( voter( "alice" )( "proxied_votes", 1500003 ), get_voter_info( "alice" ) );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //register proxy again
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                        )
   );
   BOOST_REQUIRE_EQUAL( 1500003, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 1500003, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //stake increase by proxy itself affects producers
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "30.0001 EOS", "20.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( 2000005, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 2000005, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //stake decrease by proxy itself affects producers
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "10.0001 EOS", "10.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( 1800003, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 1800003, get_producer_info( "producer3" )["total_votes"].as_uint64() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producer_pay, eosio_system_tester) try {
   issue( "alice", "100.0000 EOS", config::system_account_name);
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS"), get_balance( "alice" ) );

   fc::variant params = producer_parameters_example(50);
   vector<char> key = fc::raw::pack(get_public_key(N(alice), "active"));

   // 1 block produced

   BOOST_REQUIRE_EQUAL(success(), push_action(N(alice), N(regproducer), mvo()
                                              ("producer",  "alice")
                                              ("producer_key", key )
                                              ("prefs", params)
                                              )
                       );

   auto prod = get_producer_info( N(alice) );

   BOOST_REQUIRE_EQUAL("alice", prod["owner"].as_string());
   BOOST_REQUIRE_EQUAL(0, prod["total_votes"].as_uint64());
   REQUIRE_EQUAL_OBJECTS(params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL(string(key.begin(), key.end()), to_string(prod["packed_key"]));


   issue("bob", "2000.0000 EOS", config::system_account_name);
   BOOST_REQUIRE_EQUAL( asset::from_string("2000.0000 EOS"), get_balance( "bob" ) );

   // bob makes stake
   // 1 block produced

   BOOST_REQUIRE_EQUAL(success(), stake("bob", "11.0000 EOS", "10.1111 EOS"));

   // bob votes for alice
   // 1 block produced
   BOOST_REQUIRE_EQUAL(success(), push_action(N(bob), N(voteproducer), mvo()
                                              ("voter",  "bob")
                                              ("proxy", name(0).to_string())
                                              ("producers", vector<account_name>{ N(alice) })
                                              )
                       );

   produce_blocks(10);
   prod = get_producer_info("alice");
   BOOST_REQUIRE(prod["per_block_payments"].as_uint64() > 0);
   BOOST_REQUIRE_EQUAL(success(), push_action(N(alice), N(claimrewards), mvo()
                                              ("owner",     "alice")
                                              )
                       );
   prod = get_producer_info("alice");
   BOOST_REQUIRE_EQUAL(0, prod["per_block_payments"].as_uint64());

 } FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( voters_actions_affect_proxy_and_producers, eosio_system_tester ) try {
   create_accounts( { N(donald), N(producer1), N(producer2), N(producer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer3", 3) );

   //alice becomes a producer
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" ), get_voter_info( "alice" ) );

   //alice makes stake and votes
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "30.0001 EOS", "20.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(voteproducer), mvo()
                                               ("voter",  "alice")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer1), N(producer2) } )
                        )
   );
   BOOST_REQUIRE_EQUAL( 500002, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 500002, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   BOOST_REQUIRE_EQUAL( success(), push_action( N(donald), N(regproxy), mvo()
                                                ("proxy",  "donald")
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "donald" ), get_voter_info( "donald" ) );

   //bob chooses alice as a proxy
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(voteproducer), mvo()
                                                ("voter",  "bob")
                                                ("proxy", "alice" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_REQUIRE_EQUAL( 1500003, get_voter_info( "alice" )["proxied_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 2000005, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 2000005, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //carol chooses alice as a proxy
   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "30.0001 EOS", "20.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(voteproducer), mvo()
                                                ("voter",  "carol")
                                                ("proxy", "alice" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_REQUIRE_EQUAL( 2000005, get_voter_info( "alice" )["proxied_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 2500007, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 2500007, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );


   //proxied voter carol increases stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "50.0000 EOS", "70.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( 3200005, get_voter_info( "alice" )["proxied_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 3700007, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 3700007, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //proxied voter bob decreases stake
   BOOST_REQUIRE_EQUAL( success(), unstake( "bob", "50.0001 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( 2200003, get_voter_info( "alice" )["proxied_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 2700005, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 2700005, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //proxied voter carol chooses another proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(voteproducer), mvo()
                                                ("voter",  "carol")
                                                ("proxy", "donald" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_REQUIRE_EQUAL( 500001, get_voter_info( "alice" )["proxied_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 1700002, get_voter_info( "donald" )["proxied_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 1000003, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 1000003, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );

   //bob switches to direct voting and votes for one of the same producers, but not for another one
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(voteproducer), mvo()
                                                ("voter",  "bob")
                                                ("proxy", "")
                                                ("producers", vector<account_name>{ N(producer2) } )
                        )
   );
   BOOST_REQUIRE_EQUAL( 0, get_voter_info( "alice" )["proxied_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL(  500002, get_producer_info( "producer1" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 1000003, get_producer_info( "producer2" )["total_votes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_uint64() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_both_proxy_and_producers, eosio_system_tester ) try {
   //alice becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" ), get_voter_info( "alice" ) );

   //carol becomes a producer
   BOOST_REQUIRE_EQUAL( success(), regproducer( "carol", 1) );

   //bob chooses alice as a proxy
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: cannot vote for producers and proxy at same time"),
                        push_action( N(bob), N(voteproducer), mvo()
                                     ("voter",  "bob")
                                     ("proxy", "alice" )
                                     ("producers", vector<account_name>{ N(carol) } )
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( select_invalid_proxy, eosio_system_tester ) try {
   //accumulate proxied votes
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "100.0002 EOS", "50.0001 EOS" ) );

   //selecting account not registered as a proxy
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: proxy not found" ),
                        push_action(N(bob), N(voteproducer), mvo()
                                    ("voter",  "bob")
                                    ("proxy", "alice" )
                                    ("producers", vector<account_name>() )
                        )
   );

   //selecting not existing account as a proxy
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: proxy not found" ),
                        push_action(N(bob), N(voteproducer), mvo()
                                    ("voter",  "bob")
                                    ("proxy", "notexist" )
                                    ("producers", vector<account_name>() )
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( double_register_unregister_proxy_keeps_votes, eosio_system_tester ) try {
   //alice becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                        )
   );
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "5.0000 EOS", "5.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "staked", "10.0000 EOS" ), get_voter_info( "alice" ) );

   //bob stakes and selects alice as a proxy
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(voteproducer), mvo()
                                                ("voter",  "bob")
                                                ("proxy", "alice" )
                                                ("producers", vector<account_name>() )
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "proxied_votes", 1500003 )( "staked", "10.0000 EOS" ), get_voter_info( "alice" ) );

   //double regestering should fail without affecting total votes and stake
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: account is already a proxy" ),
                        push_action( N(alice), N(regproxy), mvo()
                                     ("proxy",  "alice")
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "proxied_votes", 1500003 )( "staked", "10.0000 EOS" ), get_voter_info( "alice" ) );

   //uregister
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(unregproxy), mvo()
                                                ("proxy",  "alice")
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice" )( "proxied_votes", 1500003 )( "staked", "10.0000 EOS" ), get_voter_info( "alice" ) );

   //double unregistering should not affect proxied_votes and stake
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: account is not a proxy" ),
                        push_action( N(alice), N(unregproxy), mvo()
                                     ("proxy",  "alice")
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice" )( "proxied_votes", 1500003 )( "staked", "10.0000 EOS" ), get_voter_info( "alice" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_cannot_use_another_proxy, eosio_system_tester ) try {
   //alice becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                        )
   );

   //bob becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(regproxy), mvo()
                                                ("proxy",  "bob")
                        )
   );
   //proxy should not be able to use a proxy
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: account registered as a proxy is not allowed to use a proxy" ),
                        push_action( N(bob), N(voteproducer), mvo()
                                     ("voter",  "bob")
                                     ("proxy", "alice" )
                                     ("producers", vector<account_name>() )
                        )
   );

   //voter that uses a proxy should not be allowed to become a proxy
   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(voteproducer), mvo()
                                                ("voter",  "carol")
                                                ("proxy", "alice" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: account that uses a proxy is not allowed to become a proxy" ),
                        push_action( N(carol), N(regproxy), mvo()
                                     ("proxy",  "carol")
                        )
   );

   //proxy should not be able to use itself as a proxy
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: account registered as a proxy is not allowed to use a proxy" ),
                        push_action( N(bob), N(voteproducer), mvo()
                                     ("voter",  "bob")
                                     ("proxy", "bob" )
                                     ("producers", vector<account_name>() )
                        )
   );

} FC_LOG_AND_RETHROW()

fc::mutable_variant_object config_to_variant( const eosio::chain::chain_config& config ) {
   return mutable_variant_object()
      ( "max_block_net_usage", config.max_block_net_usage )
      ( "target_block_net_usage_pct", config.target_block_net_usage_pct )
      ( "max_transaction_net_usage", config.max_transaction_net_usage )
      ( "base_per_transaction_net_usage", config.base_per_transaction_net_usage )
      ( "context_free_discount_net_usage_num", config.context_free_discount_net_usage_num )
      ( "context_free_discount_net_usage_den", config.context_free_discount_net_usage_den )
      ( "max_block_cpu_usage", config.max_block_cpu_usage )
      ( "target_block_cpu_usage_pct", config.target_block_cpu_usage_pct )
      ( "max_transaction_cpu_usage", config.max_transaction_cpu_usage )
      ( "base_per_transaction_cpu_usage", config.base_per_transaction_cpu_usage )
      ( "base_per_action_cpu_usage", config.base_per_action_cpu_usage )
      ( "base_setcode_cpu_usage", config.base_setcode_cpu_usage )
      ( "per_signature_cpu_usage", config.per_signature_cpu_usage )
      ( "context_free_discount_cpu_usage_num", config.context_free_discount_cpu_usage_num )
      ( "context_free_discount_cpu_usage_den", config.context_free_discount_cpu_usage_den )
      ( "max_transaction_lifetime", config.max_transaction_lifetime )
      ( "deferred_trx_expiration_window", config.deferred_trx_expiration_window )
      ( "max_transaction_delay", config.max_transaction_delay )
      ( "max_inline_action_size", config.max_inline_action_size )
      ( "max_inline_action_depth", config.max_inline_action_depth )
      ( "max_authority_depth", config.max_authority_depth )
      ( "max_generated_transaction_count", config.max_generated_transaction_count );
}

BOOST_FIXTURE_TEST_CASE( elect_producers_and_parameters, eosio_system_tester ) try {
   create_accounts( {  N(producer1), N(producer2), N(producer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer3", 3) );

   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "100.0000 EOS", "50.0000 EOS" ) );
   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(voteproducer), mvo()
                                               ("voter",  "alice")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer1) } )
                        )
   );
   produce_blocks(50);
   auto producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 1, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("producer1"), producer_keys[0].producer_name );

   auto config = config_to_variant( control->get_global_properties().configuration );
   auto prod1_config = testing::filter_fields( config, producer_parameters_example( 1 ) );
   REQUIRE_EQUAL_OBJECTS(prod1_config, config);

   // elect 2 producers
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "200.0000 EOS", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob), N(voteproducer), mvo()
                                               ("voter",  "bob")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer2) } )
                        )
   );
   produce_blocks(50);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 2, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("producer2"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("producer1"), producer_keys[1].producer_name );
   config = config_to_variant( control->get_global_properties().configuration );
   auto prod2_config = testing::filter_fields( config, producer_parameters_example( 2 ) );
   REQUIRE_EQUAL_OBJECTS(prod2_config, config);

   // elect 3 producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob), N(voteproducer), mvo()
                                               ("voter",  "bob")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer2), N(producer3) } )
                        )
   );
   produce_blocks(50);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 3, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("producer3"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("producer2"), producer_keys[1].producer_name );
   BOOST_REQUIRE_EQUAL( name("producer1"), producer_keys[2].producer_name );
   config = config_to_variant( control->get_global_properties().configuration );
   REQUIRE_EQUAL_OBJECTS(prod2_config, config);

   //back to 2 producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob), N(voteproducer), mvo()
                                               ("voter",  "bob")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer3) } )
                        )
   );
   produce_blocks(100);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 2, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("producer3"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("producer1"), producer_keys[1].producer_name );
   config = config_to_variant( control->get_global_properties().configuration );
   auto prod3_config = testing::filter_fields( config, producer_parameters_example( 3 ) );
   REQUIRE_EQUAL_OBJECTS(prod3_config, config);

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
