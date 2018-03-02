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

   asset get_balance( const account_name& act ) {
      return get_currency_balance( config::system_account_name, symbol(SY(4,EOS)), act );
   }

   fc::variant get_total_stake( const account_name& act ) {
      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_code_scope_table>( boost::make_tuple( config::system_account_name, act, N(totalband) ));
      FC_ASSERT(t_id != 0, "object not found");

      const auto& idx = db.get_index<key_value_index, by_scope_primary>();

      auto itr = idx.lower_bound( boost::make_tuple( t_id->id, act ));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");
      BOOST_REQUIRE_EQUAL( act.value, itr->primary_key );

      vector<char> data;
      read_only::copy_inline_row( *itr, data );
      return abi_ser.binary_to_variant( "total_resources", data );
   }

   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(eosio_system_tests)

BOOST_FIXTURE_TEST_CASE( delegate_to_myself, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );
   BOOST_REQUIRE_EQUAL( asset::from_string("1000.0000 EOS"), get_balance( "alice" ) );

   push_action(N(alice), N(delegatebw), mvo()
               ("from",     "alice")
               ("receiver", "alice")
               ("stake_net", "200.0000 EOS")
               ("stake_cpu", "100.0000 EOS")
               ("stake_storage", "300.0000 EOS")
   );

   auto stake = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS").amount, stake["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS").amount, stake["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("300.0000 EOS").amount, stake["storage_stake"].as_uint64());

   auto bytes = stake["storage_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( true, 0 < bytes );

   BOOST_REQUIRE_EQUAL( asset::from_string("400.0000 EOS"), get_balance( "alice" ) );

   //trying to unstake more net bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked net bandwidth"),
                        push_action( N(alice), N(undelegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "alice")
                                    ("unstake_net", "200.0001 EOS")
                                    ("unstake_cpu", "0.0000 EOS")
                                    ("unstake_bytes", 0) )
   );

   //trying to unstake more cpu bandwith than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked cpu bandwidth"),
                        push_action(N(alice), N(undelegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "alice")
                                    ("unstake_net", "000.0000 EOS")
                                    ("unstake_cpu", "100.0001 EOS")
                                    ("unstake_bytes", 0) )
   );

   //trying to unstake more storage than at stake
   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: insufficient staked storage"),
                        push_action(N(alice), N(undelegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "alice")
                                    ("unstake_net", "000.0000 EOS")
                                    ("unstake_cpu", "000.0001 EOS")
                                    ("unstake_bytes", bytes+1) )
   );

   //check that nothing has changed
   stake = get_total_stake( "alice" );
   BOOST_REQUIRE_EQUAL( asset::from_string("200.0000 EOS").amount, stake["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("100.0000 EOS").amount, stake["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("300.0000 EOS").amount, stake["storage_stake"].as_uint64());
   BOOST_REQUIRE_EQUAL( bytes, stake["storage_bytes"].as_uint64() );

   push_action(N(alice), N(undelegatebw), mvo()
               ("from",     "alice")
               ("receiver", "alice")
               ("unstake_net", "200.0000 EOS")
               ("unstake_cpu", "100.0000 EOS")
               ("unstake_bytes", bytes)
   );

   stake = get_total_stake( "alice" );
   std::cout << "STAKE: " << stake["net_weight"].as_uint64() << ' ' << stake["cpu_weight"].as_uint64() << std::endl;
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS").amount, stake["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 EOS").amount, stake["cpu_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( 0, stake["storage_stake"].as_uint64());

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stake_negative, eosio_system_tester ) try {
   issue( "alice", "1000.0000 EOS",  config::system_account_name );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        push_action( N(alice), N(delegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "alice")
                                    ("stake_net", "-0.0001 EOS")
                                    ("stake_cpu", "0.0000 EOS")
                                    ("stake_storage", "0.0000 EOS") )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        push_action( N(alice), N(delegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "alice")
                                    ("stake_net", "0.0000 EOS")
                                    ("stake_cpu", "-0.0001 EOS")
                                    ("stake_storage", "0.0000 EOS") )
   );

   BOOST_REQUIRE_EQUAL( error("condition: assertion failed: must stake a positive amount"),
                        push_action( N(alice), N(delegatebw), mvo()
                                    ("from",     "alice")
                                    ("receiver", "alice")
                                    ("stake_net", "0.0000 EOS")
                                    ("stake_cpu", "0.0000 EOS")
                                    ("stake_storage", "-0.0001 EOS") )
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
