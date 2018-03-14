#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::chain_apis;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

class eosio_system_tester : public tester {
public:

   eosio_system_tester() {
      produce_blocks( 2 );

      create_accounts( { N(currency), N(alice), N(bob), N(carol) } );
      produce_blocks( 1000 );

      set_code( config::system_account_name, eosio_system_wast );
      set_abi( config::system_account_name, eosio_system_abi );

      //set_code( N(currency), currency_wast );
      //set_abi( N(currency), currency_abi );
      produce_blocks();

      const auto& accnt = control->get_database().get<account_object,by_name>( config::system_account_name );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi);
      /*
      const global_property_object &gpo = control->get_global_properties();
      FC_ASSERT(0 < gpo.active_producers.producers.size(), "No producers");
      producer_name = (string)gpo.active_producers.producers.front().producer_name;
      */
   }

   action_result push_action(const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = config::system_account_name;
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : 0 );
   }

   action_result stake(const account_name& from, const account_name& to, const string& net, const string& cpu, const string& storage) {
      return push_action( name(from), N(delegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net", net)
                          ("stake_cpu", cpu)
                          ("stake_storage", storage)
      );
   }

   inline action_result stake(const account_name& acnt, const string& net, const string& cpu, const string& storage) {
      return stake( acnt, acnt, net, cpu, storage);
   }

   action_result unstake(const account_name& from, const account_name& to, const string& net, const string& cpu, uint64_t bytes) {
      return push_action( name(from), N(undelegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("unstake_net", net)
                          ("unstake_cpu", cpu)
                          ("unstake_bytes", bytes)
      );
   }

   inline action_result unstake(const account_name& acnt, const string& net, const string& cpu, uint64_t bytes) {
      return unstake( acnt, acnt, net, cpu, bytes );
   }

   uint32_t last_block_time() const {
      return time_point_sec( control->head_block_time() ).sec_since_epoch();
   }

   asset get_balance( const account_name& act ) {
      return get_currency_balance( config::system_account_name, symbol(SY(4,EOS)), act );
   }

   fc::variant get_total_stake( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, act, N(totalband), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "total_resources", data );
   }

   fc::variant get_voter_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(voters), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "voter_info", data );
   }

   fc::variant get_producer_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(producerinfo), act );
      return abi_ser.binary_to_variant( "producer_info", data );
   }

   abi_serializer abi_ser;
};

fc::variant simple_voter( account_name acct, uint64_t vote_stake, uint64_t ts ) {
   return mutable_variant_object()
      ("owner", acct)
      ("proxy", name(0).to_string())
      ("last_update", ts)
      ("is_proxy", 0)
      ("staked", vote_stake)
      ("unstaking", 0)
      ("unstake_per_week", 0)
      ("proxied_votes", 0)
      ("producers", variants() )
      ("deferred_trx_id", 0)
      ("last_unstake", 0);
}

fc::variant simple_voter( account_name acct, const string& vote_stake, uint64_t ts ) {
   return simple_voter( acct, asset::from_string( vote_stake ).amount, ts);
}

fc::variant proxy( account_name acct, uint64_t vote_stake, uint64_t ts ) {
   return mutable_variant_object()
      ("owner", acct)
      ("proxy", name(0).to_string())
      ("last_update", ts)
      ("is_proxy", 1)
      ("staked", vote_stake)
      ("unstaking", 0)
      ("unstake_per_week", 0)
      ("proxied_votes", 0)
      ("producers", variants() )
      ("deferred_trx_id", 0)
      ("last_unstake", 0);
}

fc::variant proxy( account_name acct, const string& vote_stake, uint64_t ts ) {
   return proxy( acct, asset::from_string( vote_stake ).amount, ts);
}

BOOST_AUTO_TEST_SUITE(eosio_system_tests)

