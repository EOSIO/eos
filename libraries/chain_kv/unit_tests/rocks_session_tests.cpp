#include "data_store_tests.hpp"
#include <b1/session/rocks_session.hpp>

using namespace eosio::session;
using namespace eosio::session_tests;

BOOST_AUTO_TEST_SUITE(rocks_session_tests)

BOOST_AUTO_TEST_CASE(rocks_session_create_test) {
   {
      auto datastore1 = eosio::session_tests::make_session("/tmp/rocks1");
      make_data_store(datastore1, char_key_values, string_t{});
      verify_equal(datastore1, char_key_values, string_t{});
   }

   {
      auto datastore2 = eosio::session_tests::make_session("/tmp/rocks2");
      make_data_store(datastore2, int_key_values, int_t{});
      verify_equal(datastore2, int_key_values, int_t{});
   }
}

BOOST_AUTO_TEST_CASE(rocks_session_rwd_test) {
   {
      auto datastore1 = eosio::session_tests::make_session("/tmp/rocks3");
      make_data_store(datastore1, char_key_values, string_t{});
      for (const auto& kv : char_batch_values) { verify_rwd(datastore1, kv.first, kv.second); }
   }

   {
      auto datastore2 = eosio::session_tests::make_session("/tmp/rocks4");
      make_data_store(datastore2, int_key_values, int_t{});
      for (const auto& kv : int_batch_values) { verify_rwd(datastore2, kv.first, kv.second); }
   }
}

BOOST_AUTO_TEST_CASE(rocks_session_rwd_batch_test) {
   {
      auto datastore1 = eosio::session_tests::make_session("/tmp/rocks5");
      make_data_store(datastore1, char_key_values, string_t{});
      verify_rwd_batch(datastore1, char_batch_values);
   }

   {
      auto datastore2 = eosio::session_tests::make_session("/tmp/rocks6");
      make_data_store(datastore2, int_key_values, int_t{});
      verify_rwd_batch(datastore2, int_batch_values);
   }
}

BOOST_AUTO_TEST_CASE(rocks_session_rw_ds_test) {
   {
      auto datastore1 = eosio::session_tests::make_session("/tmp/rocks7");
      auto datastore2 = eosio::session_tests::make_session("/tmp/rocks8");
      make_data_store(datastore1, char_key_values, string_t{});
      verify_read_from_datastore(datastore1, datastore2);
   }

   {
      auto datastore3 = eosio::session_tests::make_session("/tmp/rocks18");
      auto datastore4 = eosio::session_tests::make_session("/tmp/rocks9");
      make_data_store(datastore3, int_key_values, int_t{});
      verify_write_to_datastore(datastore3, datastore4);
   }
}

BOOST_AUTO_TEST_CASE(rocks_session_iterator_test) {
   {
      auto datastore1 = eosio::session_tests::make_session("/tmp/rocks10");
      make_data_store(datastore1, char_key_values, string_t{});
      verify_iterators(datastore1, string_t{});
   }
   {
      auto datastore2 = eosio::session_tests::make_session("/tmp/rocks11");
      make_data_store(datastore2, char_key_values, string_t{});
      verify_iterators<const decltype(datastore2)>(datastore2, string_t{});
   }
   {
      auto datastore3 = eosio::session_tests::make_session("/tmp/rocks12");
      make_data_store(datastore3, int_key_values, int_t{});
      verify_iterators(datastore3, int_t{});
   }
   {
      auto datastore4 = eosio::session_tests::make_session("/tmp/rocks13");
      make_data_store(datastore4, int_key_values, int_t{});
      verify_iterators<const decltype(datastore4)>(datastore4, int_t{});
   }
}

BOOST_AUTO_TEST_CASE(rocks_session_iterator_key_order_test) {
   {
      auto datastore1 = eosio::session_tests::make_session("/tmp/rocks14");
      make_data_store(datastore1, char_key_values, string_t{});
      verify_key_order(datastore1);
   }
   {
      auto datastore2 = eosio::session_tests::make_session("/tmp/rocks15");
      make_data_store(datastore2, char_key_values, string_t{});
      verify_key_order<const decltype(datastore2)>(datastore2);
   }
   {
      auto datastore3 = eosio::session_tests::make_session("/tmp/rocks16");
      make_data_store(datastore3, int_key_values, int_t{});
      verify_key_order(datastore3);
   }
   {
      auto datastore4 = eosio::session_tests::make_session("/tmp/rocks17");
      make_data_store(datastore4, int_key_values, int_t{});
      verify_key_order<const decltype(datastore4)>(datastore4);
   }
}

BOOST_AUTO_TEST_SUITE_END();
