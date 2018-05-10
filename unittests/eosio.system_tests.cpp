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

      create_account_with_resources( N(alice), N(eosio), asset::from_string("1.0000 EOS"), false );
      create_account_with_resources( N(bob), N(eosio), asset::from_string("0.4500 EOS"), false );
      create_account_with_resources( N(carol), N(eosio), asset::from_string("1.0000 EOS"), false );

      BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );

      // eosio pays it self for these...
      //BOOST_REQUIRE_EQUAL( asset::from_string("999999998.5000 EOS"), get_balance( "eosio" ) );

      produce_blocks();

      {
         const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         abi_ser.set_abi(abi);
      }
      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.token) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         token_abi_ser.set_abi(abi);
      }
   }

   void create_accounts_with_resources( vector<account_name> accounts, account_name creator = N(eosio) ) {
      for( auto a : accounts ) {
         create_account_with_resources( a, creator );
      }
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator ) {
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

      trx.actions.emplace_back( get_action( N(eosio), N(buyrambytes), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("bytes", 8000) )
                              );
      trx.actions.emplace_back( get_action( N(eosio), N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", "10.0000 EOS" )
                                            ("stake_cpu_quantity", "10.0000 EOS" )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), chain_id_type()  );
      return push_transaction( trx );
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, asset ramfunds, bool multisig,
                                                        asset net = asset::from_string("10.0000 EOS"), asset cpu = asset::from_string("10.0000 EOS") ) {
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
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action( N(eosio), N(buyram), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("quant", ramfunds) )
                              );

      trx.actions.emplace_back( get_action( N(eosio), N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", net )
                                            ("stake_cpu_quantity", cpu )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), chain_id_type()  );
      return push_transaction( trx );
   }

   transaction_trace_ptr setup_producer_accounts() {
      std::vector<account_name> accounts;
      accounts.reserve( 'z' - 'a' + 1);
      std::string root( "init" );
      for ( char c = 'a'; c <= 'z' ; ++c ) {
         accounts.emplace_back( root + std::string(1, c) );
      }
      
      account_name creator(N(eosio));
      signed_transaction trx;
      set_transaction_headers(trx);
      asset cpu = asset::from_string("1000000.0000 EOS");
      asset net = asset::from_string("1000000.0000 EOS");
      asset ram = asset::from_string("1.0000 EOS"); 

      for (const auto& a: accounts) {
         authority owner_auth( get_public_key( a, "owner" ) );
         trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                   newaccount{
                                         .creator  = creator,
                                         .name     = a,
                                         .owner    = owner_auth,
                                         .active   = authority( get_public_key( a, "active" ) )
                                         });

         trx.actions.emplace_back( get_action( N(eosio), N(buyram), vector<permission_level>{ {creator, config::active_name} },
                                               mvo()
                                               ("payer", creator)
                                               ("receiver", a)
                                               ("quant", ram) )
                                   );
         
         trx.actions.emplace_back( get_action( N(eosio), N(delegatebw), vector<permission_level>{ {creator, config::active_name} },
                                               mvo()
                                               ("from", creator)
                                               ("receiver", a)
                                               ("stake_net_quantity", net)
                                               ("stake_cpu_quantity", cpu )
                                               )
                                   );
      }
      
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
         ("max_ram_size", (n % 10 + 1) * 1024 * 1024)
         ("percent_of_max_inflation_rate", 50 + n)
         ("ram_reserve_ratio", 100 + n);
   }

   action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
      return push_action( acnt, N(regproducer), mvo()
                          ("producer",  acnt )
                          ("producer_key", get_public_key( acnt, "active" ) )
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
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "user_resources", data );
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

   double stake2votes( asset stake ) {
      auto now = control->pending_block_time().time_since_epoch().count() / 1000000;
      return stake.amount * pow(2, int64_t(now/ (86400 * 7))/ double(52) ); // 52 week periods (i.e. ~years)
   }

   double stake2votes( const string& s ) {
      return stake2votes( asset::from_string(s) );
   }

   fc::variant get_stats( const string& symbolname ) {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : token_abi_ser.binary_to_variant( "currency_stats", data );
   }

   asset get_token_supply() {
      return get_stats("4,EOS")["supply"].as<asset>();
   }

   fc::variant get_global_state() {
      vector<char> data = get_row_by_account( N(eosio), N(eosio), N(global), N(global) );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data );

   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
};

fc::mutable_variant_object voter( account_name acct ) {
   return mutable_variant_object()
      ("owner", acct)
      ("proxy", name(0).to_string())
      ("producers", variants() )
      ("staked", int64_t(0))
      //("last_vote_weight", double(0))
      ("proxied_vote_weight", double(0))
      ("is_proxy", 0)
      ("deferred_trx_id", 0)
      ("last_unstake_time", fc::time_point_sec() )
      ("unstaking", asset() )
      ;
}

