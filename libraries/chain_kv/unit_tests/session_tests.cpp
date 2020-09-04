#include "data_store_tests.hpp"
#include <b1/session/boost_memory_allocator.hpp>
#include <b1/session/cache.hpp>
#include <b1/session/rocks_data_store.hpp>
#include <b1/session/session.hpp>

#include <random>

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

template <typename data_store, typename container>
void verify(const data_store& ds, const container& kvs) {
    for (auto kv : ds) {
        auto current_key = *reinterpret_cast<const uint16_t*>(kv.key().data());
        auto current_value = *reinterpret_cast<const uint16_t*>(kv.value().data());

        auto it = kvs.find(current_key);
        BOOST_REQUIRE(it != std::end(kvs));
        BOOST_REQUIRE(it->first == current_key);
        BOOST_REQUIRE(it->second == current_value);
    }

    for (auto kv : kvs) {
        auto key_value = eosio::session::make_kv(&kv.first, 1, &kv.second, 1, ds.memory_allocator());
        auto result_key_value = ds.read(key_value.key());
        BOOST_REQUIRE(result_key_value != eosio::session::key_value::invalid);
        BOOST_REQUIRE(key_value == result_key_value);
    }
};

template <typename data_store, typename container>
void write(data_store& ds, const container& kvs) {
    for (auto kv : kvs) {
        ds.write(eosio::session::make_kv(&kv.first, 1, &kv.second, 1, ds.memory_allocator()));
    }
};

std::unordered_map<uint16_t, uint16_t> generate_kvs(size_t size) {
    std::random_device random_device;
    std::mt19937 generator{random_device()};
    std::uniform_int_distribution<uint16_t> distribution{0, std::numeric_limits<uint16_t>::max()};

    auto container = std::unordered_map<uint16_t, uint16_t>{};
    for (size_t i = 0; i < size; ++i) {
        container.emplace(distribution(generator), distribution(generator));
    }
    return container;
};

std::unordered_map<uint16_t, uint16_t> collapse(const std::vector<std::unordered_map<uint16_t, uint16_t>>& kvs_list) {
    if (kvs_list.empty()) {
        return std::unordered_map<uint16_t, uint16_t>{};
    }

    auto merged = kvs_list[0];
    for (size_t i = 1; i < kvs_list.size(); ++i) {
        auto& list = kvs_list[i];

        for (auto kv : list) {
            merged.insert_or_assign(kv.first, kv.second);
        }
    }
    return merged;
};

void perform_session_level_test(bool always_undo = false) {
    auto memory_allocator = boost_memory_allocator::make();
    auto cache_ds = eosio::session::make_cache(memory_allocator);
    auto rocksdb = make_rocks_db("testdb");
    auto rocks_ds = eosio::session::make_rocks_data_store(std::move(rocksdb), std::move(memory_allocator));

    auto kvs_list = std::vector<std::unordered_map<uint16_t, uint16_t>>{};

    auto root_session = eosio::session::make_session(rocks_ds, cache_ds);
    kvs_list.emplace_back(generate_kvs(50));
    write(root_session, kvs_list.back());
    verify_equal(root_session, kvs_list.back(), int_t{});
    verify_session_key_order(root_session);

    for (size_t i = 0; i < 3; ++i) {
        auto block_session = eosio::session::make_session(root_session);
        kvs_list.emplace_back(generate_kvs(50));
        write(block_session, kvs_list.back());
        verify_equal(block_session, collapse(kvs_list), int_t{});
        verify_session_key_order(block_session);

        for (size_t j = 0; j < 3; ++j) {
            auto transaction = eosio::session::make_session(block_session);
            kvs_list.emplace_back(generate_kvs(50));
            write(transaction, kvs_list.back());
            verify_equal(transaction, collapse(kvs_list), int_t{});
            verify_session_key_order(transaction);

            if (j % 2 == 1 || always_undo) {
                transaction.undo();
                kvs_list.pop_back();
            } else {
                transaction.commit();
            }

            // Verify contents of block session
            verify_equal(block_session, collapse(kvs_list), int_t{});
            verify_session_key_order(block_session);
        }

        // Verify contents of block session
        verify_equal(block_session, collapse(kvs_list), int_t{});
        verify_session_key_order(block_session);

        if (i % 2 == 1 || always_undo) {
            block_session.undo();
            kvs_list.pop_back();

            if (!always_undo) {
              // Pop the last two transaction kvs as well.
              kvs_list.pop_back();
              kvs_list.pop_back();
            }
        } else {
            block_session.commit();
        }

        // Verify contents of root session
        verify_equal(root_session, collapse(kvs_list), int_t{});
        verify_session_key_order(root_session);
    }
    // Verify contents of root session
    verify_equal(root_session, collapse(kvs_list), int_t{});
    verify_session_key_order(root_session);
    root_session.commit();

    // Verify contents of rocks ds
    verify_equal(rocks_ds, collapse(kvs_list), int_t{});
    verify_key_order(rocks_ds);
}

