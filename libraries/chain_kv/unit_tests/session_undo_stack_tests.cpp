#include "data_store_tests.hpp"
#include <b1/session/cache.hpp>
#include <b1/session/rocks_session.hpp>
#include <b1/session/session.hpp>
#include <b1/session/undo_stack.hpp>

#include <random>

using namespace eosio::session;
using namespace eosio::session_tests;

BOOST_AUTO_TEST_SUITE(session_undo_stack_tests)

BOOST_AUTO_TEST_CASE(undo_stack_test) {
   // Push the head session into the undo stack.
   auto data_store    = eosio::session::make_session(make_rocks_db());
   auto undo          = eosio::session::undo_stack(data_store);
   auto session_kvs_1 = std::unordered_map<uint16_t, uint16_t>{
      { 1, 100 }, { 2, 200 }, { 3, 300 }, { 4, 400 }, { 5, 500 },
   };
   write(data_store, session_kvs_1);
   verify_equal(data_store, session_kvs_1, int_t{});

   // Push a new session on the end of the undo stack and write some data to it
   undo.push();
   BOOST_REQUIRE(undo.revision() == 1);

   auto session_kvs_2 = std::unordered_map<uint16_t, uint16_t>{
      { 6, 600 }, { 7, 700 }, { 8, 800 }, { 9, 900 }, { 10, 1000 },
   };
   write(undo.top(), session_kvs_2);
   verify_equal(undo.top(), collapse({ session_kvs_1, session_kvs_2 }), int_t{});

   // Undo that new session
   undo.undo();
   BOOST_REQUIRE(undo.revision() == 0);
   BOOST_REQUIRE(undo.empty());
   verify_equal(data_store, session_kvs_1, int_t{});

   // Push a new session and verify.
   undo.push();
   BOOST_REQUIRE(undo.revision() == 1);
   write(undo.top(), session_kvs_2);
   verify_equal(undo.top(), collapse({ session_kvs_1, session_kvs_2 }), int_t{});

   auto session_kvs_3 = std::unordered_map<uint16_t, uint16_t>{
      { 11, 1100 }, { 12, 1200 }, { 13, 1300 }, { 14, 1400 }, { 15, 1500 },
   };
   undo.push();
   BOOST_REQUIRE(undo.revision() == 2);
   write(undo.top(), session_kvs_3);
   verify_equal(undo.top(), collapse({ session_kvs_1, session_kvs_2, session_kvs_3 }), int_t{});

   undo.squash();
   BOOST_REQUIRE(undo.revision() == 1);
   verify_equal(undo.top(), collapse({ session_kvs_1, session_kvs_2, session_kvs_3 }), int_t{});

   auto session_kvs_4 = std::unordered_map<uint16_t, uint16_t>{
      { 16, 1600 }, { 17, 1700 }, { 18, 1800 }, { 19, 1900 }, { 20, 2000 },
   };
   undo.push();
   BOOST_REQUIRE(undo.revision() == 2);
   write(undo.top(), session_kvs_4);
   verify_equal(undo.top(), collapse({ session_kvs_1, session_kvs_2, session_kvs_3, session_kvs_4 }), int_t{});

   auto session_kvs_5 = std::unordered_map<uint16_t, uint16_t>{
      { 21, 2100 }, { 22, 2200 }, { 23, 2300 }, { 24, 2400 }, { 25, 2500 },
   };
   undo.push();
   BOOST_REQUIRE(undo.revision() == 3);
   write(undo.top(), session_kvs_5);
   verify_equal(undo.top(), collapse({ session_kvs_1, session_kvs_2, session_kvs_3, session_kvs_4, session_kvs_5 }),
                int_t{});

   auto session_kvs_6 = std::unordered_map<uint16_t, uint16_t>{
      { 26, 2600 }, { 27, 2700 }, { 28, 2800 }, { 29, 2900 }, { 30, 3000 },
   };
   undo.push();
   BOOST_REQUIRE(undo.revision() == 4);
   write(undo.top(), session_kvs_6);
   verify_equal(undo.top(),
                collapse({ session_kvs_1, session_kvs_2, session_kvs_3, session_kvs_4, session_kvs_5, session_kvs_6 }),
                int_t{});

   // Commit revision 3 and verify that the top session has the correct key values.
   undo.commit(3);
   BOOST_REQUIRE(undo.revision() == 4);
   verify_equal(undo.top(), collapse({ session_kvs_1, session_kvs_2, session_kvs_3, session_kvs_4, session_kvs_5, session_kvs_6 }), int_t{});
}

BOOST_AUTO_TEST_SUITE_END();
