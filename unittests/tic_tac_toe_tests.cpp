#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>

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
using namespace eosio::testing;
using namespace fc;

const static uint32_t board_len = 9;
struct game {
   account_name     challenger;
   account_name     host;
   account_name     turn; // = account name of host/ challenger
   account_name     winner = N(none); // = none/ draw/ account name of host/ challenger
   uint8_t          board[board_len];
};
FC_REFLECT(game, (challenger)(host)(turn)(winner)(board));

namespace fc {
    template<uint32_t N>
    void from_variant(const variant& v, uint8_t (&o)[N]) {
        std::vector<uint8_t> d;
        from_variant(v, d);
        BOOST_REQUIRE_EQUAL(d.size(), N);
        memcpy(o, d.data(), N);
    }
}

struct ttt_tester : TESTER {
   void get_game(game& game, account_name scope, account_name key) {
      auto value = control->chaindb().object_by_pk({N(tic.tac.toe), scope, N(games)}, key.value).value;
      fc::from_variant(value, game);
   }
};

BOOST_AUTO_TEST_SUITE(tic_tac_toe_tests)

BOOST_AUTO_TEST_CASE( tic_tac_toe_game ) try {
   ttt_tester chain;
   abi_serializer abi_ser{json::from_string(tic_tac_toe_abi).as<abi_def>(), chain.abi_serializer_max_time};
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

   chain.push_action(N(tic.tac.toe), N(move), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player1")
           ("row", 1)
           ("column", 1)
   );

   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player1), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player1")
         ("row", 0)
         ("column", 1)
      ), eosio_assert_message_exception, eosio_assert_message_starts_with("it's not your turn yet"));

   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player2")
         ("row", 1)
         ("column", 1)
      ), eosio_assert_message_exception, eosio_assert_message_starts_with("not a valid movement"));

   chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player2")
           ("row", 0)
           ("column", 1)
   );

   chain.push_action(N(tic.tac.toe), N(move), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player1")
           ("row", 0)
           ("column", 0 )
   );

   chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player2")
           ("row", 0)
           ("column", 2)
   );

   chain.push_action(N(tic.tac.toe), N(move), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
           ("by", "player1")
           ("row", 2)
           ("column", 2)
   );

   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player2")
         ("row", 2)
         ("column", 0)
      ), eosio_assert_message_exception, eosio_assert_message_starts_with("the game has ended"));

   game current;
   chain.get_game(current, N(player1), N(player2));
   BOOST_REQUIRE_EQUAL("player1", account_name(current.winner).to_string());

   chain.push_action(N(tic.tac.toe), N(close), N(player1), mutable_variant_object()
           ("challenger", "player2")
           ("host", "player1")
   );

   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player2")
         ("row", 2)
         ("column", 0)
      ), eosio_assert_message_exception, eosio_assert_message_starts_with("game doesn't exists"));

   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(restart), N(player2), mutable_variant_object()
         ("challenger", "player2")
         ("host", "player1")
         ("by", "player2")
      ), eosio_assert_message_exception, eosio_assert_message_starts_with("game doesn't exists"));

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
   chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
           ("challenger", "player1")
           ("host", "player2")
           ("by", "player2")
           ("row", 1)
           ("column", 1)
   );

   // ... repeating to get exception to ensure restart above actually did something
   BOOST_CHECK_EXCEPTION(chain.push_action(N(tic.tac.toe), N(move), N(player2), mutable_variant_object()
         ("challenger", "player1")
         ("host", "player2")
         ("by", "player2")
         ("row", 0)
         ("column", 1)
      ), eosio_assert_message_exception, eosio_assert_message_starts_with("it's not your turn yet"));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