BOOST_AUTO_TEST_CASE(session_level_test_undo_sometimes) {
    perform_session_level_test();
}

BOOST_AUTO_TEST_CASE(session_level_test_undo_always) {
    perform_session_level_test(true);
}

BOOST_AUTO_TEST_CASE(session_level_test_attach_detach) {
    using rocks_db_type = rocks_data_store<boost_memory_allocator>;
    using cache_type = cache<boost_memory_allocator>;
    using session_type = session<rocks_db_type, cache_type>;

    auto memory_allocator = boost_memory_allocator::make();
    auto cache_ds = eosio::session::make_cache(memory_allocator);
    auto rocksdb = make_rocks_db("testdb");
    auto rocks_ds = eosio::session::make_rocks_data_store(std::move(rocksdb), std::move(memory_allocator));

    auto root_session = eosio::session::make_session(rocks_ds, cache_ds);
    auto root_session_kvs = generate_kvs(50);
    write(root_session, root_session_kvs);
    verify_equal(root_session, root_session_kvs, int_t{});
    verify_session_key_order(root_session);

    auto block_sessions = std::vector<session_type>{};
    auto block_session_kvs = std::vector<std::unordered_map<uint16_t, uint16_t>>{};
    for (size_t i = 0; i < 3; ++i) {
        block_sessions.emplace_back(eosio::session::make_session(root_session));
        block_session_kvs.emplace_back(generate_kvs(50));
        write(block_sessions.back(), block_session_kvs.back());
        verify_equal(block_sessions.back(), collapse({root_session_kvs, block_session_kvs.back()}), int_t{});
        verify_session_key_order(block_sessions.back());
        root_session.detach();
    }

    // Root session should not have changed.
    verify_equal(root_session, root_session_kvs, int_t{});

    auto transaction_sessions = std::vector<session_type>{};
    auto transaction_session_kvs = std::vector<std::unordered_map<uint16_t, uint16_t>>{};
    for (size_t i = 0; i < 3; ++i) {
        auto& block_session = block_sessions[i];
        root_session.attach(block_session);
        auto& kvs = block_session_kvs[i];

        for (size_t j = 0; j < 3; ++j) {
            transaction_sessions.emplace_back(eosio::session::make_session(block_session));
            transaction_session_kvs.emplace_back(generate_kvs(50));
            write(transaction_sessions.back(), transaction_session_kvs.back());
            verify_equal(transaction_sessions.back(), collapse({root_session_kvs, kvs, transaction_session_kvs.back()}), int_t{});
            verify_session_key_order(transaction_sessions.back());
            block_session.detach();
        }

        // Block session should not have changed.
        verify_equal(block_session, collapse({root_session_kvs, kvs}), int_t{});
        root_session.detach();
    }

    // Root session should not have changed.
    verify_equal(root_session, root_session_kvs, int_t{});

    // Attach each block and transaction, attach and commit.
    auto session_kvs = std::vector<decltype(root_session_kvs)>{};
    session_kvs.emplace_back(root_session_kvs);
    for (size_t i = 0; i < block_sessions.size(); ++i) {
        auto& block_session = block_sessions[i];
        root_session.attach(block_session);
        session_kvs.emplace_back(block_session_kvs[i]);

        for (size_t j = 3 * i; j < 3 * i + 3; ++j) {
            auto& transaction_session = transaction_sessions[j];
            block_session.attach(transaction_session);
            transaction_session.commit();
            session_kvs.emplace_back(transaction_session_kvs[j]);
        }

        block_session.commit();
    }

    // Verify contents of root session
    verify_equal(root_session, collapse(session_kvs), int_t{});
    verify_session_key_order(root_session);
    root_session.commit();

    // Verify contents of rocks ds
    verify_equal(rocks_ds, collapse(session_kvs), int_t{});
    verify_key_order(rocks_ds);
}