fc::mutable_variant_object voter( account_name acct, const string& vote_stake ) {
   return voter( acct )( "staked", asset::from_string( vote_stake ).amount );
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

   auto total = get_total_stake("alice");
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());

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

   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS").amount, total["net_weight"].as<asset>().amount );
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS").amount, total["cpu_weight"].as<asset>().amount );

   REQUIRE_MATCHING_OBJECT( voter( "alice", "300.0000 EOS"), get_voter_info( "alice" ) );

   auto bytes = total["ram_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );

   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS"), get_balance( "alice" ) );

   //unstake
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "200.0000 EOS", "100.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS").amount, total["net_weight"].as<asset>().amount);
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS").amount, total["cpu_weight"].as<asset>().amount);


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
                                    ("stake_ram_quantity", "10.0000 EOS"),
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
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0001 EOS"), total["net_weight"].as<asset>());
   auto vinfo = get_voter_info("alice" );
   wdump((vinfo));
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
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());

   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice" ) );

   //trying to unstake more net bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        unstake( "alice", "200.0001 EOS", "0.0000 EOS" )
   );

   //trying to unstake more cpu bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        unstake( "alice", "0.0000 EOS", "100.0001 EOS" )
   );

   //check that nothing has changed
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( delegate_to_another_user, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake ( "alice", "bob", "200.0000 EOS", "100.0000 EOS" ) );

   auto total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice" ) );
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
   BOOST_REQUIRE_EQUAL( asset::from_string("230.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("970.0000 EOS"), get_balance( "carol" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol", "30.0000 EOS" ), get_voter_info( "carol" ) );

   //alice should not be able to unstake money staked by carol
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        unstake( "alice", "bob", "2001.0000 EOS", "1.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        unstake( "alice", "bob", "1.0000 EOS", "101.0000 EOS" )
   );

   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("230.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS"), total["cpu_weight"].as<asset>());
   //balance should not change after unsuccessfull attempts to unstake
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice" ) );
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
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("30.0000 EOS"), total["cpu_weight"].as<asset>());

   //cpu
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "100.0000 EOS", "0.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("30.0000 EOS"), total["cpu_weight"].as<asset>());

   //net
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "0.0000 EOS", "200.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("230.0000 EOS"), total["cpu_weight"].as<asset>());

   //unstake cpu
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "100.0000 EOS", "0.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("230.0000 EOS"), total["cpu_weight"].as<asset>());

   //unstake net
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "0.0000 EOS", "200.0000 EOS" ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("30.0000 EOS"), total["cpu_weight"].as<asset>());
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( adding_stake_partial_unstake, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "200.0000 EOS", "100.0000 EOS" ) );

   REQUIRE_MATCHING_OBJECT( voter( "alice", "300.0000 EOS" ), get_voter_info( "alice" ) );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "100.0000 EOS", "50.0000 EOS" ) );

   auto total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("310.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("160.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice", "450.0000 EOS" ), get_voter_info( "alice" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("550.0000 EOS"), get_balance( "alice" ) );

   //unstake a share
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "bob", "150.0000 EOS", "75.0000 EOS" ) );

   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("160.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("85.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice", "225.0000 EOS" ), get_voter_info( "alice" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "bob", "50.0000 EOS", "25.0000 EOS" ) );
   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("60.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice", "150.0000 EOS" ), get_voter_info( "alice" ) );

   //combined amount should be available only in 3 days
   produce_block( fc::days(2) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("550.0000 EOS"), get_balance( "alice" ) );
   produce_block( fc::days(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("850.0000 EOS"), get_balance( "alice" ) );

} FC_LOG_AND_RETHROW()


