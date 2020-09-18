#include "data_store_tests.hpp"
#include <chain_kv/cache.hpp>

using namespace eosio::chain_kv;

BOOST_AUTO_TEST_SUITE(cache_tests)

BOOST_AUTO_TEST_CASE(cache_create_test) {
   cache cache1;
   make_data_store(cache1, char_key_values, string_t{});
   verify_equal(cache1, char_key_values, string_t{});

   cache cache2;
   make_data_store(cache2, int_key_values, int_t{});
   verify_equal(cache2, int_key_values, int_t{});
}

BOOST_AUTO_TEST_CASE(cache_rwd_test) {
   cache cache1;
   make_data_store(cache1, char_key_values, string_t{});
   for (const auto& kv : char_batch_values) { verify_rwd(cache1, kv.first, kv.second); }

   cache cache2;
   make_data_store(cache2, int_key_values, int_t{});
   for (const auto& kv : int_batch_values) { verify_rwd(cache2, kv.first, kv.second); }
}

BOOST_AUTO_TEST_CASE(cache_rwd_batch_test) {
   cache cache1;
   make_data_store(cache1, char_key_values, string_t{});
   verify_rwd_batch(cache1, char_batch_values);

   cache cache2;
   make_data_store(cache2, int_key_values, int_t{});
   verify_rwd_batch(cache2, int_batch_values);
}

BOOST_AUTO_TEST_CASE(cache_rw_ds_test) {
   cache cache1, cache2;
   make_data_store(cache1, char_key_values, string_t{});
   verify_read_from_datastore(cache1, cache2);

   cache cache3, cache4;
   make_data_store(cache3, int_key_values, int_t{});
   verify_write_to_datastore(cache3, cache4);
}

BOOST_AUTO_TEST_CASE(cache_iterator_test) {
   cache cache1;
   make_data_store(cache1, char_key_values, string_t{});
   verify_iterators(cache1, string_t{});
   cache cache2;
   make_data_store(cache2, char_key_values, string_t{});
   verify_iterators<const decltype(cache2)>(cache2, string_t{});

   cache cache3;
   make_data_store(cache3, int_key_values, int_t{});
   verify_iterators(cache3, int_t{});
   cache cache4;
   make_data_store(cache4, int_key_values, int_t{});
   verify_iterators<const decltype(cache4)>(cache4, int_t{});
}

BOOST_AUTO_TEST_CASE(cache_iterator_key_order_test) {
   cache cache1;
   make_data_store(cache1, char_key_values, string_t{});
   verify_key_order(cache1);
   cache cache2;
   make_data_store(cache2, char_key_values, string_t{});
   verify_key_order<const decltype(cache2)>(cache2);

   cache cache3;
   make_data_store(cache3, int_key_values, int_t{});
   verify_key_order(cache3);
   cache cache4;
   make_data_store(cache4, int_key_values, int_t{});
   verify_key_order<const decltype(cache4)>(cache4);
}

BOOST_AUTO_TEST_SUITE_END();