BOOST_AUTO_TEST_CASE(session_overwrite_key_in_child) {
    auto verify_key_value = [](auto& ds, uint16_t key, uint16_t expected_value) {
        auto key_ = eosio::session::make_bytes(&key, 1, ds.memory_allocator());
        auto value = eosio::session::make_bytes(&expected_value, 1, ds.memory_allocator());
        auto key_value = ds.read(key_);
        BOOST_REQUIRE(key_value.value() == value);

        auto begin = std::begin(ds);
        auto it = std::begin(ds);
        do {
            auto key_value = *it;
            if (key_value.key() == key_) {
                BOOST_REQUIRE(key_value.value() == value);
                break;
            }
            ++it;
        } while (it != begin);
    };

    auto memory_allocator = boost_memory_allocator::make();
    auto cache_ds = eosio::session::make_cache(memory_allocator);
    auto rocksdb = make_rocks_db("testdb");
    auto rocks_ds = eosio::session::make_rocks_data_store(std::move(rocksdb), std::move(memory_allocator));

    auto root_session = eosio::session::make_session(rocks_ds, cache_ds);
    auto root_session_kvs = std::unordered_map<uint16_t, uint16_t> {
      {0, 10},
      {1, 9},
      {2, 8},
      {3, 7},
      {4, 6},
      {5, 5},
      {6, 4},
      {7, 3},
      {8, 2},
      {9, 1},
      {10, 0}
    };
    write(root_session, root_session_kvs);
    verify_equal(root_session, root_session_kvs, int_t{});
    verify_session_key_order(root_session);

    auto block_session = eosio::session::make_session(root_session);
    auto block_session_kvs = std::unordered_map<uint16_t, uint16_t> {
      {0, 1000},
      {1, 1001},
      {2, 1002},
      {3, 1003},
      {4, 1004},
    };
    write(block_session, block_session_kvs);
    //verify_equal(root_session, root_session_kvs, int_t{});
    verify_equal(block_session, collapse({root_session_kvs, block_session_kvs}), int_t{});
    verify_session_key_order(block_session);
    verify_key_value(block_session, 0, 1000);
    verify_key_value(block_session, 1, 1001);
    verify_key_value(block_session, 2, 1002);
    verify_key_value(block_session, 3, 1003);
    verify_key_value(block_session, 4, 1004);

    auto transaction_session = eosio::session::make_session(block_session);
    auto transaction_session_kvs = std::unordered_map<uint16_t, uint16_t> {
      {0, 2000},
      {1, 2001},
      {2, 2002},
      {3, 2003},
      {4, 2004},
      {9, 2005},
      {10, 2006}
    };
    write(transaction_session, transaction_session_kvs);
    //verify_equal(root_session, root_session_kvs, int_t{});
    //verify_equal(block_session, collapse({root_session_kvs, block_session_kvs}), int_t{});
    verify_equal(transaction_session, collapse({root_session_kvs, block_session_kvs, transaction_session_kvs}), int_t{});
    verify_session_key_order(transaction_session);
    verify_key_value(transaction_session, 0, 2000);
    verify_key_value(transaction_session, 1, 2001);
    verify_key_value(transaction_session, 2, 2002);
    verify_key_value(transaction_session, 3, 2003);
    verify_key_value(transaction_session, 4, 2004);
    verify_key_value(transaction_session, 9, 2005);
    verify_key_value(transaction_session, 10, 2006);
}

