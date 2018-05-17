#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#include <eosio.msig/eosio.msig.wast.hpp>
#include <eosio.msig/eosio.msig.abi.hpp>

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

      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.token) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         token_abi_ser.set_abi(abi);
      }

      create_currency( N(eosio.token), config::system_account_name, asset::from_string("10000000000.0000 EOS") );
      issue(config::system_account_name,      "1000000000.0000 EOS");
      BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );

      set_code( config::system_account_name, eosio_system_wast );
      set_abi( config::system_account_name, eosio_system_abi );
      
      { 
         const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         abi_ser.set_abi(abi);
      }
      
      produce_blocks();

      create_account_with_resources( N(alice1111111), N(eosio), asset::from_string("1.0000 EOS"), false );
      create_account_with_resources( N(bob111111111), N(eosio), asset::from_string("0.4500 EOS"), false );
      create_account_with_resources( N(carol1111111), N(eosio), asset::from_string("1.0000 EOS"), false );

      BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );
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
                                            ("transfer", 0 )
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
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), chain_id_type()  );
      return push_transaction( trx );
   }

   transaction_trace_ptr setup_producer_accounts( const std::vector<account_name>& accounts ) {
      account_name creator(N(eosio));
      signed_transaction trx;
      set_transaction_headers(trx);
      asset cpu = asset::from_string("80.0000 EOS");
      asset net = asset::from_string("80.0000 EOS");
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
                                               ("transfer", 0 )
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

   action_result sellram( const account_name& account, uint64_t numbytes ) {
      return push_action( account, N(sellram), mvo()( "account", account)("bytes",numbytes) );
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = config::system_account_name;
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   }

   action_result stake( const account_name& from, const account_name& to, const string& net, const string& cpu ) {
      return push_action( name(from), N(delegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
                                            ("transfer", 0 )
      );
   }

   action_result stake( const account_name& acnt, const string& net, const string& cpu ) {
      return stake( acnt, acnt, net, cpu );
   }

   action_result stake_with_transfer( const account_name& from, const account_name& to, const string& net, const string& cpu ) {
      return push_action( name(from), N(delegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
                          ("transfer", true )
      );
   }

   action_result stake_with_transfer( const account_name& acnt, const string& net, const string& cpu ) {
      return stake_with_transfer( acnt, acnt, net, cpu );
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
         ("min_transaction_cpu_usage", 100 + n )
         ("max_transaction_lifetime", 3600 + n)
         ("deferred_trx_expiration_window", 600 + n)
         ("max_transaction_delay", 10*86400+n)
         ("max_inline_action_size", 4096 + n)
         ("max_inline_action_depth", 4 + n)
         ("max_authority_depth", 6 + n)
         ("max_generated_transaction_count", 10 + n)
         ("max_ram_size", (n % 10 + 1) * 1024 * 1024)
         ("ram_reserve_ratio", 100 + n);
   }

   action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
      action_result r = push_action( acnt, N(regproducer), mvo()
                          ("producer",  acnt )
                          ("producer_key", get_public_key( acnt, "active" ) )
                          ("url", "" )
                          ("location", 0 )
      );
      BOOST_REQUIRE_EQUAL( success(), r);
      return r;
   }

   uint32_t last_block_time() const {
      return time_point_sec( control->head_block_time() ).sec_since_epoch();
   }

   asset get_balance( const account_name& act ) {
      vector<char> data = get_row_by_account( N(eosio.token), act, N(accounts), symbol(SY(4,EOS)).to_symbol_code().value );
      return data.empty() ? asset(0, symbol(SY(4,EOS))) : token_abi_ser.binary_to_variant("account", data)["balance"].as<asset>();
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
         ("maximum_supply", maxsupply );

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

fc::mutable_variant_object voter( account_name acct, int64_t vote_stake ) {
   return voter( acct )( "staked", vote_stake );
}

fc::mutable_variant_object proxy( account_name acct ) {
   return voter( acct )( "is_proxy", 1 );
}

inline uint64_t M( const string& eos_str ) {
   return asset::from_string( eos_str ).amount;
}

BOOST_AUTO_TEST_SUITE(eosio_system_tests)

BOOST_FIXTURE_TEST_CASE( buysell, eosio_system_tester ) try {

   BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS"), get_balance( "alice1111111" ) );

   transfer( "eosio", "alice1111111", "1000.0000 EOS", "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", "200.0000 EOS", "100.0000 EOS" ) );

   auto total = get_total_stake( "alice1111111" );
   auto init_bytes =  total["ram_bytes"].as_uint64();

   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "200.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("800.0000 EOS"), get_balance( "alice1111111" ) );

   total = get_total_stake( "alice1111111" );
   auto bytes = total["ram_bytes"].as_uint64();
   auto bought_bytes = bytes - init_bytes;
   wdump((init_bytes)(bought_bytes)(bytes) );

   BOOST_REQUIRE_EQUAL( true, 0 < bought_bytes );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", bought_bytes ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("999.9999 EOS"), get_balance( "alice1111111" ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( true, total["ram_bytes"].as_uint64() == init_bytes );

   transfer( "eosio", "alice1111111", "100000000.0000 EOS", "eosio" );
   BOOST_REQUIRE_EQUAL( asset::from_string("100000999.9999 EOS"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10000000.0000 EOS" ) );

   total = get_total_stake( "alice1111111" );
   bytes = total["ram_bytes"].as_uint64();
   bought_bytes = bytes - init_bytes;
   wdump((init_bytes)(bought_bytes)(bytes) );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", bought_bytes ) );
   total = get_total_stake( "alice1111111" );

   bytes = total["ram_bytes"].as_uint64();
   bought_bytes = bytes - init_bytes;
   wdump((init_bytes)(bought_bytes)(bytes) );

   BOOST_REQUIRE_EQUAL( true, total["ram_bytes"].as_uint64() == init_bytes );
   BOOST_REQUIRE_EQUAL( asset::from_string("100000999.9993 EOS"), get_balance( "alice1111111" ) );


   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "30.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("100000439.9993 EOS"), get_balance( "alice1111111" ) );

   auto newtotal = get_total_stake( "alice1111111" );

   auto newbytes = newtotal["ram_bytes"].as_uint64();
   bought_bytes = newbytes - bytes;
   wdump((newbytes)(bytes)(bought_bytes) );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", bought_bytes ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("100000999.9991 EOS"), get_balance( "alice1111111" ) );


   newtotal = get_total_stake( "alice1111111" );
   auto startbytes = newtotal["ram_bytes"].as_uint64();

   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10000000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10000000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10000000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10000000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "10000000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "100000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "100000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "100000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", "300000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("49400999.9991 EOS"), get_balance( "alice1111111" ) );

   auto finaltotal = get_total_stake( "alice1111111" );
   auto endbytes = finaltotal["ram_bytes"].as_uint64();

   bought_bytes = endbytes - startbytes;
   wdump((startbytes)(endbytes)(bought_bytes) );

   BOOST_REQUIRE_EQUAL( success(), sellram( "alice1111111", bought_bytes ) );

   BOOST_REQUIRE_EQUAL( asset::from_string("100000999.9943 EOS"), get_balance( "alice1111111" ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_unstake, eosio_system_tester ) try {
   //issue( "eosio", "1000.0000 EOS", config::system_account_name );

   BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", "1000.0000 EOS", "eosio" );
   BOOST_REQUIRE_EQUAL( asset::from_string("999999000.0000 EOS"), get_balance( "eosio" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", "200.0000 EOS", "100.0000 EOS" ) );

   auto total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());

   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", "200.0000 EOS", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "alice1111111", "200.0000 EOS", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );

   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
   //after 3 days funds should be released
   produce_block( fc::hours(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice1111111" ) );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", "200.0000 EOS", "100.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
   total = get_total_stake("bob111111111");
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());

   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS").amount, total["net_weight"].as<asset>().amount );
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS").amount, total["cpu_weight"].as<asset>().amount );

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "300.0000 EOS"), get_voter_info( "alice1111111" ) );

   auto bytes = total["ram_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );

   //unstake from bob111111111
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "bob111111111", "200.0000 EOS", "100.0000 EOS" ) );
   total = get_total_stake("bob111111111");
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS"), total["cpu_weight"].as<asset>());
   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
   //after 3 days funds should be released
   produce_block( fc::hours(1) );
   produce_blocks(1);

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "0.0000 EOS" ), get_voter_info( "alice1111111" ) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice1111111" ) );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_unstake_with_transfer, eosio_system_tester ) try {
   //issue( "eosio", "1000.0000 EOS", config::system_account_name );
   BOOST_REQUIRE_EQUAL( asset::from_string("1000000000.0000 EOS"), get_balance( "eosio" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS"), get_balance( "alice1111111" ) );

   //eosio stakes for alice with transfer flag
   BOOST_REQUIRE_EQUAL( success(), stake_with_transfer( "eosio", "alice1111111", "200.0000 EOS", "100.0000 EOS" ) );

   //check that alice has both bandwidth and voting power
   auto total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "300.0000 EOS"), get_voter_info( "alice1111111" ) );

   //BOOST_REQUIRE_EQUAL( asset::from_string("999999700.0000 EOS"), get_balance( "eosio" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS"), get_balance( "alice1111111" ) );

   //alice stakes for herself
   transfer( "eosio", "alice1111111", "1000.0000 EOS", "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", "200.0000 EOS", "100.0000 EOS" ) );
   //now alice's stake should be equal to transfered from eosio + own stake
   total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("410.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "600.0000 EOS"), get_voter_info( "alice1111111" ) );

   //alice can unstake everything (including what was transfered)
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "alice1111111", "400.0000 EOS", "200.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );

   produce_block( fc::hours(3*24-1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
   //after 3 days funds should be released
   produce_block( fc::hours(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("1300.0000 EOS"), get_balance( "alice1111111" ) );

   //stake should be equal to what was staked in constructor, votring power should be 0
   total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("10.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "0.0000 EOS"), get_voter_info( "alice1111111" ) );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( fail_without_auth, eosio_system_tester ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake( "eosio", "alice1111111", "2000.0000 EOS", "1000.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", "10.0000 EOS", "10.0000 EOS" ) );

   BOOST_REQUIRE_EQUAL( error("missing authority of alice1111111"),
                        push_action( N(alice1111111), N(delegatebw), mvo()
                                    ("from",     "alice1111111")
                                    ("receiver", "bob111111111")
                                    ("stake_net_quantity", "10.0000 EOS")
                                    ("stake_cpu_quantity", "10.0000 EOS")
                                    ("transfer", 0 )
                                    ,false
                        )
   );

   BOOST_REQUIRE_EQUAL( error("missing authority of alice1111111"),
                        push_action(N(alice1111111), N(undelegatebw), mvo()
                                    ("from",     "alice1111111")
                                    ("receiver", "bob111111111")
                                    ("unstake_net_quantity", "200.0000 EOS")
                                    ("unstake_cpu_quantity", "100.0000 EOS")
                                    ("transfer", 0 )
                                    ,false
                        )
   );
   //REQUIRE_MATCHING_OBJECT( , get_voter_info( "alice1111111" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( stake_negative, eosio_system_tester ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice1111111", "-0.0001 EOS", "0.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice1111111", "0.0000 EOS", "-0.0001 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice1111111", "00.0000 EOS", "00.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice1111111", "0.0000 EOS", "00.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( true, get_voter_info( "alice1111111" ).is_null() );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unstake_negative, eosio_system_tester ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", "200.0001 EOS", "100.0001 EOS" ) );

   auto total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0001 EOS"), total["net_weight"].as<asset>());
   auto vinfo = get_voter_info("alice1111111" );
   wdump((vinfo));
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "300.0002 EOS" ), get_voter_info( "alice1111111" ) );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice1111111", "bob111111111", "-1.0000 EOS", "0.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice1111111", "bob111111111", "0.0000 EOS", "-1.0000 EOS" )
   );

   //unstake all zeros
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice1111111", "bob111111111", "0.0000 EOS", "0.0000 EOS" )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unstake_more_than_at_stake, eosio_system_tester ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "200.0000 EOS", "100.0000 EOS" ) );

   auto total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());

   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );

   //trying to unstake more net bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        unstake( "alice1111111", "200.0001 EOS", "0.0000 EOS" )
   );

   //trying to unstake more cpu bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        unstake( "alice1111111", "0.0000 EOS", "100.0001 EOS" )
   );

   //check that nothing has changed
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( delegate_to_another_user, eosio_system_tester ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake ( "alice1111111", "bob111111111", "200.0000 EOS", "100.0000 EOS" ) );

   auto total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("210.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
   //all voting power goes to alice1111111
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "300.0000 EOS" ), get_voter_info( "alice1111111" ) );
   //but not to bob111111111
   BOOST_REQUIRE_EQUAL( true, get_voter_info( "bob111111111" ).is_null() );

   //bob111111111 should not be able to unstake what was staked by alice1111111
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: unable to find key"),
                        unstake( "bob111111111", "0.0000 EOS", "10.0000 EOS" )
   );

   issue( "carol1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", "bob111111111", "20.0000 EOS", "10.0000 EOS" ) );
   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("230.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS"), total["cpu_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("970.0000 EOS"), get_balance( "carol1111111" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", "30.0000 EOS" ), get_voter_info( "carol1111111" ) );

   //alice1111111 should not be able to unstake money staked by carol1111111
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        unstake( "alice1111111", "bob111111111", "2001.0000 EOS", "1.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        unstake( "alice1111111", "bob111111111", "1.0000 EOS", "101.0000 EOS" )
   );

   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("230.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS"), total["cpu_weight"].as<asset>());
   //balance should not change after unsuccessfull attempts to unstake
   BOOST_REQUIRE_EQUAL( asset::from_string("700.0000 EOS"), get_balance( "alice1111111" ) );
   //voting power too
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "300.0000 EOS" ), get_voter_info( "alice1111111" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", "30.0000 EOS" ), get_voter_info( "carol1111111" ) );
   BOOST_REQUIRE_EQUAL( true, get_voter_info( "bob111111111" ).is_null() );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( stake_unstake_separate, eosio_system_tester ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice1111111" ) );

   //everything at once
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "10.0000 EOS", "20.0000 EOS" ) );
   auto total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("30.0000 EOS"), total["cpu_weight"].as<asset>());

   //cpu
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "100.0000 EOS", "0.0000 EOS" ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("30.0000 EOS"), total["cpu_weight"].as<asset>());

   //net
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "0.0000 EOS", "200.0000 EOS" ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("230.0000 EOS"), total["cpu_weight"].as<asset>());

   //unstake cpu
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "100.0000 EOS", "0.0000 EOS" ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("230.0000 EOS"), total["cpu_weight"].as<asset>());

   //unstake net
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "0.0000 EOS", "200.0000 EOS" ) );
   total = get_total_stake( "alice1111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("20.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("30.0000 EOS"), total["cpu_weight"].as<asset>());
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( adding_stake_partial_unstake, eosio_system_tester ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", "200.0000 EOS", "100.0000 EOS" ) );

   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "300.0000 EOS" ), get_voter_info( "alice1111111" ) );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "bob111111111", "100.0000 EOS", "50.0000 EOS" ) );

   auto total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("310.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("160.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "450.0000 EOS" ), get_voter_info( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("550.0000 EOS"), get_balance( "alice1111111" ) );

   //unstake a share
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "bob111111111", "150.0000 EOS", "75.0000 EOS" ) );

   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("160.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("85.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "225.0000 EOS" ), get_voter_info( "alice1111111" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "bob111111111", "50.0000 EOS", "25.0000 EOS" ) );
   total = get_total_stake( "bob111111111" );
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS"), total["net_weight"].as<asset>());
   BOOST_REQUIRE_EQUAL( asset::from_string("60.0000 EOS"), total["cpu_weight"].as<asset>());
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111", "150.0000 EOS" ), get_voter_info( "alice1111111" ) );

   //combined amount should be available only in 3 days
   produce_block( fc::days(2) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("550.0000 EOS"), get_balance( "alice1111111" ) );
   produce_block( fc::days(1) );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL( asset::from_string("850.0000 EOS"), get_balance( "alice1111111" ) );

} FC_LOG_AND_RETHROW()


