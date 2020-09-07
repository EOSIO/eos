#include "data_store_tests.hpp"
#include <b1/session/boost_memory_allocator.hpp>
#include <b1/session/rocks_data_store.hpp>

using namespace eosio::session;
using namespace eosio::session_tests;

namespace eosio::session_tests {

template <typename Allocator>
rocks_data_store<Allocator> make_rocks_data_store(const std::string name = "testdb")
{
  auto a = Allocator::make();
  auto rocksdb = make_rocks_db(name);
  return eosio::session::make_rocks_data_store(std::move(rocksdb), std::move(a));
}

}

BOOST_AUTO_TEST_SUITE(rocks_data_store_tests)

BOOST_AUTO_TEST_CASE(rocks_data_store_create_test) {
  {
    auto datastore1 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore1, char_key_values, string_t{});
    verify_equal(datastore1, char_key_values, string_t{});
  }

  {
    auto datastore2 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore2, int_key_values, int_t{});
    verify_equal(datastore2, int_key_values, int_t{});
  }
}

BOOST_AUTO_TEST_CASE(rocks_data_store_rwd_test) {
  {
    auto datastore1 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore1, char_key_values, string_t{});
    for (const auto& kv : char_batch_values) {
      verify_rwd(datastore1, kv);
    }
  }

  {
    auto datastore2 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore2, int_key_values, int_t{});
    for (const auto& kv : int_batch_values) {
      verify_rwd(datastore2, kv);
    }
  }
}

BOOST_AUTO_TEST_CASE(rocks_data_store_rwd_batch_test) {
  {
    auto datastore1 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore1, char_key_values, string_t{});
    verify_rwd_batch(datastore1, char_batch_values);
  }

  {
    auto datastore2 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore2, int_key_values, int_t{});
    verify_rwd_batch(datastore2, int_batch_values);
  }
}

BOOST_AUTO_TEST_CASE(rocks_data_store_rw_ds_test) {
  {
    auto datastore1 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    auto datastore2 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>("testdb2");
    make_data_store(datastore1, char_key_values, string_t{});
    verify_read_from_datastore(datastore1, datastore2);
  }

  {
    auto datastore3 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    auto datastore4 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>("testdb2");
    make_data_store(datastore3, int_key_values, int_t{});
    verify_write_to_datastore(datastore3, datastore4);
  }
}

BOOST_AUTO_TEST_CASE(rocks_data_store_iterator_test) {
  {
    auto datastore1 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore1, char_key_values, string_t{});
    verify_iterators(datastore1, string_t{});
  }
  {
    auto datastore2 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore2, char_key_values, string_t{});
    verify_iterators<const decltype(datastore2)>(datastore2, string_t{});
  }
  {
    auto datastore3 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore3, int_key_values, int_t{});
    verify_iterators(datastore3, int_t{});
  }
  {
    auto datastore4 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore4, int_key_values, int_t{});  
    verify_iterators<const decltype(datastore4)>(datastore4, int_t{});
  }
}

BOOST_AUTO_TEST_CASE(rocks_data_store_iterator_key_order_test) {
  {
    auto datastore1 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore1, char_key_values, string_t{});
    verify_key_order(datastore1);
  }
  {
    auto datastore2 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore2, char_key_values, string_t{});
    verify_key_order<const decltype(datastore2)>(datastore2);
  }
  {
    auto datastore3 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore3, int_key_values, int_t{});
    verify_key_order(datastore3);
  }
  {
    auto datastore4 = eosio::session_tests::make_rocks_data_store<boost_memory_allocator>();
    make_data_store(datastore4, int_key_values, int_t{}); 
    verify_key_order<const decltype(datastore4)>(datastore4);
  }
}

BOOST_AUTO_TEST_SUITE_END();