BOOST_AUTO_TEST_CASE(session_delete_key_in_child) {
    auto verify_keys_deleted = [](auto& ds, const auto& keys) {
        for (const uint16_t& key : keys) {
          auto key_ = eosio::session::make_bytes(&key, 1, ds.memory_allocator());
          BOOST_REQUIRE(ds.read(key_) == eosio::session::key_value::invalid);
          BOOST_REQUIRE(ds.find(key_) == std::end(ds));
          BOOST_REQUIRE(ds.contains(key_) == false);
        }

        auto begin = std::begin(ds);
        auto it = std::begin(ds);
        do {
            auto key_value = *it;
            BOOST_REQUIRE(keys.find(*reinterpret_cast<const uint16_t*>(key_value.key().data())) == std::end(keys));
            ++it;
        } while (it != begin);
    };

    auto verify_keys_deleted_datastore = [](auto& ds, const auto& keys) {
        for (const uint16_t& key : keys) {
            auto key_ = eosio::session::make_bytes(&key, 1, ds.memory_allocator());
            BOOST_REQUIRE(ds.read(key_) == eosio::session::key_value::invalid);
            BOOST_REQUIRE(ds.find(key_) == std::end(ds));
            BOOST_REQUIRE(ds.contains(key_) == false);
        }

        for (const auto key_value : ds) {
            BOOST_REQUIRE(keys.find(*reinterpret_cast<const uint16_t*>(key_value.key().data())) == std::end(keys));
        }
    };

    auto verify_keys_exist = [](auto& ds, const auto& key_values) {
        for (const auto& key_value : key_values) {
            auto key = eosio::session::make_bytes(&key_value.first, 1, ds.memory_allocator());
            auto value = eosio::session::make_kv(&key_value.first, 1, &key_value.second, 1, ds.memory_allocator());
            BOOST_REQUIRE(ds.read(key) == value);
            BOOST_REQUIRE(ds.find(key) != std::end(ds));
            BOOST_REQUIRE(ds.contains(key) == true);
        }

        auto found = size_t{0};
        auto begin = std::begin(ds);
        auto it = std::begin(ds);
        do {
            auto key_value = *it;
            auto key = *reinterpret_cast<const uint16_t*>(key_value.key().data());
            auto value = *reinterpret_cast<const uint16_t*>(key_value.value().data());

            auto kv_it = key_values.find(key);
            if (kv_it != std::end(key_values)) {
                BOOST_REQUIRE(value == kv_it->second);
                ++found;
            }

            ++it;
        } while (it != begin);
        BOOST_REQUIRE(found == key_values.size());
    };

    auto verify_keys_exist_datastore = [](auto& ds, const auto& key_values) {
        for (const auto& key_value : key_values) {
            auto key = eosio::session::make_bytes(&key_value.first, 1, ds.memory_allocator());
            auto value = eosio::session::make_kv(&key_value.first, 1, &key_value.second, 1, ds.memory_allocator());
            BOOST_REQUIRE(ds.read(key) == value);
            BOOST_REQUIRE(ds.find(key) != std::end(ds));
            BOOST_REQUIRE(ds.contains(key) == true);
        }
        
        auto found = size_t{0};
        for (const auto key_value : ds) {
            auto key = *reinterpret_cast<const uint16_t*>(key_value.key().data());
            auto value = *reinterpret_cast<const uint16_t*>(key_value.value().data());

            auto kv_it = key_values.find(key);
            if (kv_it != std::end(key_values)) {
                BOOST_REQUIRE(value == kv_it->second);
                ++found;
            }
        }
        BOOST_REQUIRE(found == key_values.size());
    };

    auto delete_key = [](auto& ds, uint16_t key) {
        auto key_ = eosio::session::make_bytes(&key, 1, ds.memory_allocator());
        ds.erase(key_);
    };

    auto memory_allocator = boost_memory_allocator::make();
    auto cache_ds = eosio::session::make_cache(memory_allocator);
    auto rocksdb = make_rocks_db("testdb");
    auto rocks_ds = eosio::session::make_rocks_data_store(std::move(rocksdb), std::move(memory_allocator));

    auto root_session = eosio::session::make_session(rocks_ds, cache_ds);
    auto root_session_kvs = std::unordered_map<uint16_t, uint16_t> {
      {0, 10},
      {1, 9},
      {2, 8},
      {3, 7},
      {4, 6},
      {5, 5},
      {6, 4},
      {7, 3},
      {8, 2},
      {9, 1},
      {10, 0}
    };
    write(root_session, root_session_kvs);
    verify_equal(root_session, root_session_kvs, int_t{});
    verify_session_key_order(root_session);

    auto block_session = eosio::session::make_session(root_session);
    delete_key(block_session, 2);
    delete_key(block_session, 4);
    delete_key(block_session, 6);
    verify_keys_deleted(block_session, std::unordered_set<uint16_t>{2, 4, 6});
    //verify_equal(root_session, root_session_kvs, int_t{});

    auto transaction_session = eosio::session::make_session(block_session);
    auto transaction_session_kvs = std::unordered_map<uint16_t, uint16_t> {
      {2, 2003},
      {4, 2004}
    };
    write(transaction_session, transaction_session_kvs);
    verify_keys_deleted(transaction_session, std::unordered_set<uint16_t>{6});
    verify_keys_exist(transaction_session, std::unordered_map<uint16_t, uint16_t> {
      {2, 2003},
      {4, 2004}
    });
    //verify_equal(root_session, root_session_kvs, int_t{});

    transaction_session.commit();
    verify_keys_deleted(block_session, std::unordered_set<uint16_t>{6});
    verify_keys_exist(block_session, std::unordered_map<uint16_t, uint16_t> {
      {2, 2003},
      {4, 2004}
    });
    block_session.commit();
    verify_keys_deleted(root_session, std::unordered_set<uint16_t>{6});
    verify_keys_exist(root_session, std::unordered_map<uint16_t, uint16_t> {
      {2, 2003},
      {4, 2004}
    });
    root_session.commit();
    verify_keys_deleted_datastore(rocks_ds, std::unordered_set<uint16_t>{6});
    verify_keys_exist_datastore(rocks_ds, std::unordered_map<uint16_t, uint16_t> {
      {2, 2003},
      {4, 2004}
    });
}