// Tests for voting
BOOST_FIXTURE_TEST_CASE( producer_register_unregister, eosio_system_tester ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );

   fc::variant params = producer_parameters_example(1);
   auto key =  fc::crypto::public_key( std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV") );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", key )
                                               ("url", "http://block.one")
                                               ("location", "0")
                        )
   );

   auto info = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", info["url"].as_string() );

   //call regproducer again to change parameters
   fc::variant params2 = producer_parameters_example(2);

   info = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", info["url"].as_string() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(unregprod), mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   info = get_producer_info( "alice1111111" );
   //key should be empty
   wdump((info));
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(info["producer_key"].as_string()) );
   //everything else should stay the same
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", info["url"].as_string() );

   //unregister bob111111111 who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer not found" ),
                        push_action( N(bob111111111), N(unregprod), mvo()
                                     ("producer",  "bob111111111")
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_producer, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url", "http://block.one")
                                               ("location", 0 )
                        )
   );
   auto prod = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, prod["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", prod["url"].as_string() );

   issue( "bob111111111", "2000.0000 EOS",  config::system_account_name );
   issue( "carol1111111", "3000.0000 EOS",  config::system_account_name );

   //bob111111111 makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "11.0000 EOS", "0.1111 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("1988.8889 EOS"), get_balance( "bob111111111" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", "11.1111 EOS" ), get_voter_info( "bob111111111" ) );

   //bob111111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob111111111), N(voteproducer), mvo()
                                               ("voter",  "bob111111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice1111111) } )
                        )
   );

   //check that producer parameters stay the same after voting
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes("11.1111 EOS") == prod["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "alice1111111", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( "http://block.one", prod["url"].as_string() );

   //carol1111111 makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", "22.0000 EOS", "0.2222 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", "22.2222 EOS" ), get_voter_info( "carol1111111" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("2977.7778 EOS"), get_balance( "carol1111111" ) );
   //carol1111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), push_action(N(carol1111111), N(voteproducer), mvo()
                                               ("voter",  "carol1111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice1111111) } )
                        )
   );
   //new stake votes be added to alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes("33.3333 EOS") == prod["total_votes"].as_double() );

   //bob111111111 increases his stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "55.0000 EOS", "0.5555 EOS" ) );
   //should increase alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes("88.8888 EOS") == prod["total_votes"].as_double() );

   //carol1111111 unstakes part of the stake
   BOOST_REQUIRE_EQUAL( success(), unstake( "carol1111111", "2.0000 EOS", "0.0002 EOS"/*"2.0000 EOS", "0.0002 EOS"*/ ) );
   //should decrease alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   wdump((prod));
   BOOST_TEST_REQUIRE( stake2votes("86.8886 EOS") == prod["total_votes"].as_double() );

   //bob111111111 revokes his vote
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(voteproducer), mvo()
                                               ("voter",  "bob111111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>() )
                        )
   );
   //should decrease alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes("20.2220 EOS") == prod["total_votes"].as_double() );
   //but eos should still be at stake
   BOOST_REQUIRE_EQUAL( asset::from_string("1933.3334 EOS"), get_balance( "bob111111111" ) );

   //carol1111111 unstakes rest of eos
   BOOST_REQUIRE_EQUAL( success(), unstake( "carol1111111", "20.0000 EOS", "0.2220 EOS" ) );
   //should decrease alice1111111's total_votes to zero
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 0.0 == prod["total_votes"].as_double() );

   //carol1111111 should receive funds in 3 days
   produce_block( fc::days(3) );
   produce_block();
   BOOST_REQUIRE_EQUAL( asset::from_string("3000.0000 EOS"), get_balance( "carol1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unregistered_producer_voting, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue( "bob111111111", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "13.0000 EOS", "0.5791 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", "13.5791 EOS" ), get_voter_info( "bob111111111" ) );

   //bob111111111 should not be able to vote for alice1111111 who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer is not registered" ),
                        push_action( N(bob111111111), N(voteproducer), mvo()
                                    ("voter",  "bob111111111")
                                    ("proxy", name(0).to_string() )
                                    ("producers", vector<account_name>{ N(alice1111111) } )
                        )
   );

   //alice1111111 registers as a producer
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );
   //and then unregisters
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(unregprod), mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   //key should be empty
   auto prod = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()) );

   //bob111111111 should not be able to vote for alice1111111 who is an unregistered producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer is not currently registered" ),
                        push_action( N(bob111111111), N(voteproducer), mvo()
                                    ("voter",  "bob111111111")
                                    ("proxy", name(0).to_string() )
                                    ("producers", vector<account_name>{ N(alice1111111) } )
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( more_than_30_producer_voting, eosio_system_tester ) try {
   issue( "bob111111111", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "13.0000 EOS", "0.5791 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", "13.5791 EOS" ), get_voter_info( "bob111111111" ) );

   //bob111111111 should not be able to vote for alice1111111 who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: attempt to vote for too many producers" ),
                        push_action( N(bob111111111), N(voteproducer), mvo()
                                     ("voter",  "bob111111111")
                                     ("proxy", name(0).to_string() )
                                     ("producers", vector<account_name>(31, N(alice1111111)) )
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_same_producer_30_times, eosio_system_tester ) try {
   issue( "bob111111111", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "50.0000 EOS", "50.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", "100.0000 EOS" ), get_voter_info( "bob111111111" ) );

   //alice1111111 becomes a producer
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key(N(alice1111111), "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );

   //bob111111111 should not be able to vote for alice1111111 who is not a producer
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: producer votes must be unique and sorted" ),
                        push_action( N(bob111111111), N(voteproducer), mvo()
                                     ("voter",  "bob111111111")
                                     ("proxy", name(0).to_string() )
                                     ("producers", vector<account_name>(30, N(alice1111111)) )
                        )
   );

   auto prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 0 == prod["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( producer_keep_votes, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( N(alice1111111), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );

   //bob111111111 makes stake
   issue( "bob111111111", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "13.0000 EOS", "0.5791 EOS" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", "13.5791 EOS" ), get_voter_info( "bob111111111" ) );

   //bob111111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob111111111), N(voteproducer), mvo()
                                               ("voter",  "bob111111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice1111111) } )
                        )
   );

   auto prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes("13.5791 EOS") == prod["total_votes"].as_double() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(unregprod), mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //key should be empty
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()) );
   //check parameters just in case
   //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes("13.5791 EOS"), prod["total_votes"].as_double() );

   //regtister the same producer again
   params = producer_parameters_example(2);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes("13.5791 EOS"), prod["total_votes"].as_double() );

   //change parameters
   params = producer_parameters_example(3);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //votes should stay the same
   BOOST_TEST_REQUIRE( stake2votes("13.5791 EOS"), prod["total_votes"].as_double() );
   //check parameters just in case
   //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_two_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   //alice1111111 becomes a producer
   fc::variant params = producer_parameters_example(1);
   auto key = get_public_key( N(alice1111111), "active" );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );
   //bob111111111 becomes a producer
   params = producer_parameters_example(2);
   key = get_public_key( N(bob111111111), "active" );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(regproducer), mvo()
                                               ("producer",  "bob111111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );

   //carol1111111 votes for alice1111111 and bob111111111
   issue( "carol1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", "15.0005 EOS", "5.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(carol1111111), N(voteproducer), mvo()
                                               ("voter",  "carol1111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice1111111), N(bob111111111) } )
                        )
   );

   auto alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes("20.0005 EOS") == alice_info["total_votes"].as_double() );
   auto bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( stake2votes("20.0005 EOS") == bob_info["total_votes"].as_double() );

   //carol1111111 votes for alice1111111 (but revokes vote for bob111111111)
   BOOST_REQUIRE_EQUAL( success(), push_action(N(carol1111111), N(voteproducer), mvo()
                                               ("voter",  "carol1111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice1111111) } )
                        )
   );

   alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes("20.0005 EOS") == alice_info["total_votes"].as_double() );
   bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( 0 == bob_info["total_votes"].as_double() );

   //alice1111111 votes for herself and bob111111111
   issue( "alice1111111", "2.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "1.0000 EOS", "1.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(voteproducer), mvo()
                                               ("voter",  "alice1111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(alice1111111), N(bob111111111) } )
                        )
   );

   alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( stake2votes("22.0005 EOS") == alice_info["total_votes"].as_double() );

   bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( stake2votes("2.0000 EOS") == bob_info["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_register_unregister_keeps_stake, eosio_system_tester ) try {
   //register proxy by first action for this user ever
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(regproxy), mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", true )
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(regproxy), mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //stake and then register as a proxy
   issue( "bob111111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "200.0002 EOS", "100.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(regproxy), mvo()
                                               ("proxy",  "bob111111111")
                                               ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "bob111111111" )( "staked", 3000003 ), get_voter_info( "bob111111111" ) );
   //unrgister and check that stake is still in place
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(regproxy), mvo()
                                               ("proxy",  "bob111111111")
                                               ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", "300.0003 EOS" ), get_voter_info( "bob111111111" ) );

   //register as a proxy and then stake
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol1111111), N(regproxy), mvo()
                                               ("proxy",  "carol1111111")
                                               ("isproxy", true)
                        )
   );
   issue( "carol1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", "246.0002 EOS", "531.0001 EOS" ) );
   //check that both proxy flag and stake a correct
   REQUIRE_MATCHING_OBJECT( proxy( "carol1111111" )( "staked", 7770003 ), get_voter_info( "carol1111111" ) );

   //unregister
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol1111111), N(regproxy), mvo()
                                                ("proxy",  "carol1111111")
                                                ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", "777.0003 EOS" ), get_voter_info( "carol1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_stake_unstake_keeps_proxy_flag, eosio_system_tester ) try {
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", true)
                        )
   );
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //stake
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "100.0000 EOS", "50.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "staked", 1500000 ), get_voter_info( "alice1111111" ) );

   //stake more
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "30.0000 EOS", "20.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )("staked", 2000000 ), get_voter_info( "alice1111111" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "65.0000 EOS", "35.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )("staked", 1000000 ), get_voter_info( "alice1111111" ) );

   //unstake the rest
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "65.0000 EOS", "35.0000 EOS" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "staked", 0 ), get_voter_info( "alice1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_actions_affect_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   create_accounts_with_resources( {  N(defproducer1), N(defproducer2), N(defproducer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3", 3) );

   //register as a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );

   //accumulate proxied votes
   issue( "bob111111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob111111111), N(voteproducer), mvo()
                                               ("voter",  "bob111111111")
                                               ("proxy", "alice1111111" )
                                               ("producers", vector<account_name>() )
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "proxied_vote_weight", stake2votes("150.0003 EOS") ), get_voter_info( "alice1111111" ) );

   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(voteproducer), mvo()
                                               ("voter",  "alice1111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(defproducer1), N(defproducer2) } )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 0 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //vote for another producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(voteproducer), mvo()
                                               ("voter",  "alice1111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(defproducer1), N(defproducer3) } )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", false)
                        )
   );
   //REQUIRE_MATCHING_OBJECT( voter( "alice1111111" )( "proxied_vote_weight", stake2votes("150.0003 EOS") ), get_voter_info( "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //register proxy again
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //stake increase by proxy itself affects producers
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "30.0001 EOS", "20.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( stake2votes("200.0005 EOS"), get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( stake2votes("200.0005 EOS"), get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //stake decrease by proxy itself affects producers
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice1111111", "10.0001 EOS", "10.0001 EOS" ) );
   BOOST_TEST_REQUIRE( stake2votes("180.0003 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("180.0003 EOS") == get_producer_info( "defproducer3" )["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producer_pay, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   const double continuous_rate = 4.879 / 100.;
   const double usecs_per_year  = 52 * 7 * 24 * 3600 * 1000000ll;
   const double secs_per_year   = 52 * 7 * 24 * 3600;

   const asset large_asset = asset::from_string("80.0000 EOS");
   create_account_with_resources( N(defproducera), config::system_account_name, asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(defproducerb), config::system_account_name, asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(defproducerc), config::system_account_name, asset::from_string("1.0000 EOS"), false, large_asset, large_asset );

   create_account_with_resources( N(producvotera), config::system_account_name, asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(producvoterb), config::system_account_name, asset::from_string("1.0000 EOS"), false, large_asset, large_asset );

   BOOST_REQUIRE_EQUAL(success(), regproducer(N(defproducera)));
   auto prod = get_producer_info( N(defproducera) );
   BOOST_REQUIRE_EQUAL("defproducera", prod["owner"].as_string());
   BOOST_REQUIRE_EQUAL(0, prod["total_votes"].as_double());

   transfer( config::system_account_name, "producvotera", "400000000.0000 EOS", config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("producvotera", "100000000.0000 EOS", "100000000.0000 EOS"));

   BOOST_REQUIRE_EQUAL(success(), push_action(N(producvotera), N(voteproducer), mvo()
                                              ("voter",  "producvotera")
                                              ("proxy", name(0).to_string())
                                              ("producers", vector<account_name>{ N(defproducera) })
                                              )
                       );

   // defproducera is the only active producer
   // produce enough blocks so new schedule kicks in and defproducera produces some blocks
   {
      produce_blocks(50);

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = initial_global_state["savings"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < unpaid_blocks);
      BOOST_REQUIRE_EQUAL(0, prod["last_claim_time"].as<uint64_t>());
      
      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

      const asset initial_supply  = get_token_supply();
      const asset initial_balance = get_balance(N(defproducera));

      BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
      
      const auto global_state          = get_global_state();
      const uint64_t claim_time        = global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = global_state["savings"].as<int64_t>();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();


      prod = get_producer_info("defproducera");
      BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
      const asset supply  = get_token_supply();
      const asset balance = get_balance(N(defproducera));

      BOOST_REQUIRE_EQUAL(claim_time, prod["last_claim_time"].as<uint64_t>());

      auto usecs_between_fills = claim_time - initial_claim_time;
      int32_t secs_between_fills = usecs_between_fills/1000000;

      BOOST_REQUIRE_EQUAL(0, initial_savings);
      BOOST_REQUIRE_EQUAL(0, initial_perblock_bucket);
      BOOST_REQUIRE_EQUAL(0, initial_pervote_bucket);
      
      BOOST_REQUIRE_EQUAL(int64_t( ( initial_supply.amount * double(secs_between_fills) * continuous_rate ) / secs_per_year ),
                          supply.amount - initial_supply.amount);
      BOOST_REQUIRE_EQUAL(int64_t( ( initial_supply.amount * double(secs_between_fills) * (4.   * continuous_rate/ 5.) / secs_per_year ) ),
                          savings - initial_savings);
      BOOST_REQUIRE_EQUAL(int64_t( ( initial_supply.amount * double(secs_between_fills) * (0.25 * continuous_rate/ 5.) / secs_per_year ) ),
                          balance.amount - initial_balance.amount);

      int64_t from_perblock_bucket = int64_t( initial_supply.amount * double(secs_between_fills) * (0.25 * continuous_rate/ 5.) / secs_per_year ) ;
      int64_t from_pervote_bucket  = int64_t( initial_supply.amount * double(secs_between_fills) * (0.75 * continuous_rate/ 5.) / secs_per_year ) ;
      

      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_pervote_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(0, pervote_bucket);
      } else {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(from_pervote_bucket, pervote_bucket);
      }
   }

   {
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within past day"),
                          push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
   }

   // defproducera waits for 23 hours and 55 minutes, can't claim rewards yet
   {
      produce_block(fc::seconds(23 * 3600 + 55 * 60));
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within past day"),
                          push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
   }

   // wait 5 more minutes, defproducera can now claim rewards again
   {
      produce_block(fc::seconds(5 * 60));

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = initial_global_state["savings"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const double   initial_tot_vote_weight   = initial_global_state["total_producer_vote_weight"].as<double>();

      prod = get_producer_info("defproducera");
      const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < unpaid_blocks);
      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);
      BOOST_REQUIRE(0 < prod["total_votes"].as<double>());
      BOOST_TEST(initial_tot_vote_weight, prod["total_votes"].as<double>());
      BOOST_REQUIRE(0 < prod["last_claim_time"].as<uint64_t>());

      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

      const asset initial_supply  = get_token_supply();
      const asset initial_balance = get_balance(N(defproducera));


      BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));

      const auto global_state          = get_global_state();
      const uint64_t claim_time        = global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = global_state["savings"].as<int64_t>();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();


      prod = get_producer_info("defproducera");
      BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
      const asset supply  = get_token_supply();
      const asset balance = get_balance(N(defproducera));

      BOOST_REQUIRE_EQUAL(claim_time, prod["last_claim_time"].as<uint64_t>());
      auto usecs_between_fills = claim_time - initial_claim_time;

      BOOST_REQUIRE_EQUAL(int64_t( ( double(initial_supply.amount) * double(usecs_between_fills) * continuous_rate / usecs_per_year ) ),
                          supply.amount - initial_supply.amount);
      BOOST_REQUIRE_EQUAL( (supply.amount - initial_supply.amount) - (supply.amount - initial_supply.amount) / 5,
                          savings - initial_savings);

      int64_t to_producer        = int64_t( (double(initial_supply.amount) * double(usecs_between_fills) * continuous_rate) / usecs_per_year ) / 5;
      int64_t to_perblock_bucket = to_producer / 4;
      int64_t to_pervote_bucket  = to_producer - to_perblock_bucket;

      if (to_pervote_bucket + initial_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(to_perblock_bucket + to_pervote_bucket + initial_pervote_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(0, pervote_bucket);
      } else {
         BOOST_REQUIRE_EQUAL(to_perblock_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(to_pervote_bucket + initial_pervote_bucket, pervote_bucket);
      }
   }

   // defproducerb tries to claim rewards but he's not on the list
   {
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: unable to find key"),
                          push_action(N(defproducerb), N(claimrewards), mvo()("owner", "defproducerb")));
   }

   // test stability over a year
   {
      regproducer(N(defproducerb));
      regproducer(N(defproducerc));
      const asset   initial_supply  = get_token_supply();
      const int64_t initial_savings = get_global_state()["savings"].as<int64_t>();
      for (uint32_t i = 0; i < 7 * 52; ++i) {
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducerc), N(claimrewards), mvo()("owner", "defproducerc")));
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducerb), N(claimrewards), mvo()("owner", "defproducerb")));
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
      }
      const asset   supply  = get_token_supply();
      const int64_t savings = get_global_state()["savings"].as<int64_t>();
      // Amount issued per year is very close to the 5% inflation target. Small difference (500 tokens out of 50'000'000 issued)
      // is due to compounding every 8 hours in this test as opposed to theoretical continuous compounding 
      BOOST_REQUIRE(500 * 10000 > int64_t(double(initial_supply.amount) * double(0.05)) - (supply.amount - initial_supply.amount));
      BOOST_REQUIRE(500 * 10000 > int64_t(double(initial_supply.amount) * double(0.04)) - (savings - initial_savings));
   }
} FC_LOG_AND_RETHROW()



