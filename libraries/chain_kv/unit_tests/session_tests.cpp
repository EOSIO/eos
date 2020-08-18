#include "data_store_tests.hpp"
#include <b1/session/boost_memory_allocator.hpp>
#include <b1/session/cache.hpp>
#include <b1/session/rocks_data_store.hpp>
#include <b1/session/session.hpp>

using namespace eosio::session;
using namespace eosio::session_tests;

namespace eosio::session_tests {

template <typename allocator>
eosio::session::session<rocks_data_store<allocator>, cache<allocator>> make_session(const std::string& name = "testdb")
{
    auto a = allocator::make();
    auto rocksdb = make_rocks_db(name);
    auto rocks_data_store = eosio::session::make_rocks_data_store(rocksdb, a);
    auto cache = eosio::session::make_cache(a);
    return eosio::session::make_session(std::move(rocks_data_store), std::move(cache));
}

}

BOOST_AUTO_TEST_SUITE(session_tests)

// TODO: Add specific session/undo stack related tests.

BOOST_AUTO_TEST_CASE(session_create_test) {
    {
        auto session1 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session1, char_key_values, string_t{});
        verify_equal(session1, char_key_values, string_t{});
    }
    {
        auto session2 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session2, int_key_values, int_t{});
        verify_equal(session2, int_key_values, int_t{});
    }
}

BOOST_AUTO_TEST_CASE(session_rwd_test) {
    {
        auto session1 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session1, char_key_values, string_t{});
        for (const auto& kv : char_batch_values) {
            verify_rwd(session1, kv);
        }
    }
    {
        auto session2 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session2, int_key_values, int_t{});
        for (const auto& kv : int_batch_values) {
            verify_rwd(session2, kv);
        }
    }
}

BOOST_AUTO_TEST_CASE(session_rwd_batch_test) {
    {
        auto session1 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session1, char_key_values, string_t{});
        verify_rwd_batch(session1, char_batch_values);
    }
    {
        auto session2 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session2, int_key_values, int_t{});
        verify_rwd_batch(session2, int_batch_values);
    }
}

BOOST_AUTO_TEST_CASE(session_rw_ds_test) {
    {
        auto session1 = eosio::session_tests::make_session<boost_memory_allocator>();
        auto session2 = eosio::session_tests::make_session<boost_memory_allocator>("testdb2");
        make_data_store(session1, char_key_values, string_t{});
        verify_read_from_datastore(session1, session2);
    }
    {
        auto session3 = eosio::session_tests::make_session<boost_memory_allocator>();
        auto session4 = eosio::session_tests::make_session<boost_memory_allocator>("testdb2");
        make_data_store(session3, int_key_values, int_t{});
        verify_write_to_datastore(session3, session4);
    }
}

BOOST_AUTO_TEST_CASE(session_iterator_test) {
    {
        auto session1 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session1, char_key_values, string_t{});
        verify_iterators(session1, string_t{});
    }
    {
        auto session2 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session2, char_key_values, string_t{});
        verify_iterators<const decltype(session2)>(session2, string_t{});
    }
    {
        auto session3 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session3, int_key_values, int_t{});
        verify_iterators(session3, int_t{});
    }
    {
        auto session4 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session4, int_key_values, int_t{});  
        verify_iterators<const decltype(session4)>(session4, int_t{});
    }
}

BOOST_AUTO_TEST_CASE(session_iterator_key_order_test) {
    {
        auto session1 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session1, char_key_values, string_t{});
        verify_session_key_order(session1);
    }
    {
        auto session2 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session2, char_key_values, string_t{});
        verify_session_key_order<const decltype(session2)>(session2);
    }
    {
        auto session3 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session3, int_key_values, int_t{});
        verify_session_key_order(session3);
    }
    {
        auto session4 = eosio::session_tests::make_session<boost_memory_allocator>();
        make_data_store(session4, int_key_values, int_t{}); 
        verify_session_key_order<const decltype(session4)>(session4);
    }
}

BOOST_AUTO_TEST_CASE(session_iterate_over_multiple_levels) {
  auto memory_allocator = boost_memory_allocator::make();
  auto cache_ds = eosio::session::make_cache(memory_allocator);
  auto rocksdb = make_rocks_db("testdb");
  auto rocks_ds = eosio::session::make_rocks_data_store(std::move(rocksdb), std::move(memory_allocator));

  // {
  //     auto root_session = eosio::session::make_session(rocks_ds, cache_ds);
  //     {
  //         auto block_session = eosio::session::make_session(root_session);
  //         {
  //             auto transaction = eosio::session::make_session(block_session);
  //         }
  //         // verify contents of block
  //         {
  //             auto transaction = eosio::session::make_session(block_session);
  //         }
  //         // verify contents of block
  //         {
  //             auto transaction = eosio::session::make_session(block_session);
  //         }
  //         // verify contents of block
  //     }
  //     // Verify contents of root session
  //     {
  //         auto block_session = eosio::session::make_session(root_session);
  //     }
  //     // Verify contents of root session
  //     {
  //         auto block_session = eosio::session::make_session(root_session);
  //     }
  //     // Verify contents of root session
  // }
  // // Verify contents of rocks ds
}

BOOST_AUTO_TEST_CASE(session_iterate_over_undo) {

}

BOOST_AUTO_TEST_CASE(session_iterate_over_commit) {

}

BOOST_AUTO_TEST_SUITE_END();