BOOST_FIXTURE_TEST_CASE( stake_unstake, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice" ) );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "200.0000 EOS", "100.0000 EOS", "500.0000 EOS" ) );

   auto total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("500.0000 EOS").amount, total["storage_stake"].as_uint64());
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "300.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );

   auto bytes = total["storage_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );

   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS"), get_balance( "alice" ) );

   //unstake
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "200.0000 EOS", "100.0000 EOS", bytes ) );
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS").amount, total["storage_stake"].as_uint64());
   BOOST_REQUIRE_EQUAL( 0, total["storage_bytes"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "0.00 EOS",  last_block_time() ), get_voter_info( "alice" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( fail_without_auth, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( error("missing authority of alice"),
                        push_action( N(alice), N(delegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "bob")
                                    ("stake_net", "10.0000 EOS")
                                    ("stake_cpu", "10.0000 EOS")
                                    ("stake_storage", "10.0000 EOS"),
                                    false
                        )
   );

   BOOST_REQUIRE_EQUAL( error("missing authority of alice"),
                        push_action(N(alice), N(undelegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "bob")
                                    ("unstake_net", "200.0000 EOS")
                                    ("unstake_cpu", "100.0000 EOS")
                                    ("unstake_bytes", 0)
                                    ,false
                        )
   );
   //REQUIRE_EQUAL_OBJECTS( , get_voter_info( "alice" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( stake_negative, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice", "-0.0001 EOS", "0.0000 EOS", "0.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice", "0.0000 EOS", "-0.0001 EOS", "0.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice", "00.0000 EOS", "00.0000 EOS", "0.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        stake( "alice", "0.0000 EOS", "00.0000 EOS", "0.0000 EOS" )
   );

   BOOST_REQUIRE_EQUAL( true, get_voter_info( "alice" ).is_null() );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unstake_negative, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "200.0001 EOS", "100.0001 EOS", "300.0000 EOS" ) );

   auto total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0001 EOS").amount, total["net_weight"].as_uint64());
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "300.0002 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice", "bob", "-1.0000 EOS", "0.0000 EOS", 0 )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice", "bob", "0.0000 EOS", "-1.0000 EOS", 0 )
   );

   //unstake all zeros
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must unstake a positive amount"),
                        unstake( "alice", "bob", "0.0000 EOS", "0.0000 EOS", 0 )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unstake_more_than_at_stake, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "200.0000 EOS", "100.0000 EOS", "150.0000 EOS" ) );

   auto total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("150.0000 EOS").amount, total["storage_stake"].as_uint64());
   auto bytes = total["storage_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );

   BOOST_REQUIRE_EQUAL( asset::from_string("550.0000 EOS"), get_balance( "alice" ) );

   //trying to unstake more net bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        unstake( "alice", "200.0001 EOS", "0.0000 EOS", 0 )
   );

   //trying to unstake more cpu bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        unstake( "alice", "000.0000 EOS", "100.0001 EOS", 0 )
   );

   //trying to unstake more storage than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked storage"),
                        unstake( "alice", "000.0000 EOS", "0.0000 EOS", bytes+1 )
   );

   //check that nothing has changed
   total = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("150.0000 EOS").amount, total["storage_stake"].as_uint64());
   BOOST_REQUIRE_EQUAL( bytes, total["storage_bytes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( asset::from_string("550.0000 EOS"), get_balance( "alice" ) );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( delegate_to_another_user, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( success(), stake ( "alice", "bob", "200.0000 EOS", "100.0000 EOS", "80.0000 EOS" ) );

   auto total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("80.0000 EOS").amount, total["storage_stake"].as_uint64());
   auto bytes = total["storage_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );
   BOOST_REQUIRE_EQUAL( asset::from_string("620.0000 EOS"), get_balance( "alice" ) );
   //all voting power goes to alice
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "300.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );
   //but not to bob
   BOOST_REQUIRE_EQUAL( true, get_voter_info( "bob" ).is_null() );

   //bob should not be able to unstake what was staked by alice
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: unable to find key"),
                        unstake( "bob", "0.0000 EOS", "10.0000 EOS", bytes )
   );

   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "bob", "20.0000 EOS", "10.0000 EOS", "8.0000 EOS" ) );
   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("220.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("88.0000 EOS").amount, total["storage_stake"].as_uint64());
   auto bytes2 = total["storage_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, bytes < bytes2 );
   BOOST_REQUIRE_EQUAL( asset::from_string("962.0000 EOS"), get_balance( "carol" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "carol", "30.0000 EOS",  last_block_time() ), get_voter_info( "carol" ) );

   //alice should not be able to unstake money staked by carol
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        unstake( "alice", "bob", "2001.0000 EOS", "1.0000 EOS", bytes-1 )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        unstake( "alice", "bob", "1.0000 EOS", "101.0000 EOS", bytes-1 )
   );

    BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked storage"),
                         unstake( "alice", "bob", "1.0000 EOS", "1.0000 EOS", bytes+1 )
    );

   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("220.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("110.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("88.0000 EOS").amount, total["storage_stake"].as_uint64());
   BOOST_REQUIRE_EQUAL( bytes2, total["storage_bytes"].as_uint64());
   //balance should not change after unsuccessfull attempts to unstake
   BOOST_REQUIRE_EQUAL( asset::from_string("620.0000 EOS"), get_balance( "alice" ) );
   //voting power too
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "300.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "carol", "30.0000 EOS",  last_block_time() ), get_voter_info( "carol" ) );
   BOOST_REQUIRE_EQUAL( true, get_voter_info( "bob" ).is_null() );
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( adding_stake_partial_unstake, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "200.0000 EOS", "100.0000 EOS", "80.0000 EOS" ) );

   auto total = get_total_stake( "bob" );
   auto bytes0 = total["storage_bytes"].as_uint64();
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "300.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );

   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "bob", "100.0000 EOS", "50.0000 EOS", "40.0000 EOS" ) );

   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("300.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("150.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS").amount, total["storage_stake"].as_uint64());
   auto bytes = total["storage_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, bytes0 < bytes );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "450.0000 EOS",  last_block_time() ), get_voter_info( "alice" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("430.0000 EOS"), get_balance( "alice" ) );

   //unstake a share
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "bob", "150.0000 EOS", "75.0000 EOS", bytes / 2 ) );

   total = get_total_stake( "bob" );
   BOOST_REQUIRE_EQUAL( asset::from_string("150.0000 EOS").amount, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("75.0000 EOS").amount, total["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( bytes-bytes/2, total["storage_bytes"].as_uint64() );
   BOOST_REQUIRE_EQUAL( asset::from_string("120.0000 EOS").amount - asset::from_string("120.0000 EOS").amount * (bytes/2)/bytes,
                        total["storage_stake"].as_uint64());
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "225.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("714.9599 EOS"), get_balance( "alice" ) );

} FC_LOG_AND_RETHROW()

// Tests for voting

static fc::variant producer_parameters_example(int n) {
   return mutable_variant_object()
      ("target_block_size", 1024 * 1024 + n)
      ("max_block_size", 10 * 1024 + n)
      ("target_block_acts_per_scope", 1000 + n)
      ("max_block_acts_per_scope", 10000 + n)
      ("target_block_acts", 1100 + n)
      ("max_block_acts", 11000 + n)
      ("max_storage_size", 2000 + n)
      ("max_transaction_lifetime", 3600 + n)
      ("max_transaction_exec_time", 9900 + n)
      ("max_authority_depth", 6 + n)
      ("max_inline_depth", 4 + n)
      ("max_inline_action_size", 4096 + n)
      ("max_generated_transaction_size", 64*1024 + n)
      ("inflation_rate", 1050 + n)
      ("storage_reserve_ratio", 100 + n);
}


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
   BOOST_REQUIRE_EQUAL( N(alice), info["owner"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_uint64() );
   REQUIRE_EQUAL_OBJECTS( params, info["prefs"] );
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
   BOOST_REQUIRE_EQUAL( N(alice), info["owner"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_uint64() );
   REQUIRE_EQUAL_OBJECTS( params2, info["prefs"] );
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
   BOOST_REQUIRE_EQUAL( N(alice), info["owner"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_uint64() );
   REQUIRE_EQUAL_OBJECTS( params2, info["prefs"] );

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
   BOOST_REQUIRE_EQUAL( N(alice), prod["owner"].as_uint64() );
   BOOST_REQUIRE_EQUAL( 0, prod["total_votes"].as_uint64() );
   REQUIRE_EQUAL_OBJECTS( params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(prod["packed_key"]) );

   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   issue( "carol", "3000.0000 EOS",  config::system_account_name );

   //bob makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "11.0000 EOS", "0.1111 EOS", "0.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( asset::from_string("1988.8889 EOS"), get_balance( "bob" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "bob", "11.1111 EOS",  last_block_time() ), get_voter_info( "bob" ) );

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
   BOOST_REQUIRE_EQUAL( N(alice), prod["owner"].as_uint64() );
   REQUIRE_EQUAL_OBJECTS( params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(prod["packed_key"]) );

   //carol makes stake
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "22.0000 EOS", "0.2222 EOS", "0.0000 EOS" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "carol", "22.2222 EOS",  last_block_time() ), get_voter_info( "carol" ) );
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
   BOOST_REQUIRE_EQUAL( asset::from_string("3000.0000 EOS"), get_balance( "carol" ) );
   //check that the producer parameters stay the same after all
   BOOST_REQUIRE_EQUAL( N(alice), prod["owner"].as_uint64() );
   REQUIRE_EQUAL_OBJECTS( params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(prod["packed_key"]) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unregistered_producer_voting, eosio_system_tester ) try {
   issue( "bob", "2000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "13.0000 EOS", "0.5791 EOS", "0.0000 EOS" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "bob", "13.5791 EOS",  last_block_time()-1 ), get_voter_info( "bob" ) );

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
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "13.0000 EOS", "0.5791 EOS", "0.0000 EOS" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "bob", "13.5791 EOS",  last_block_time()-1 ), get_voter_info( "bob" ) );

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
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "50.0000 EOS", "50.0000 EOS", "0.0000 EOS" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "bob", "100.0000 EOS",  last_block_time()-1 ), get_voter_info( "bob" ) );

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
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "13.0000 EOS", "0.5791 EOS", "0.0000 EOS" ) );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "bob", "13.5791 EOS",  last_block_time() ), get_voter_info( "bob" ) );

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
   REQUIRE_EQUAL_OBJECTS( params, prod["prefs"]);
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
   REQUIRE_EQUAL_OBJECTS( params, prod["prefs"]);
   BOOST_REQUIRE_EQUAL( string(key.begin(), key.end()), to_string(prod["packed_key"]) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_register_unregister, eosio_system_tester ) try {
   //register proxy by first action for this user ever
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(regproxy), mvo()
                                               ("proxy",  "alice")
                        )
   );
   REQUIRE_EQUAL_OBJECTS( proxy( "alice", "000.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice), N(unregproxy), mvo()
                                               ("proxy",  "alice")
                        )
   );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "alice", "00.000 EOS",  last_block_time() ), get_voter_info( "alice" ) );

   //stake and then register as a proxy
   issue( "bob", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "bob", "200.0002 EOS", "100.0001 EOS", "80.0000 EOS" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(regproxy), mvo()
                                               ("proxy",  "bob")
                        )
   );
   REQUIRE_EQUAL_OBJECTS( proxy( "bob", "300.0003 EOS",  last_block_time() ), get_voter_info( "bob" ) );
   //unrgister and check that stake is still in place
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob), N(unregproxy), mvo()
                                               ("proxy",  "bob")
                        )
   );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "bob", "300.0003 EOS",  last_block_time()-1 ), get_voter_info( "bob" ) );

   //register as a proxy and then stake
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(regproxy), mvo()
                                               ("proxy",  "carol")
                        )
   );
   issue( "carol", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), stake( "carol", "246.0002 EOS", "531.0001 EOS", "80.0000 EOS" ) );
   //check that both proxy flag and stake a correct
   REQUIRE_EQUAL_OBJECTS( proxy( "carol", "777.0003 EOS",  last_block_time()-1 ), get_voter_info( "carol" ) );

   //unregister
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol), N(unregproxy), mvo()
                                                ("proxy",  "carol")
                        )
   );
   REQUIRE_EQUAL_OBJECTS( simple_voter( "carol", "777.0003 EOS",  last_block_time() ), get_voter_info( "carol" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_stake_unstake_keeps_proxy_flag, eosio_system_tester ) try {
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                               ("proxy",  "alice")
                        )
   );
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   REQUIRE_EQUAL_OBJECTS( proxy( "alice", "0.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );

   //stake
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "100.0000 EOS", "50.0000 EOS", "80.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_EQUAL_OBJECTS( proxy( "alice", "150.0000 EOS",  last_block_time() ), get_voter_info( "alice" ) );

   //stake more
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "30.0000 EOS", "20.0000 EOS", "80.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_EQUAL_OBJECTS( proxy( "alice", "200.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "65.0000 EOS", "35.0000 EOS", 0 ) );
   REQUIRE_EQUAL_OBJECTS( proxy( "alice", "100.0000 EOS",  last_block_time() ), get_voter_info( "alice" ) );

   //unstake the rest
   BOOST_REQUIRE_EQUAL( success(), unstake( "alice", "65.0000 EOS", "35.0000 EOS", 0 ) );
   REQUIRE_EQUAL_OBJECTS( proxy( "alice", "0.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_multiple_users, eosio_system_tester ) try {
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice), N(regproxy), mvo()
                                               ("proxy",  "alice")
                        )
   );
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   REQUIRE_EQUAL_OBJECTS( proxy( "alice", "0.0000 EOS",  last_block_time()-1 ), get_voter_info( "alice" ) );

   //stake
   BOOST_REQUIRE_EQUAL( success(), stake( "alice", "100.0000 EOS", "50.0000 EOS", "80.0000 EOS" ) );
   //check that account is still a proxy
   REQUIRE_EQUAL_OBJECTS( proxy( "alice", "150.0000 EOS",  last_block_time() ), get_voter_info( "alice" ) );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