BOOST_FIXTURE_TEST_CASE(multiple_producer_pay, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   const int64_t secs_per_year  = 52 * 7 * 24 * 3600;
   const double  usecs_per_year = secs_per_year * 1000000;
   const double  cont_rate      = 4.879/100.;

   const asset net = asset::from_string("80.0000 EOS");
   const asset cpu = asset::from_string("80.0000 EOS");
   create_account_with_resources( N(producvotera), config::system_account_name, asset::from_string("1.0000 EOS"), false, net, cpu );
   create_account_with_resources( N(producvoterb), config::system_account_name, asset::from_string("1.0000 EOS"), false, net, cpu );
   create_account_with_resources( N(producvoterc), config::system_account_name, asset::from_string("1.0000 EOS"), false, net, cpu );

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      const std::string root("defproducer");
      for ( char c = 'a'; c <= 'z'; ++c ) {
         producer_names.emplace_back(root + std::string(1, c));
      }
      setup_producer_accounts(producer_names);
      for (const auto& p: producer_names) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         produce_blocks(1);
         ilog( "------ get pro----------" );
         wdump((p));
         BOOST_TEST(0 == get_producer_info(p)["total_votes"].as<double>());
      }
   }

   {
      transfer( config::system_account_name, "producvotera", "100000000.0000 EOS", config::system_account_name );
      BOOST_REQUIRE_EQUAL(success(), stake("producvotera", "30000000.0000 EOS", "30000000.0000 EOS") );
      transfer( config::system_account_name, "producvoterb", "100000000.0000 EOS", config::system_account_name);
      BOOST_REQUIRE_EQUAL(success(), stake("producvoterb", "30000000.0000 EOS", "30000000.0000 EOS") );
      transfer( config::system_account_name, "producvoterc", "100000000.0000 EOS", config::system_account_name);
      BOOST_REQUIRE_EQUAL(success(), stake("producvoterc", "30000000.0000 EOS", "30000000.0000 EOS") );
   }

   // producvotera votes for defproducera ... defproducerj
   // producvoterb votes for defproducera ... defproduceru
   // producvoterc votes for defproducera ... defproducerz
   {
      BOOST_REQUIRE_EQUAL(success(), push_action(N(producvotera), N(voteproducer), mvo()
                                                 ("voter",  "producvotera")
                                                 ("proxy", name(0).to_string())
                                                 ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+10))
                                                 )
                          );

      BOOST_REQUIRE_EQUAL(success(), push_action(N(producvoterb), N(voteproducer), mvo()
                                                 ("voter",  "producvoterb")
                                                 ("proxy", name(0).to_string())
                                                 ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
                                                 )
                          );

      BOOST_REQUIRE_EQUAL(success(), push_action(N(producvoterc), N(voteproducer), mvo()
                                                 ("voter",  "producvoterc")
                                                 ("proxy", name(0).to_string())
                                                 ("producers", vector<account_name>(producer_names.begin(), producer_names.end()))
                                                 )
                          );

   }

   {
      auto proda = get_producer_info( N(defproducera) );
      auto prodj = get_producer_info( N(defproducerj) );
      auto prodk = get_producer_info( N(defproducerk) );
      auto produ = get_producer_info( N(defproduceru) );
      auto prodv = get_producer_info( N(defproducerv) );
      auto prodz = get_producer_info( N(defproducerz) );

      BOOST_REQUIRE (0 == proda["unpaid_blocks"].as<uint32_t>() && 0 == prodz["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE (0 == proda["last_claim_time"].as<uint64_t>() && 0 == prodz["last_claim_time"].as<uint64_t>());

      // check vote ratios
      BOOST_REQUIRE ( 0 < proda["total_votes"].as<double>() && 0 < prodz["total_votes"].as<double>() );
      BOOST_TEST( proda["total_votes"].as<double>() == prodj["total_votes"].as<double>() );
      BOOST_TEST( prodk["total_votes"].as<double>() == produ["total_votes"].as<double>() );
      BOOST_TEST( prodv["total_votes"].as<double>() == prodz["total_votes"].as<double>() );
      BOOST_TEST( 2 * proda["total_votes"].as<double>() == 3 * produ["total_votes"].as<double>() );
      BOOST_TEST( proda["total_votes"].as<double>() ==  3 * prodz["total_votes"].as<double>() );
   }

   // give a chance for everyone to produce blocks
   {
      produce_blocks(23 * 12 + 20);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced = false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
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
      BOOST_TEST(total_votes == get_global_state()["total_producer_vote_weight"].as<double>());
      std::for_each(vote_shares.begin(), vote_shares.end(), [total_votes](double& x) { x /= total_votes; });

      BOOST_TEST(double(1) == std::accumulate(vote_shares.begin(), vote_shares.end(), double(0)));
      BOOST_TEST(double(3./57.) == vote_shares.front());
      BOOST_TEST(double(1./57.) == vote_shares.back());
   }

   {
      const uint32_t prod_index = 2;
      const auto prod_name = producer_names[prod_index];


      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = initial_global_state["savings"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    initial_supply            = get_token_supply();
      const asset    initial_balance           = get_balance(prod_name);
      const uint32_t initial_unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
      
      const auto     global_state      = get_global_state();
      const uint64_t claim_time        = global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = global_state["savings"].as<int64_t>();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    supply            = get_token_supply();
      const asset    balance           = get_balance(prod_name);
      const uint32_t unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      const uint64_t usecs_between_fills = claim_time - initial_claim_time;
      const int32_t secs_between_fills = static_cast<int32_t>(usecs_between_fills / 1000000);


      const double expected_supply_growth = initial_supply.amount * double(usecs_between_fills) * cont_rate / usecs_per_year;
      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth), supply.amount - initial_supply.amount );
      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth) - int64_t(expected_supply_growth)/5, savings - initial_savings );

      const int64_t expected_perblock_bucket = int64_t( initial_supply.amount * secs_between_fills * (0.25 * cont_rate/ 5.) / secs_per_year ) ;
      const int64_t expected_pervote_bucket  = int64_t( initial_supply.amount * secs_between_fills * (0.75 * cont_rate/ 5.) / secs_per_year ) ;

      const int64_t from_perblock_bucket = initial_unpaid_blocks * expected_perblock_bucket / initial_tot_unpaid_blocks ;
      const int64_t from_pervote_bucket  = int64_t( vote_shares[prod_index] * expected_pervote_bucket);
      
      BOOST_REQUIRE( 1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)) );

      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_pervote_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(expected_pervote_bucket - from_pervote_bucket, pervote_bucket);
      } else {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(expected_pervote_bucket, pervote_bucket);
      }

      produce_blocks(5);

      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within past day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   {
      const uint32_t prod_index = 23;
      const auto prod_name = producer_names[prod_index];
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
      BOOST_REQUIRE_EQUAL(0, get_balance(prod_name).amount);
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within past day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   // wait to 23 hours which is not enough for producers to get deactivated
   // payment calculations don't change. By now, pervote_bucket has grown enough
   // that a producer's share is more than 100 tokens
   produce_block(fc::seconds(23 * 3600));

   {
      const uint32_t prod_index = 15;
      const auto prod_name = producer_names[prod_index];

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = initial_global_state["savings"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    initial_supply            = get_token_supply();
      const asset    initial_balance           = get_balance(prod_name);
      const uint32_t initial_unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));

      const auto     global_state      = get_global_state();
      const uint64_t claim_time        = global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = global_state["savings"].as<int64_t>();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    supply            = get_token_supply();
      const asset    balance           = get_balance(prod_name);
      const uint32_t unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      const uint64_t usecs_between_fills = claim_time - initial_claim_time;

      const double expected_supply_growth = initial_supply.amount * double(usecs_between_fills) * cont_rate / usecs_per_year;
      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth), supply.amount - initial_supply.amount );
      BOOST_REQUIRE_EQUAL( int64_t(expected_supply_growth) - int64_t(expected_supply_growth)/5, savings - initial_savings );

      const int64_t expected_perblock_bucket = int64_t( initial_supply.amount * double(usecs_between_fills) * (0.25 * cont_rate/ 5.) / usecs_per_year )
                                               + initial_perblock_bucket;
      const int64_t expected_pervote_bucket  = int64_t( initial_supply.amount * double(usecs_between_fills) * (0.75 * cont_rate/ 5.) / usecs_per_year )
                                               + initial_pervote_bucket;
      const int64_t from_perblock_bucket = initial_unpaid_blocks * expected_perblock_bucket / initial_tot_unpaid_blocks ;
      const int64_t from_pervote_bucket  = int64_t( vote_shares[prod_index] * expected_pervote_bucket);

      BOOST_REQUIRE( 1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)) );
      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_pervote_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(expected_pervote_bucket - from_pervote_bucket, pervote_bucket);
      } else {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket, balance.amount - initial_balance.amount);
         BOOST_REQUIRE_EQUAL(expected_pervote_bucket, pervote_bucket);
      }

      produce_blocks(5);

      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within past day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   {
      const uint32_t prod_index = 24;
      const auto prod_name = producer_names[prod_index];
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
      BOOST_REQUIRE(100 * 10000 <= get_balance(prod_name).amount);
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: already claimed rewards within past day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   // wait two more hours, now most producers haven't produced in a day and will
   // be deactivated
   produce_block(fc::seconds(2 * 3600));
   produce_blocks(8 * 21 * 12);

   {
      bool all_newly_elected_produced = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_newly_elected_produced = false;
         }
      }
      BOOST_REQUIRE(all_newly_elected_produced);
   }

   {
      uint32_t survived_active_producers = 0;
      uint32_t one_inactive_index = 0;
      for (uint32_t i = 0; i < 21; ++i) {
         if (fc::crypto::public_key() != fc::crypto::public_key(get_producer_info(producer_names[i])["producer_key"].as_string())) {
            ++survived_active_producers;
         } else {
            one_inactive_index = i;
         }
      }

      BOOST_REQUIRE(3 >= survived_active_producers);

      auto inactive_prod_info = get_producer_info(producer_names[one_inactive_index]);
      BOOST_REQUIRE_EQUAL(0, inactive_prod_info["time_became_active"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: producer does not have an active key"),
                          push_action(producer_names[one_inactive_index], N(claimrewards), mvo()("owner", producer_names[one_inactive_index])));
      // re-register deactivated producer and let him produce blocks again
      const uint32_t initial_unpaid_blocks = inactive_prod_info["unpaid_blocks"].as<uint32_t>();
      regproducer(producer_names[one_inactive_index]);
      produce_blocks(21 * 12);
      auto reactivated_prod_info   = get_producer_info(producer_names[one_inactive_index]);
      const uint32_t unpaid_blocks = reactivated_prod_info["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(initial_unpaid_blocks + 12 <= unpaid_blocks);
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(producer_names[one_inactive_index], N(claimrewards), mvo()("owner", producer_names[one_inactive_index])));
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producers_upgrade_system_contract, eosio_system_tester) try {
   //install multisig contract
   abi_serializer msig_abi_ser;
   {
      create_account_with_resources( N(eosio.msig), N(eosio) );
      BOOST_REQUIRE_EQUAL( success(), buyram( "eosio", "eosio.msig", "5000.0000 EOS" ) );
      produce_block();

      auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                            config::system_account_name,  mutable_variant_object()
                                            ("account", "eosio.msig")
                                            ("is_priv", 1)
      );

      set_code( N(eosio.msig), eosio_msig_wast );
      set_abi( N(eosio.msig), eosio_msig_abi );

      produce_blocks();
      const auto& accnt = control->db().get<account_object,by_name>( N(eosio.msig) );
      abi_def msig_abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, msig_abi), true);
      msig_abi_ser.set_abi(msig_abi);
   }

   //stake more than 15% of total EOS supply to activate chain
   transfer( "eosio", "alice1111111", "650000000.0000 EOS", "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", "300000000.0000 EOS", "300000000.0000 EOS" ) );

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

         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
      }
   }
   produce_blocks( 250);

   //BOOST_REQUIRE_EQUAL( name("defproducer2"), producer_keys[1].producer_name );

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
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

   //vote for producers
   {
      transfer( config::system_account_name, "alice1111111", "100000000.0000 EOS", config::system_account_name );
      BOOST_REQUIRE_EQUAL(success(), stake( "alice1111111", "30000000.0000 EOS", "30000000.0000 EOS" ) );
      BOOST_REQUIRE_EQUAL(success(), buyram( "alice1111111", "alice1111111", "30000000.0000 EOS" ) );
      BOOST_REQUIRE_EQUAL(success(), push_action(N(alice1111111), N(voteproducer), mvo()
                                                 ("voter",  "alice1111111")
                                                 ("proxy", name(0).to_string())
                                                 ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
                                                 )
      );
   }
   produce_blocks( 250 );

   auto producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 21, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducera"), producer_keys[0].producer_name );

   //helper function
   auto push_action_msig = [&]( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) -> action_result {
         string action_type_name = msig_abi_ser.get_action_type(name);

         action act;
         act.account = N(eosio.msig);
         act.name = name;
         act.data = msig_abi_ser.variant_to_binary( action_type_name, data );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   };

   // test begins
   vector<permission_level> prod_perms;
   for ( auto& x : producer_names ) {
      prod_perms.push_back( { name(x), config::active_name } );
   }
   //prepare system contract with different hash (contract differs in one byte)
   string eosio_system_wast2 = eosio_system_wast;
   string msg = "producer votes must be unique and sorted";
   auto pos = eosio_system_wast2.find(msg);
   BOOST_REQUIRE( pos != std::string::npos );
   msg[0] = 'P';
   eosio_system_wast2.replace( pos, msg.size(), msg );

   transaction trx;
   {
      auto code = wast_to_wasm( eosio_system_wast2 );
      variant pretty_trx = fc::mutable_variant_object()
         ("expiration", "2020-01-01T00:30")
         ("ref_block_num", 2)
         ("ref_block_prefix", 3)
         ("max_net_usage_words", 0)
         ("max_cpu_usage_ms", 0)
         ("delay_sec", 0)
         ("actions", fc::variants({
               fc::mutable_variant_object()
                  ("account", name(config::system_account_name))
                  ("name", "setcode")
                  ("authorization", vector<permission_level>{ { config::system_account_name, config::active_name } })
                  ("data", fc::mutable_variant_object() ("account", name(config::system_account_name))
                   ("vmtype", 0)
                   ("vmversion", "0")
                   ("code", bytes( code.begin(), code.end() ))
                  )
                  })
         );
      abi_serializer::from_variant(pretty_trx, trx, get_resolver());
   }

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( N(alice1111111), N(propose), mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("trx",           trx)
                                                    ("requested", prod_perms)
                       )
   );

   // get 15 approvals
   for ( size_t i = 0; i < 14; ++i ) {
      BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[i]), N(approve), mvo()
                                                       ("proposer",      "alice1111111")
                                                       ("proposal_name", "upgrade1")
                                                       ("level",         permission_level{ name(producer_names[i]), config::active_name })
                          )
      );
   }

   //should fail
   BOOST_REQUIRE_EQUAL(error("condition: assertion failed: transaction authorization failed"),
                       push_action_msig( N(alice1111111), N(exec), mvo()
                                         ("proposer",      "alice1111111")
                                         ("proposal_name", "upgrade1")
                                         ("executer",      "alice1111111")
                       )
   );

   // one more approval
   BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[14]), N(approve), mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("level",         permission_level{ name(producer_names[14]), config::active_name })
                          )
   );

   transaction_trace_ptr trace;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace = t; } } );
   BOOST_REQUIRE_EQUAL(success(), push_action_msig( N(alice1111111), N(exec), mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("executer",      "alice1111111")
                       )
   );

   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );

   produce_blocks( 250 );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(producer_onblock_check, eosio_system_tester) try {

   const asset large_asset = asset::from_string("80.0000 EOS");
   create_account_with_resources( N(producvotera), config::system_account_name, asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(producvoterb), config::system_account_name, asset::from_string("1.0000 EOS"), false, large_asset, large_asset );
   create_account_with_resources( N(producvoterc), config::system_account_name, asset::from_string("1.0000 EOS"), false, large_asset, large_asset );

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   producer_names.reserve('z' - 'a' + 1);
   const std::string root("defproducer");
   for ( char c = 'a'; c <= 'z'; ++c ) {
      producer_names.emplace_back(root + std::string(1, c));
   }
   setup_producer_accounts(producer_names);

   for (auto a:producer_names)
      regproducer(a);

   BOOST_REQUIRE_EQUAL(0, get_producer_info( producer_names.front() )["total_votes"].as<double>());
   BOOST_REQUIRE_EQUAL(0, get_producer_info( producer_names.back() )["total_votes"].as<double>());

   transfer(config::system_account_name, "producvotera", "200000000.0000 EOS", config::system_account_name);
   
   BOOST_REQUIRE_EQUAL(success(), stake("producvotera", "70000000.0000 EOS", "70000000.0000 EOS"));
   BOOST_REQUIRE_EQUAL(success(), push_action(N(producvotera), N(voteproducer), mvo()
                                                ("voter",  "producvotera")
                                                ("proxy", name(0).to_string())
                                                ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+10))
                                                ));

   // give a chance for everyone to produce blocks
   {
      produce_blocks(21 * 12);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced= false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE_EQUAL(false, all_21_produced);
      BOOST_REQUIRE_EQUAL(true, rest_didnt_produce);
   }

   {
      BOOST_CHECK_EQUAL(0, get_global_state()["total_unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: not enough has been staked for producers to claim rewards"),
                          push_action(producer_names.front(), N(claimrewards), mvo()("owner", producer_names.front())));
      BOOST_REQUIRE_EQUAL(0, get_balance(producer_names.front()).amount);
      BOOST_REQUIRE_EQUAL(error("condition: assertion failed: not enough has been staked for producers to claim rewards"),
                          push_action(producer_names.back(), N(claimrewards), mvo()("owner", producer_names.back())));
      BOOST_REQUIRE_EQUAL(0, get_balance(producer_names.back()).amount);
   }

   // stake across 15% boundary
   transfer(config::system_account_name, "producvoterb", "100000000.0000 EOS", config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("producvoterb", "4000000.0000 EOS", "4000000.0000 EOS"));
   transfer(config::system_account_name, "producvoterc", "100000000.0000 EOS", config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("producvoterc", "2000000.0000 EOS", "2000000.0000 EOS"));

   BOOST_REQUIRE_EQUAL(success(), push_action(N(producvoterb), N(voteproducer), mvo()
                                                ("voter",  "producvoterb")
                                                ("proxy", name(0).to_string())
                                                ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
                                                )
                        );

   BOOST_REQUIRE_EQUAL(success(), push_action(N(producvoterc), N(voteproducer), mvo()
                                                ("voter",  "producvoterc")
                                                ("proxy", name(0).to_string())
                                                ("producers", vector<account_name>(producer_names.begin(), producer_names.end()))
                                                )
                        );

   // give a chance for everyone to produce blocks
   {
      produce_blocks(21 * 12);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced= false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE_EQUAL(true, all_21_produced);
      BOOST_REQUIRE_EQUAL(true, rest_didnt_produce);
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(producer_names.front(), N(claimrewards), mvo()("owner", producer_names.front())));
      BOOST_REQUIRE(0 < get_balance(producer_names.front()).amount);
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( voters_actions_affect_proxy_and_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+6) ) try {
   create_accounts_with_resources( { N(donald111111), N(defproducer1), N(defproducer2), N(defproducer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3", 3) );

   //alice1111111 becomes a producer
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //alice1111111 makes stake and votes
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "30.0001 EOS", "20.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(voteproducer), mvo()
                                               ("voter",  "alice1111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(defproducer1), N(defproducer2) } )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("50.0002 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("50.0002 EOS") == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   BOOST_REQUIRE_EQUAL( success(), push_action( N(donald111111), N(regproxy), mvo()
                                                ("proxy",  "donald111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "donald111111" ), get_voter_info( "donald111111" ) );

   //bob111111111 chooses alice1111111 as a proxy
   issue( "bob111111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(voteproducer), mvo()
                                                ("voter",  "bob111111111")
                                                ("proxy", "alice1111111" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("150.0003 EOS") == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("200.0005 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("200.0005 EOS") == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //carol1111111 chooses alice1111111 as a proxy
   issue( "carol1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", "30.0001 EOS", "20.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol1111111), N(voteproducer), mvo()
                                                ("voter",  "carol1111111")
                                                ("proxy", "alice1111111" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("200.0005 EOS") == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("250.0007 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("250.0007 EOS") == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );


   //proxied voter carol1111111 increases stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", "50.0000 EOS", "70.0000 EOS" ) );
   BOOST_TEST_REQUIRE( stake2votes("320.0005 EOS") == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("370.0007 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("370.0007 EOS") == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //proxied voter bob111111111 decreases stake
   BOOST_REQUIRE_EQUAL( success(), unstake( "bob111111111", "50.0001 EOS", "50.0001 EOS" ) );
   BOOST_TEST_REQUIRE( stake2votes("220.0003 EOS") == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("270.0005 EOS") == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("270.0005 EOS") == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //proxied voter carol1111111 chooses another proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol1111111), N(voteproducer), mvo()
                                                ("voter",  "carol1111111")
                                                ("proxy", "donald111111" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_TEST_REQUIRE( stake2votes("50.0001 EOS"), get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("170.0002 EOS"), get_voter_info( "donald111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("100.0003 EOS"), get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("100.0003 EOS"), get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //bob111111111 switches to direct voting and votes for one of the same producers, but not for another one
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(voteproducer), mvo()
                                                ("voter",  "bob111111111")
                                                ("proxy", "")
                                                ("producers", vector<account_name>{ N(defproducer2) } )
                        )
   );
   BOOST_TEST_REQUIRE( 0.0 == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE(  stake2votes("50.0002 EOS"), get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( stake2votes("100.0003 EOS"), get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 0.0 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_both_proxy_and_producers, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //carol1111111 becomes a producer
   BOOST_REQUIRE_EQUAL( success(), regproducer( "carol1111111", 1) );

   //bob111111111 chooses alice1111111 as a proxy
   issue( "bob111111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: cannot vote for producers and proxy at same time"),
                        push_action( N(bob111111111), N(voteproducer), mvo()
                                     ("voter",  "bob111111111")
                                     ("proxy", "alice1111111" )
                                     ("producers", vector<account_name>{ N(carol1111111) } )
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( select_invalid_proxy, eosio_system_tester ) try {
   //accumulate proxied votes
   issue( "bob111111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "100.0002 EOS", "50.0001 EOS" ) );

   //selecting account not registered as a proxy
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: invalid proxy specified" ),
                        push_action(N(bob111111111), N(voteproducer), mvo()
                                    ("voter",  "bob111111111")
                                    ("proxy", "alice1111111" )
                                    ("producers", vector<account_name>() )
                        )
   );

   //selecting not existing account as a proxy
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: invalid proxy specified" ),
                        push_action(N(bob111111111), N(voteproducer), mvo()
                                    ("voter",  "bob111111111")
                                    ("proxy", "notexist" )
                                    ("producers", vector<account_name>() )
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( double_register_unregister_proxy_keeps_votes, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  1)
                        )
   );
   issue( "alice1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "5.0000 EOS", "5.0000 EOS" ) );
   edump((get_voter_info("alice1111111")));
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "staked", 100000 ), get_voter_info( "alice1111111" ) );

   //bob111111111 stakes and selects alice1111111 as a proxy
   issue( "bob111111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(voteproducer), mvo()
                                                ("voter",  "bob111111111")
                                                ("proxy", "alice1111111" )
                                                ("producers", vector<account_name>() )
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "proxied_vote_weight", stake2votes( "150.0003 EOS" ))( "staked", 100000 ), get_voter_info( "alice1111111" ) );

   //double regestering should fail without affecting total votes and stake
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: action has no effect" ),
                        push_action( N(alice1111111), N(regproxy), mvo()
                                     ("proxy",  "alice1111111")
                                     ("isproxy",  1)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "proxied_vote_weight", stake2votes("150.0003 EOS") )( "staked", 100000 ), get_voter_info( "alice1111111" ) );

   //uregister
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  0)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" )( "proxied_vote_weight", stake2votes("150.0003 EOS") )( "staked", 100000 ), get_voter_info( "alice1111111" ) );

   //double unregistering should not affect proxied_votes and stake
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: action has no effect" ),
                        push_action( N(alice1111111), N(regproxy), mvo()
                                     ("proxy",  "alice1111111")
                                     ("isproxy",  0)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" )( "proxied_vote_weight", stake2votes("150.0003 EOS"))( "staked", 100000 ), get_voter_info( "alice1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_cannot_use_another_proxy, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  1)
                        )
   );

   //bob111111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(regproxy), mvo()
                                                ("proxy",  "bob111111111")
                                                ("isproxy",  1)
                        )
   );
   //proxy should not be able to use a proxy
   issue( "bob111111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: account registered as a proxy is not allowed to use a proxy" ),
                        push_action( N(bob111111111), N(voteproducer), mvo()
                                     ("voter",  "bob111111111")
                                     ("proxy", "alice1111111" )
                                     ("producers", vector<account_name>() )
                        )
   );

   //voter that uses a proxy should not be allowed to become a proxy
   issue( "carol1111111", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol1111111", "100.0002 EOS", "50.0001 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol1111111), N(voteproducer), mvo()
                                                ("voter",  "carol1111111")
                                                ("proxy", "alice1111111" )
                                                ("producers", vector<account_name>() )
                        )
   );
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: account that uses a proxy is not allowed to become a proxy" ),
                        push_action( N(carol1111111), N(regproxy), mvo()
                                     ("proxy",  "carol1111111")
                                     ("isproxy",  1)
                        )
   );

   //proxy should not be able to use itself as a proxy
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: cannot proxy to self" ),
                        push_action( N(bob111111111), N(voteproducer), mvo()
                                     ("voter",  "bob111111111")
                                     ("proxy", "bob111111111" )
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
      ( "min_transaction_cpu_usage", config.min_transaction_cpu_usage )
      ( "max_transaction_lifetime", config.max_transaction_lifetime )
      ( "deferred_trx_expiration_window", config.deferred_trx_expiration_window )
      ( "max_transaction_delay", config.max_transaction_delay )
      ( "max_inline_action_size", config.max_inline_action_size )
      ( "max_inline_action_depth", config.max_inline_action_depth )
      ( "max_authority_depth", config.max_authority_depth )
      ( "max_generated_transaction_count", config.max_generated_transaction_count );
}

BOOST_FIXTURE_TEST_CASE( elect_producers /*_and_parameters*/, eosio_system_tester ) try {
   create_accounts_with_resources( {  N(defproducer1), N(defproducer2), N(defproducer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3", 3) );

   //stake more than 15% of total EOS supply to activate chain
   transfer( "eosio", "alice1111111", "600000000.0000 EOS", "eosio" );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", "300000000.0000 EOS", "300000000.0000 EOS" ) );
   //                                                           1000000000.0000
   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(voteproducer), mvo()
                                               ("voter",  "alice1111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(defproducer1) } )
                        )
   );
   produce_blocks(250);
   auto producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 1, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_keys[0].producer_name );

   //auto config = config_to_variant( control->get_global_properties().configuration );
   //auto prod1_config = testing::filter_fields( config, producer_parameters_example( 1 ) );
   //REQUIRE_EQUAL_OBJECTS(prod1_config, config);

   // elect 2 producers
   issue( "bob111111111", "80000.0000 EOS",  config::system_account_name );
   ilog("stake");
   BOOST_REQUIRE_EQUAL( success(), stake( "bob111111111", "40000.0000 EOS", "40000.0000 EOS" ) );
   ilog("start vote");
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob111111111), N(voteproducer), mvo()
                                               ("voter",  "bob111111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(defproducer2) } )
                        )
   );
   ilog(".");
   produce_blocks(250);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 2, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer2"), producer_keys[1].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //auto prod2_config = testing::filter_fields( config, producer_parameters_example( 2 ) );
   //REQUIRE_EQUAL_OBJECTS(prod2_config, config);

   // elect 3 producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob111111111), N(voteproducer), mvo()
                                               ("voter",  "bob111111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(defproducer2), N(defproducer3) } )
                        )
   );
   produce_blocks(250);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 3, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer2"), producer_keys[1].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer3"), producer_keys[2].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //REQUIRE_EQUAL_OBJECTS(prod2_config, config);

   //back to 2 producers
   BOOST_REQUIRE_EQUAL( success(), push_action(N(bob111111111), N(voteproducer), mvo()
                                               ("voter",  "bob111111111")
                                               ("proxy", name(0).to_string() )
                                               ("producers", vector<account_name>{ N(defproducer3) } )
                        )
   );
   produce_blocks(250);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 2, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer3"), producer_keys[1].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //auto prod3_config = testing::filter_fields( config, producer_parameters_example( 3 ) );
   //REQUIRE_EQUAL_OBJECTS(prod3_config, config);

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