// BOOST_AUTO_TEST_CASE(session_iteration) {
//     using rocks_db_type = rocks_data_store<boost_memory_allocator>;
//     using cache_type = cache<boost_memory_allocator>;
//     using session_type = session<rocks_db_type, cache_type>;

//     auto memory_allocator = boost_memory_allocator::make();
//     auto cache_ds = eosio::session::make_cache(memory_allocator);
//     auto rocksdb = make_rocks_db("testdb");
//     auto rocks_ds = eosio::session::make_rocks_data_store(std::move(rocksdb), std::move(memory_allocator));

//     auto root_session = eosio::session::make_session(rocks_ds, cache_ds);
//     auto root_session_kvs = generate_kvs(5000);
//     write(root_session, root_session_kvs);
//     // Commit some data to the database.
//     root_session.commit();

//     auto root_session_kvs_2 = generate_kvs(5000);
//     write(root_session, root_session_kvs_2);

//     auto block_session_kvs = generate_kvs(5000);
//     auto block_session = eosio::session::make_session(root_session);
//     write(block_session, block_session_kvs);

//     auto transaction_session_kvs = generate_kvs(5000);
//     auto transaction_session = eosio::session::make_session(block_session);
//     write(transaction_session, transaction_session_kvs);
    
//     auto set = collapse({root_session_kvs, root_session_kvs_2, block_session_kvs, transaction_session_kvs});

//     // Iterate a few times just for a time measurement.
//     for (size_t i = 0; i < 50; ++i) {
//         auto begin = std::begin(transaction_session);
//         auto current = std::begin(transaction_session);
//         do {
//         } while (++current != begin);
//     }
// }

BOOST_AUTO_TEST_SUITE_END();