#include "data_store_tests.hpp"
#include <b1/session/cache.hpp>

using namespace eosio::session;
using namespace eosio::session_tests;

BOOST_AUTO_TEST_SUITE(cache_tests)

BOOST_AUTO_TEST_CASE(cache_create_test) {
   auto cache1 = eosio::session::make_cache();
   make_data_store(cache1, char_key_values, string_t{});
   verify_equal(cache1, char_key_values, string_t{});

   auto cache2 = eosio::session::make_cache();
   make_data_store(cache2, int_key_values, int_t{});
   verify_equal(cache2, int_key_values, int_t{});
}

BOOST_AUTO_TEST_CASE(cache_rwd_test) {
   auto cache1 = eosio::session::make_cache();
   make_data_store(cache1, char_key_values, string_t{});
   for (const auto& kv : char_batch_values) { verify_rwd(cache1, kv.first, kv.second); }

   auto cache2 = eosio::session::make_cache();
   make_data_store(cache2, int_key_values, int_t{});
   for (const auto& kv : int_batch_values) { verify_rwd(cache2, kv.first, kv.second); }
}

BOOST_AUTO_TEST_CASE(cache_rwd_batch_test) {
   auto cache1 = eosio::session::make_cache();
   make_data_store(cache1, char_key_values, string_t{});
   verify_rwd_batch(cache1, char_batch_values);

   auto cache2 = eosio::session::make_cache();
   make_data_store(cache2, int_key_values, int_t{});
   verify_rwd_batch(cache2, int_batch_values);
}

BOOST_AUTO_TEST_CASE(cache_rw_ds_test) {
   auto cache1 = eosio::session::make_cache();
   auto cache2 = eosio::session::make_cache();
   make_data_store(cache1, char_key_values, string_t{});
   verify_read_from_datastore(cache1, cache2);

   auto cache3 = eosio::session::make_cache();
   auto cache4 = eosio::session::make_cache();
   make_data_store(cache3, int_key_values, int_t{});
   verify_write_to_datastore(cache3, cache4);
}

BOOST_AUTO_TEST_CASE(cache_iterator_test) {
   auto cache1 = eosio::session::make_cache();
   make_data_store(cache1, char_key_values, string_t{});
   verify_iterators(cache1, string_t{});
   auto cache2 = eosio::session::make_cache();
   make_data_store(cache2, char_key_values, string_t{});
   verify_iterators<const decltype(cache2)>(cache2, string_t{});

   auto cache3 = eosio::session::make_cache();
   make_data_store(cache3, int_key_values, int_t{});
   verify_iterators(cache3, int_t{});
   auto cache4 = eosio::session::make_cache();
   make_data_store(cache4, int_key_values, int_t{});
   verify_iterators<const decltype(cache4)>(cache4, int_t{});
}

BOOST_AUTO_TEST_CASE(cache_iterator_key_order_test) {
   auto cache1 = eosio::session::make_cache();
   make_data_store(cache1, char_key_values, string_t{});
   verify_key_order(cache1);
   auto cache2 = eosio::session::make_cache();
   make_data_store(cache2, char_key_values, string_t{});
   verify_key_order<const decltype(cache2)>(cache2);

   auto cache3 = eosio::session::make_cache();
   make_data_store(cache3, int_key_values, int_t{});
   verify_key_order(cache3);
   auto cache4 = eosio::session::make_cache();
   make_data_store(cache4, int_key_values, int_t{});
   verify_key_order<const decltype(cache4)>(cache4);
}

BOOST_AUTO_TEST_SUITE_END();
