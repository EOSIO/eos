#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(eosio_system_tests)

BOOST_FIXTURE_TEST_CASE( eosio_system_load, tester ) try {

   produce_block();
   edump((name(config::system_account_name)));
   set_code(config::system_account_name, eosio_system_wast);
   set_abi(config::system_account_name, eosio_system_abi);
   produce_block();


   create_accounts( {N(dan),N(brendan)} );

   push_action(N(eosio), N(issue), N(eosio), mvo()
           ("to",       "eosio")
           ("quantity", "1000000.0000 EOS")
   );
   push_action(N(eosio), N(transfer), N(eosio), mvo()
       ("from", "eosio")
       ("to", "dan")
       ("quantity", "100.0000 EOS")
       ("memo", "hi" ) 
   );

   push_action(N(eosio), N(transfer), N(dan), mvo()
       ("from", "dan")
       ("to", "brendan")
       ("quantity", "50.0000 EOS")
       ("memo", "hi" ) 
   );

   wlog( "reg producer" );
   auto regtrace = push_action(N(eosio), N(regproducer), N(dan), mvo()
       ("producer", "dan")
       ("producer_key", "")
   );
   wdump((regtrace));
   produce_block();

   wlog( "transfer" );
   push_action(N(eosio), N(transfer), N(dan), mvo()
       ("from", "dan")
       ("to", "brendan")
       ("quantity", "5.0000 EOS")
       ("memo", "hi" ) 
   );

   /*
   permission_level_weight plw{ permission_level{N(eosio),N(active)}, 1};;
   set_authority( N(dan), N(active), 
                  authority( 1,
                   vector<key_weight>({ {get_public_key(N(dan),"active"),1 } }),
                   vector<permission_level_weight>({plw}) ) );
                   */


   wlog( "stake vote" );
   auto trace = push_action(N(eosio), N(stakevote), N(dan), mvo()
       ("voter", "dan")
       ("amount", "5.0000 EOS")
   );

   wdump((trace));

   /*

   produce_blocks(2);
   create_accounts( {N(multitest)} );
   produce_blocks(2);

   set_code( N(multitest), eosio_system_test_wast );
   set_abi( N(multitest), eosio_system_test_abi );


   produce_blocks(1);
   */

} FC_LOG_AND_RETHROW() 

BOOST_AUTO_TEST_SUITE_END()
