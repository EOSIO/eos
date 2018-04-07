#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <tic_tac_toe/tic_tac_toe.wast.hpp>
#include <tic_tac_toe/tic_tac_toe.abi.hpp>
#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>

#include <proxy/proxy.wast.hpp>
#include <proxy/proxy.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;

struct movement {
   uint32_t    row;
   uint32_t    column;

   template<typename DataStream>
   friend DataStream& operator << ( DataStream& ds, const movement& t ){ \
      return ds << t.row << t.column;
   }
   template<typename DataStream>
   friend DataStream& operator >> ( DataStream& ds, movement& t ){
      return ds >> t.row >> t.column;
   }
};

BOOST_AUTO_TEST_SUITE(tic_tac_toe_tests)

BOOST_AUTO_TEST_CASE( tic_tac_toe_game ) try {
   TESTER chain;
   abi_serializer abi_ser = json::from_string(tic_tac_toe_abi).as<abi_def>();

   chain.set_code(config::system_account_name, eosio_system_wast);
   chain.set_abi(config::system_account_name, eosio_system_abi);

   chain.produce_blocks();
   chain.create_account(N(tic.tac.toe));
   chain.produce_blocks(10);

   chain.set_code(N(tic.tac.toe), tic_tac_toe_wast);
   chain.set_abi(N(tic.tac.toe), tic_tac_toe_abi);

   chain.produce_blocks();
   chain.create_account(N(player1));
   chain.create_account(N(player2));
   chain.produce_blocks(10);

   chain.push_action(N(tic.tac.toe), N(create), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
   );

   chain.produce_blocks();

   auto mvt = mutable_variant_object()
           ("row", 1)
           ("column", 1);
   chain.push_action(N(tic.tac.toe), N(move), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player1")
           ("mvt", mvt)
   );

   mvt = mutable_variant_object()
           ("row", 0)
           ("column", 1);
   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player1), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player1")
         ("mvt", mvt)
      ), transaction_exception, assert_message_contains("not your turn"));

   mvt = mutable_variant_object()
           ("row", 1)
           ("column", 1);
   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player2")
         ("mvt", mvt)
      ), transaction_exception, assert_message_contains("not a valid movement"));

   mvt = mutable_variant_object()
           ("row", 0)
           ("column", 1);
   chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player2")
           ("mvt", mvt)
   );

   mvt = mutable_variant_object()
           ("row", 0)
           ("column", 0 );
   chain.push_action(N(tic.tac.toe), N(move), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player1")
           ("mvt", mvt)
   );

   mvt = mutable_variant_object()
           ("row", 0)
           ("column", 2);
   chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player2")
           ("mvt", mvt)
   );

   mvt = mutable_variant_object()
           ("row", 2)
           ("column", 2);
   chain.push_action(N(tic.tac.toe), N(move), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player1")
           ("mvt", mvt)
   );

   mvt = mutable_variant_object()
           ("row", 2)
           ("column", 0);
   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player2")
         ("mvt", mvt)
      ), transaction_exception, assert_message_contains("the game has ended"));

   chain.push_action(N(tic.tac.toe), N(close), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
   );

   mvt = mutable_variant_object()
           ("row", 2)
           ("column", 0);
   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player2")
         ("mvt", mvt)
      ), transaction_exception, assert_message_contains("game doesn't exists"));

   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(restart), N(player2), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player2")
      ), transaction_exception, assert_message_contains("game doesn't exists"));

   chain.push_action(N(tic.tac.toe), N(create), N(player2), mutable_variant_object()
           ("challenger", "player1")
           ("host", "player2")
   );

   chain.push_action(N(tic.tac.toe), N(restart), N(player1), mutable_variant_object()
           ("challenger", "player1")
           ("host", "player2")
           ("by", "player1")
   );

   // making a move and ...
   mvt = mutable_variant_object()
           ("row", 1)
           ("column", 1);
   chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
           ("challenger", "player1")
           ("host", "player2")
           ("by", "player2")
           ("mvt", mvt)
   );

   // ... repeating to get exception to ensure restart above actually did something
   mvt = mutable_variant_object()
           ("row", 0)
           ("column", 1);
   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
         ("challenger", "player1")
         ("host", "player2")
         ("by", "player2")
         ("mvt", mvt)
      ), transaction_exception, assert_message_contains("not your turn"));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