// Tests for voting
BOOST_FIXTURE_TEST_CASE( producer_register_unregister, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   fc::variant params = producer_parameters_example(1);
   auto key =  fc::crypto::public_key( std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV") );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", key )
                                               ("url", "http://block.one")
                        )
   );

   auto info = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( "alice", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", info["url"].as_string() );

   //call regproducer again to change parameters
   fc::variant params2 = producer_parameters_example(2);

   info = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( "alice", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", info["url"].as_string() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(unregprod), mvo()
                                               ("producer",  "alice")
                        )
   );
   info = get_producer_info( "alice" );
   //key should be empty
   wdump((info));
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(info["producer_key"].as_string()) );
   //everything else should stay the same
   BOOST_REQUIRE_EQUAL( "alice", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", info["url"].as_string() );

   //unregister bob who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer not found" ),
                        push_action( N(bob), N(unregprod), mvo()
                                     ("producer",  "bob")
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_producer, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", get_public_key( N(alice), "active") )
                                               ("url", "http://block.one")
                        )
   );
   auto prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( "alice", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, prod["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", prod["url"].as_string() );

   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   issue( "carol", "3000.0000 EOS",  config::system_account_name );

   //bob makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "11.0000 EOS", "0.1111 EOS" ) );
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
   BOOST_TEST_REQUIRE( stake2votes("11.1111 EOS") == prod["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "alice", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( "http://block.one", prod["url"].as_string() );

   //carol makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "22.0000 EOS", "0.2222 EOS" ) );
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
   BOOST_TEST_REQUIRE( stake2votes("33.3333 EOS") == prod["total_votes"].as_double() );

   //bob increases his stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "55.0000 EOS", "0.5555 EOS" ) );
   //should increase alice's total_votes
   prod = get_producer_info( "alice" );
   BOOST_TEST_REQUIRE( stake2votes("88.8888 EOS") == prod["total_votes"].as_double() );

   //carol unstakes part of the stake
   BOOST_REQUIRE_EQUAL( success(), unstake( "carol", "2.0000 EOS", "0.0002 EOS"/*"2.0000 EOS", "0.0002 EOS"*/ ) );
   //should decrease alice's total_votes
   prod = get_producer_info( "alice" );
   wdump((prod));
   BOOST_TEST_REQUIRE( stake2votes("86.8886 EOS") == prod["total_votes"].as_double() );

   //bob revokes his vote
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(voteproducer), mvo()
                                               ("voter",  "bob")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>() )
                        )
   );
   //should decrease alice's total_votes
   prod = get_producer_info( "alice" );
   BOOST_TEST_REQUIRE( stake2votes("20.2220 EOS") == prod["total_votes"].as_double() );
   //but eos should still be at stake
   BOOST_REQUIRE_EQUAL( asset::from_string("1933.3334 EOS"), get_balance( "bob" ) );

   //carol unstakes rest of eos
   BOOST_REQUIRE_EQUAL( success(), unstake( "carol", "20.0000 EOS", "0.2220 EOS" ) );
   //should decrease alice's total_votes to zero
   prod = get_producer_info( "alice" );
   BOOST_TEST_REQUIRE( 0.0 == prod["total_votes"].as_double() );

   //carol should receive funds in 3 days
   produce_block( fc::days(3) );
   produce_block();
   BOOST_REQUIRE_EQUAL( asset::from_string("3000.0000 EOS"), get_balance( "carol" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unregistered_producer_voting, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "13.0000 EOS", "0.5791 EOS" ) );
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
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", get_public_key( N(alice), "active") )
                                               ("url", "")
                        )
   );
   //and then unregisters
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(unregprod), mvo()
                                               ("producer",  "alice")
                        )
   );
   //key should be empty
   auto prod = get_producer_info( "alice" );
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()) );

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
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", get_public_key(N(alice), "active") )
                                               ("url", "")
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
   BOOST_TEST_REQUIRE( 0 == prod["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( producer_keep_votes, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( N(alice), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", get_public_key( N(alice), "active") )
                                               ("url", "")
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
   BOOST_TEST_REQUIRE( stake2votes("13.5791 EOS") == prod["total_votes"].as_double() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(unregprod), mvo()
                                               ("producer",  "alice")
                        )
   );
   prod = get_producer_info( "alice" );
   //key should be empty
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()) );
   //check parameters just in case
   //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes("13.5791 EOS"), prod["total_votes"].as_double() );

   //regtister the same producer again
   params = producer_parameters_example(2);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", get_public_key( N(alice), "active") )
                                               ("url", "")
                        )
   );
   prod = get_producer_info( "alice" );
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes("13.5791 EOS"), prod["total_votes"].as_double() );

   //change parameters
   params = producer_parameters_example(3);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", get_public_key( N(alice), "active") )
                                               ("url","")
                        )
   );
   prod = get_producer_info( "alice" );
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes("13.5791 EOS"), prod["total_votes"].as_double() );
   //check parameters just in case
   //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_two_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   //alice becomes a producer
   fc::variant params = producer_parameters_example(1);
   auto key = get_public_key( N(alice), "active" );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproducer), mvo()
                                               ("producer",  "alice")
                                               ("producer_key", get_public_key( N(alice), "active") )
                                               ("url","")
                        )
   );
   //bob becomes a producer
   params = producer_parameters_example(2);
   key = get_public_key( N(bob), "active" );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(regproducer), mvo()
                                               ("producer",  "bob")
                                               ("producer_key", get_public_key( N(alice), "active") )
                                               ("url","")
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
   BOOST_TEST_REQUIRE( stake2votes("20.0005 EOS") == alice_info["total_votes"].as_double() );
   auto bob_info = get_producer_info( "bob" );
   BOOST_TEST_REQUIRE( stake2votes("20.0005 EOS") == bob_info["total_votes"].as_double() );

   //carol votes for alice (but revokes vote for bob)
   BOOST_REQUIRE_EQUAL( success(), push_action(N(carol), N(voteproducer), mvo()
                                               ("voter",  "carol")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice) } )
                        )
   );

   alice_info = get_producer_info( "alice" );
   BOOST_TEST_REQUIRE( stake2votes("20.0005 EOS") == alice_info["total_votes"].as_double() );
   bob_info = get_producer_info( "bob" );
   BOOST_TEST_REQUIRE( 0 == bob_info["total_votes"].as_double() );

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
   BOOST_TEST_REQUIRE( stake2votes("22.0005 EOS") == alice_info["total_votes"].as_double() );

   bob_info = get_producer_info( "bob" );
   BOOST_TEST_REQUIRE( stake2votes("2.0000 EOS") == bob_info["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_register_unregister_keeps_stake, eosio_system_tester ) try {
   //register proxy by first action for this user ever
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(regproxy), mvo()
                                               ("proxy",  "alice")
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" ), get_voter_info( "alice" ) );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(regproxy), mvo()
                                               ("proxy",  "alice")
                                               ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice" ), get_voter_info( "alice" ) );

   //stake and then register as a proxy
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "200.0002 EOS", "100.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(regproxy), mvo()
                                               ("proxy",  "bob")
                                               ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "bob" )( "staked", "300.0003 EOS" ), get_voter_info( "bob" ) );
   //unrgister and check that stake is still in place
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(regproxy), mvo()
                                               ("proxy",  "bob")
                                               ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "bob", "300.0003 EOS" ), get_voter_info( "bob" ) );

   //register as a proxy and then stake
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(regproxy), mvo()
                                               ("proxy",  "carol")
                                               ("isproxy", true)
                        )
   );
   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "246.0002 EOS", "531.0001 EOS" ) );
   //check that both proxy flag and stake a correct
   REQUIRE_MATCHING_OBJECT( proxy( "carol" )( "staked", "777.0003 EOS" ), get_voter_info( "carol" ) );

   //unregister
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(regproxy), mvo()
                                                ("proxy",  "carol")
                                                ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "carol", "777.0003 EOS" ), get_voter_info( "carol" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_stake_unstake_keeps_proxy_flag, eosio_system_tester ) try {
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                               ("proxy",  "alice")
                                               ("isproxy", true)
                        )
   );
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" ), get_voter_info( "alice" ) );

   //stake
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "100.0000 EOS", "50.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "staked", 1500000 ), get_voter_info( "alice" ) );

   //stake more
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "30.0000 EOS", "20.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )("staked", 2000000 ), get_voter_info( "alice" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "65.0000 EOS", "35.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )("staked", 1000000 ), get_voter_info( "alice" ) );

   //unstake the rest
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "65.0000 EOS", "35.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "staked", 0 ), get_voter_info( "alice" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_actions_affect_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   create_accounts_with_resources( {  N(producer1), N(producer2), N(producer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer3", 3) );

   //register as a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                                                ("isproxy", true)
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
   REQUIRE_MATCHING_OBJECT( proxy( "alice" )( "proxied_vote_weight", stake2votes("150.0003 EOS") ), get_voter_info( "alice" ) );

   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(voteproducer), mvo()
                                               ("voter",  "alice")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer1), N(producer2) } )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 0 == get_producer_info( "producer3" )["total_votes"].as_double() );

   //vote for another producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(voteproducer), mvo()
                                               ("voter",  "alice")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(producer1), N(producer3) } )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "producer3" )["total_votes"].as_double() );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                                                ("isproxy", false)
                        )
   );
   //REQUIRE_MATCHING_OBJECT( voter( "alice" )( "proxied_vote_weight", stake2votes("150.0003 EOS") ), get_voter_info( "alice" ) );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_double() );

   //register proxy again
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                                                ("isproxy", true)
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "producer3" )["total_votes"].as_double() );

   //stake increase by proxy itself affects producers
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "30.0001 EOS", "20.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( stake2votes("200.0005 EOS"), get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( stake2votes("200.0005 EOS"), get_producer_info( "producer3" )["total_votes"].as_double() );

   //stake decrease by proxy itself affects producers
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "10.0001 EOS", "10.0001 EOS" ) );
   BOOST_TEST_REQUIRE( stake2votes("180.0003 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("180.0003 EOS") == get_producer_info( "producer3" )["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producer_pay, eosio_system_tester) try {
   const asset large_asset = asset::from_string("100000000.0000 EOS");
   create_account_with_resources( N(inita), N(eosio), asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(initb), N(eosio), asset::from_string("1.0000 EOS"), false, large_asset, large_asset );

   create_account_with_resources( N(vota), N(eosio), asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(votb), N(eosio), asset::from_string("1.0000 EOS"), false, large_asset, large_asset );

   BOOST_REQUIRE_EQUAL(success(), regproducer(N(inita)));
   auto prod = get_producer_info( N(inita) );
   BOOST_REQUIRE_EQUAL("inita", prod["owner"].as_string());
   BOOST_REQUIRE_EQUAL(0, prod["total_votes"].as_double());

   issue( "vota", "400000000.0000 EOS", config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("vota", "100000000.0000 EOS", "100000000.0000 EOS"));

   BOOST_REQUIRE_EQUAL(success(), push_action(N(vota), N(voteproducer), mvo()
                                              ("voter",  "vota")
                                              ("proxy", name(0).to_string())
                                              ("producers", vector<account_name>{ N(inita) })
                                              )
                       );

   // inita is the only active producer
   // produce enough blocks so new schedule kicks in and inita produces some blocks
   {
      produce_blocks(50);

      const auto     initial_global_state   = get_global_state();
      const uint64_t initial_claim_time     = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const asset    initial_pervote_bucket = initial_global_state["pervote_bucket"].as<asset>();
      const asset    initial_savings        = initial_global_state["savings"].as<asset>();

      prod = get_producer_info("inita");
      const uint32_t produced_blocks = prod["produced_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < produced_blocks);
      BOOST_REQUIRE_EQUAL(0, prod["last_claim_time"].as<uint64_t>());
      const asset initial_supply  = get_token_supply();
      const asset initial_balance = get_balance(N(inita));
      
      BOOST_REQUIRE_EQUAL(success(), push_action(N(inita), N(claimrewards), mvo()("owner", "inita")));
      
      const auto global_state       = get_global_state();
      const uint64_t claim_time     = global_state["last_pervote_bucket_fill"].as_uint64();
      const asset    pervote_bucket = global_state["pervote_bucket"].as<asset>();
      const asset    savings        = global_state["savings"].as<asset>();
      prod = get_producer_info("inita");
      BOOST_REQUIRE_EQUAL(1, prod["produced_blocks"].as<uint32_t>());
      const asset supply  = get_token_supply();
      const asset balance = get_balance(N(inita));

      BOOST_REQUIRE_EQUAL(claim_time, prod["last_claim_time"].as<uint64_t>());
      const int32_t secs_between_fills = static_cast<int32_t>((claim_time - initial_claim_time) / 1000000);
      
      BOOST_REQUIRE_EQUAL(0, initial_pervote_bucket.amount);

      BOOST_REQUIRE_EQUAL(int64_t( (initial_supply.amount * secs_between_fills * ((4.879-1.0)/100.0)) / (52*7*24*3600) ),
                          savings.amount - initial_savings.amount);
      
      int64_t block_payments  = int64_t( initial_supply.amount * produced_blocks * (0.25/100.0) / (52*7*24*3600*2) );
      int64_t from_pervote_bucket = int64_t( initial_supply.amount * secs_between_fills * (0.75/100.0) / (52*7*24*3600) );

      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(block_payments + from_pervote_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(0, pervote_bucket.amount);
      } else {
         BOOST_REQUIRE_EQUAL(block_payments, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(from_pervote_bucket, pervote_bucket.amount);
      }

      const int64_t max_supply_growth = int64_t( (initial_supply.amount * secs_between_fills * (4.879/100.0)) / (52*7*24*3600) );
      BOOST_REQUIRE(max_supply_growth >= supply.amount - initial_supply.amount);
   }
   
   {
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within a day"),
                          push_action(N(inita), N(claimrewards), mvo()("owner", "inita")));
   }

   // inita waits for 23 hours and 55 minutes, can't claim rewards yet
   {
      produce_block(fc::seconds(23 * 3600 + 55 * 60));
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within a day"),
                          push_action(N(inita), N(claimrewards), mvo()("owner", "inita")));
   }

   // wait 5 more minutes, inita can now claim rewards again
   {
      produce_block(fc::seconds(5 * 60));
      const auto     initial_global_state   = get_global_state();
      const uint64_t initial_claim_time     = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const asset    initial_pervote_bucket = initial_global_state["pervote_bucket"].as<asset>();
      const asset    initial_savings        = initial_global_state["savings"].as<asset>();

      prod = get_producer_info("inita");
      const uint32_t produced_blocks = prod["produced_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < produced_blocks);
      BOOST_REQUIRE(0 < prod["last_claim_time"].as<uint64_t>());
      const asset initial_supply  = get_token_supply();
      const asset initial_balance = get_balance(N(inita));

      BOOST_REQUIRE_EQUAL(success(),
                          push_action(N(inita), N(claimrewards), mvo()("owner", "inita")));
      
      const auto     global_state   = get_global_state();
      const uint64_t claim_time     = global_state["last_pervote_bucket_fill"].as_uint64();
      const asset    pervote_bucket = global_state["pervote_bucket"].as<asset>();
      const asset    savings        = global_state["savings"].as<asset>();

      prod = get_producer_info("inita");
      BOOST_REQUIRE_EQUAL(1, prod["produced_blocks"].as<uint32_t>());
      const asset supply  = get_token_supply();
      const asset balance = get_balance(N(inita));

      BOOST_REQUIRE_EQUAL(claim_time, prod["last_claim_time"].as<uint64_t>());
      const int32_t secs_between_fills = static_cast<int32_t>((claim_time - initial_claim_time) / 1000000);

      BOOST_REQUIRE_EQUAL(int64_t( (initial_supply.amount * secs_between_fills * ((4.879-1.0)/100.0)) / (52*7*24*3600) ),
                          savings.amount - initial_savings.amount);

      int64_t block_payments      = int64_t( initial_supply.amount * produced_blocks * (0.25/100.0) / (52*7*24*3600*2) );
      int64_t from_pervote_bucket = int64_t( initial_pervote_bucket.amount + initial_supply.amount * secs_between_fills * (0.75/100.0) / (52*7*24*3600) );

      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(block_payments + from_pervote_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(0, pervote_bucket.amount);
      } else {
         BOOST_REQUIRE_EQUAL(block_payments, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(from_pervote_bucket, pervote_bucket.amount);
      }

      const int64_t max_supply_growth = int64_t( (initial_supply.amount * secs_between_fills * (4.879/100.0)) / (52*7*24*3600) );
      BOOST_REQUIRE(max_supply_growth >= supply.amount - initial_supply.amount);
   }
   
   // initb tries to claim rewards but he's not on the list
   {
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: account name is not in producer list"),
                          push_action(N(initb), N(claimrewards), mvo()("owner", "initb")));
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(multiple_producer_pay, eosio_system_tester) try {

   const auto tol = boost::test_tools::tolerance(0.0000000001);

   const int64_t secs_per_year = 52 * 7 * 24 * 3600;
   const int64_t blocks_per_year = 52 * 7* 24 * 3600 * 2;

   const double cont_rate    = 4.879/100.;
   const double standby_rate = 0.750/100.;
   const double block_rate   = 0.250/100.;

   const asset large_asset = asset::from_string("100000000.0000 EOS");
   create_account_with_resources( N(vota), N(eosio), asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(votb), N(eosio), asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(votc), N(eosio), asset::from_string("1.0000 EOS"), false, large_asset, large_asset );

   // create accounts {inita, initb, ..., initz} and register as producers
   setup_producer_accounts();
   std::vector<account_name> producer_names;
   {
      producer_names.reserve( 'z' - 'a' + 1);
      const std::string root( "init" );
      for ( char c = 'a'; c <= 'z' ; ++c ) {
         producer_names.emplace_back(root + std::string(1, c));
         regproducer( producer_names.back() );
      }
      
      BOOST_REQUIRE_EQUAL(0, get_producer_info( N(inita) )["total_votes"].as<double>());
      BOOST_REQUIRE_EQUAL(0, get_producer_info( N(initz) )["total_votes"].as<double>());
   }

   {
      issue( "vota", "100000000.0000 EOS", config::system_account_name);
      BOOST_REQUIRE_EQUAL(success(), stake("vota", "30000000.0000 EOS", "30000000.0000 EOS"));
      issue( "votb", "100000000.0000 EOS", config::system_account_name);
      BOOST_REQUIRE_EQUAL(success(), stake("votb", "30000000.0000 EOS", "30000000.0000 EOS"));
      issue( "votc", "100000000.0000 EOS", config::system_account_name);
      BOOST_REQUIRE_EQUAL(success(), stake("votc", "30000000.0000 EOS", "30000000.0000 EOS"));
   }

   // vota votes for inita ... initj
   // votb votes for inita ... initu
   // votc votes for inita ... initz
   {
      BOOST_REQUIRE_EQUAL(success(), push_action(N(vota), N(voteproducer), mvo()
                                                 ("voter",  "vota")
                                                 ("proxy", name(0).to_string())
                                                 ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+10))
                                                 )
                          );

      BOOST_REQUIRE_EQUAL(success(), push_action(N(votb), N(voteproducer), mvo()
                                                 ("voter",  "votb")
                                                 ("proxy", name(0).to_string())
                                                 ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
                                                 )
                          );
      
      BOOST_REQUIRE_EQUAL(success(), push_action(N(votc), N(voteproducer), mvo()
                                                 ("voter",  "votc")
                                                 ("proxy", name(0).to_string())
                                                 ("producers", vector<account_name>(producer_names.begin(), producer_names.end()))
                                                 )
                          );
   }

   {
      auto proda = get_producer_info( N(inita) );
      auto prodj = get_producer_info( N(initj) );
      auto prodk = get_producer_info( N(initk) );
      auto produ = get_producer_info( N(initu) );
      auto prodv = get_producer_info( N(initv) );
      auto prodz = get_producer_info( N(initz) );

      BOOST_REQUIRE (0 == proda["produced_blocks"].as<uint32_t>() && 0 == prodz["produced_blocks"].as<uint32_t>());
      BOOST_REQUIRE (0 == proda["last_claim_time"].as<uint64_t>() && 0 == prodz["last_claim_time"].as<uint64_t>());

      // check vote ratios
      BOOST_REQUIRE ( 0 < proda["total_votes"].as<double>() && 0 < prodz["total_votes"].as<double>() );
      BOOST_TEST( proda["total_votes"].as<double>() == prodj["total_votes"].as<double>(), tol );
      BOOST_TEST( prodk["total_votes"].as<double>() == produ["total_votes"].as<double>(), tol );
      BOOST_TEST( prodv["total_votes"].as<double>() == prodz["total_votes"].as<double>(), tol );
      BOOST_TEST( 2 * proda["total_votes"].as<double>() == 3 * produ["total_votes"].as<double>(), tol );
      BOOST_TEST( proda["total_votes"].as<double>() ==  3 * prodz["total_votes"].as<double>(), tol );
   }
   
   // give a chance for everyone to produce blocks
   {
      produce_blocks(21 * 12 + 20);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["produced_blocks"].as<uint32_t>()) {
            all_21_produced= false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["produced_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE(all_21_produced && rest_didnt_produce);
   }

   std::vector<double> vote_shares(producer_names.size());
   {
      double total_votes = 0;
      for (uint32_t i = 0; i < producer_names.size(); ++i) {
         vote_shares[i] = get_producer_info(producer_names[i])["total_votes"].as<double>();
         total_votes += vote_shares[i];
      }
      std::for_each(vote_shares.begin(), vote_shares.end(), [total_votes](double& x) { x /= total_votes; });
      
      BOOST_TEST(double(1) == std::accumulate(vote_shares.begin(), vote_shares.end(), double(0)), tol);
      BOOST_TEST(double(3./57.) == vote_shares[0], tol);
   }
  
   {
      const uint32_t prod_index = 2;
      const auto prod_name = producer_names[prod_index];
      const auto produced_blocks = get_producer_info(prod_name)["produced_blocks"].as<uint32_t>();

      const auto    initial_global_state    = get_global_state();
      const uint64_t initial_claim_time     = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const asset    initial_pervote_bucket = initial_global_state["pervote_bucket"].as<asset>();
      const asset    initial_savings        = initial_global_state["savings"].as<asset>();
      const asset    initial_supply         = get_token_supply();
      const asset    initial_balance        = get_balance(prod_name);
 
      BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
      
      const auto     global_state   = get_global_state();
      const uint64_t claim_time     = global_state["last_pervote_bucket_fill"].as_uint64();
      const asset    pervote_bucket = global_state["pervote_bucket"].as<asset>();
      const asset    savings        = global_state["savings"].as<asset>();
      const asset    supply         = get_token_supply();
      const asset    balance        = get_balance(prod_name);

      const int32_t secs_between_fills = static_cast<int32_t>((claim_time - initial_claim_time) / 1000000);

      BOOST_REQUIRE_EQUAL(int64_t( (initial_supply.amount * secs_between_fills * (cont_rate - standby_rate - block_rate)) / secs_per_year ),
                          savings.amount - initial_savings.amount);
      
      int64_t block_payments          = int64_t( initial_supply.amount * produced_blocks * block_rate / blocks_per_year );
      int64_t expected_pervote_bucket = int64_t( initial_pervote_bucket.amount + initial_supply.amount * secs_between_fills * standby_rate / secs_per_year ); 
      int64_t from_pervote_bucket     = int64_t( vote_shares[prod_index] * expected_pervote_bucket );
      
      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(block_payments + from_pervote_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(expected_pervote_bucket - from_pervote_bucket, pervote_bucket.amount);
      } else {
         BOOST_REQUIRE_EQUAL(block_payments, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(expected_pervote_bucket, pervote_bucket.amount);
      }

      produce_blocks(5);

      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within a day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   {
      const uint32_t prod_index = 23;
      const auto prod_name = producer_names[prod_index];
      const uint64_t initial_claim_time = get_global_state()["last_pervote_bucket_fill"].as_uint64();
      const asset    initial_supply     = get_token_supply();
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
      BOOST_REQUIRE_EQUAL(0, get_balance(prod_name).amount);
      BOOST_REQUIRE_EQUAL(initial_claim_time, get_global_state()["last_pervote_bucket_fill"].as_uint64());
      BOOST_REQUIRE_EQUAL(initial_supply, get_token_supply());
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within a day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   produce_block(fc::seconds(24 * 3600));

   {
      const uint32_t prod_index = 15;
      const auto prod_name = producer_names[prod_index];
      const auto produced_blocks = get_producer_info(prod_name)["produced_blocks"].as<uint32_t>();

      const auto     initial_global_state   = get_global_state();
      const uint64_t initial_claim_time     = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const asset    initial_pervote_bucket = initial_global_state["pervote_bucket"].as<asset>();
      const asset    initial_savings        = initial_global_state["savings"].as<asset>();
      const asset    initial_supply         = get_token_supply();
      const asset    initial_balance        = get_balance(prod_name);

      BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));

      const auto     global_state   = get_global_state();
      const uint64_t claim_time     = global_state["last_pervote_bucket_fill"].as_uint64();
      const asset    pervote_bucket = global_state["pervote_bucket"].as<asset>();
      const asset    savings        = global_state["savings"].as<asset>();
      const asset    supply         = get_token_supply();
      const asset    balance        = get_balance(prod_name);

      const int32_t secs_between_fills = static_cast<int32_t>((claim_time - initial_claim_time) / 1000000);

      BOOST_REQUIRE_EQUAL(int64_t( (initial_supply.amount * secs_between_fills * (cont_rate - standby_rate - block_rate)) / secs_per_year ),
                          savings.amount - initial_savings.amount);

      int64_t block_payments          = int64_t( initial_supply.amount * produced_blocks * block_rate / blocks_per_year );
      int64_t expected_pervote_bucket = int64_t( initial_pervote_bucket.amount + initial_supply.amount * secs_between_fills * standby_rate / secs_per_year );
      int64_t from_pervote_bucket     = int64_t( vote_shares[prod_index] * expected_pervote_bucket );

      BOOST_REQUIRE_EQUAL(block_payments + from_pervote_bucket, balance.amount - initial_balance.amount);
      BOOST_REQUIRE_EQUAL(expected_pervote_bucket - from_pervote_bucket, pervote_bucket.amount);

      produce_blocks(5);

      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within a day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   {
      const uint32_t prod_index = 23;
      const auto prod_name = producer_names[prod_index];
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
      BOOST_REQUIRE(100 * 10000 <= get_balance(prod_name).amount);
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within a day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( voters_actions_affect_proxy_and_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+6) ) try {
   create_accounts_with_resources( { N(donald), N(producer1), N(producer2), N(producer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "producer3", 3) );

   //alice becomes a producer
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                                                ("isproxy", true)
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
   BOOST_TEST_REQUIRE( stake2votes("50.0002 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("50.0002 EOS") == get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_double() );

   BOOST_REQUIRE_EQUAL( success(), push_action( N(donald), N(regproxy), mvo()
                                                ("proxy",  "donald")
                                                ("isproxy", true)
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
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_voter_info( "alice" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("200.0005 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("200.0005 EOS") == get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_double() );

   //carol chooses alice as a proxy
   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "30.0001 EOS", "20.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(voteproducer), mvo()
                                                ("voter",  "carol")
                                                ("proxy", "alice" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("200.0005 EOS") == get_voter_info( "alice" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("250.0007 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("250.0007 EOS") == get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_double() );


   //proxied voter carol increases stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "50.0000 EOS", "70.0000 EOS" ) );
   BOOST_TEST_REQUIRE( stake2votes("320.0005 EOS") == get_voter_info( "alice" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("370.0007 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("370.0007 EOS") == get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_double() );

   //proxied voter bob decreases stake
   BOOST_REQUIRE_EQUAL( success(), unstake( "bob", "50.0001 EOS", "50.0001 EOS" ) );
   BOOST_TEST_REQUIRE( stake2votes("220.0003 EOS") == get_voter_info( "alice" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("270.0005 EOS") == get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("270.0005 EOS") == get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_double() );

   //proxied voter carol chooses another proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(voteproducer), mvo()
                                                ("voter",  "carol")
                                                ("proxy", "donald" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("50.0001 EOS"), get_voter_info( "alice" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("170.0002 EOS"), get_voter_info( "donald" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("100.0003 EOS"), get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("100.0003 EOS"), get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "producer3" )["total_votes"].as_double() );

   //bob switches to direct voting and votes for one of the same producers, but not for another one
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(voteproducer), mvo()
                                                ("voter",  "bob")
                                                ("proxy", "")
                                                ("producers", vector<account_name>{ N(producer2) } )
                        )
   );
   BOOST_TEST_REQUIRE( 0.0 == get_voter_info( "alice" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE(  stake2votes("50.0002 EOS"), get_producer_info( "producer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("100.0003 EOS"), get_producer_info( "producer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 0.0 == get_producer_info( "producer3" )["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_both_proxy_and_producers, eosio_system_tester ) try {
   //alice becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                                ("proxy",  "alice")
                                                ("isproxy", true)
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
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: invalid proxy specified" ),
                        push_action(N(bob), N(voteproducer), mvo()
                                    ("voter",  "bob")
                                    ("proxy", "alice" )
                                    ("producers", vector<account_name>() )
                        )
   );

   //selecting not existing account as a proxy
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: invalid proxy specified" ),
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
   create_accounts_with_resources( {  N(producer1), N(producer2), N(producer3) } );
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
