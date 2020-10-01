#include "data_store_tests.hpp"
#include <b1/session/cache.hpp>
#include <b1/session/rocks_session.hpp>
#include <b1/session/session.hpp>

using namespace eosio::session;
using namespace eosio::session_tests;

namespace eosio::session_tests {

void perform_session_level_test(const std::string& dbpath, bool always_undo = false) {
   auto kvs_list     = std::vector<std::unordered_map<uint16_t, uint16_t>>{};
   auto ordered_list = std::vector<std::map<uint16_t, uint16_t>>{};

   auto root_session  = eosio::session_tests::make_session(dbpath);
   using session_type = eosio::session::session<decltype(root_session)>;
   kvs_list.emplace_back(generate_kvs(50));
   ordered_list.emplace_back(std::begin(kvs_list.back()), std::end(kvs_list.back()));
   write(root_session, kvs_list.back());
   verify_equal(root_session, kvs_list.back(), int_t{});
   verify_session_key_order(root_session);

   for (size_t i = 0; i < 3; ++i) {
      auto block_session = session_type(root_session);
      kvs_list.emplace_back(generate_kvs(50));
      ordered_list.emplace_back(std::begin(kvs_list.back()), std::end(kvs_list.back()));
      write(block_session, kvs_list.back());
      verify_equal(block_session, collapse(kvs_list), int_t{});
      verify_session_key_order(block_session);

      for (size_t j = 0; j < 3; ++j) {
         auto transaction = session_type(block_session);
         kvs_list.emplace_back(generate_kvs(50));
         ordered_list.emplace_back(std::begin(kvs_list.back()), std::end(kvs_list.back()));
         write(transaction, kvs_list.back());
         verify_equal(transaction, collapse(kvs_list), int_t{});
         verify_session_key_order(transaction);

         if (j % 2 == 1 || always_undo) {
            transaction.undo();
            kvs_list.pop_back();
         } else {
            transaction.commit();
            transaction.detach();
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
         block_session.detach();
      }

      // Verify contents of root session
      verify_equal(root_session, collapse(kvs_list), int_t{});
      verify_session_key_order(root_session);
   }
   // Verify contents of root session
   verify_equal(root_session, collapse(kvs_list), int_t{});
   verify_session_key_order(root_session);
}

} // namespace eosio::session_tests

BOOST_AUTO_TEST_SUITE(session_tests)

BOOST_AUTO_TEST_CASE(session_create_test) {
   {
      auto session1 = eosio::session_tests::make_session("/tmp/session18");
      make_data_store(session1, char_key_values, string_t{});
      verify_equal(session1, char_key_values, string_t{});
   }
   {
      auto session2 = eosio::session_tests::make_session("/tmp/session19");
      make_data_store(session2, int_key_values, int_t{});
      verify_equal(session2, int_key_values, int_t{});
   }
}

BOOST_AUTO_TEST_CASE(session_rwd_test) {
   {
      auto session1 = eosio::session_tests::make_session("/tmp/session20");
      make_data_store(session1, char_key_values, string_t{});
      for (const auto& kv : char_batch_values) { verify_rwd(session1, kv.first, kv.second); }
   }
   {
      auto session2 = eosio::session_tests::make_session("/tmp/session21");
      make_data_store(session2, int_key_values, int_t{});
      for (const auto& kv : int_batch_values) { verify_rwd(session2, kv.first, kv.second); }
   }
}

BOOST_AUTO_TEST_CASE(session_rwd_batch_test) {
   {
      auto session1 = eosio::session_tests::make_session("/tmp/session1");
      make_data_store(session1, char_key_values, string_t{});
      verify_rwd_batch(session1, char_batch_values);
   }
   {
      auto session2 = eosio::session_tests::make_session("/tmp/session2");
      make_data_store(session2, int_key_values, int_t{});
      verify_rwd_batch(session2, int_batch_values);
   }
}

BOOST_AUTO_TEST_CASE(session_rw_ds_test) {
   {
      auto session1 = eosio::session_tests::make_session("/tmp/session3");
      auto session2 = eosio::session_tests::make_session("/tmp/session4");
      make_data_store(session1, char_key_values, string_t{});
      verify_read_from_datastore(session1, session2);
   }
   {
      auto session3 = eosio::session_tests::make_session("/tmp/session5");
      auto session4 = eosio::session_tests::make_session("/tmp/session6");
      make_data_store(session3, int_key_values, int_t{});
      verify_write_to_datastore(session3, session4);
   }
}

BOOST_AUTO_TEST_CASE(session_iterator_test) {
   {
      auto session1 = eosio::session_tests::make_session("/tmp/session7");
      make_data_store(session1, char_key_values, string_t{});
      verify_iterators(session1, string_t{});
   }
   {
      auto session2 = eosio::session_tests::make_session("/tmp/session8");
      make_data_store(session2, char_key_values, string_t{});
      verify_iterators<const decltype(session2)>(session2, string_t{});
   }
   {
      auto session3 = eosio::session_tests::make_session("/tmp/session9");
      make_data_store(session3, int_key_values, int_t{});
      verify_iterators(session3, int_t{});
   }
   {
      auto session4 = eosio::session_tests::make_session("/tmp/session10");
      make_data_store(session4, int_key_values, int_t{});
      verify_iterators<const decltype(session4)>(session4, int_t{});
   }
}

BOOST_AUTO_TEST_CASE(session_iterator_key_order_test) {
   {
      auto session1 = eosio::session_tests::make_session("/tmp/session11");
      make_data_store(session1, char_key_values, string_t{});
      verify_session_key_order(session1);
   }
   {
      auto session2 = eosio::session_tests::make_session("/tmp/session12");
      make_data_store(session2, char_key_values, string_t{});
      verify_session_key_order<const decltype(session2)>(session2);
   }
   {
      auto session3 = eosio::session_tests::make_session("/tmp/session13");
      make_data_store(session3, int_key_values, int_t{});
      verify_session_key_order(session3);
   }
   {
      auto session4 = eosio::session_tests::make_session("/tmp/session14");
      make_data_store(session4, int_key_values, int_t{});
      verify_session_key_order<const decltype(session4)>(session4);
   }
}

BOOST_AUTO_TEST_CASE(session_level_test_undo_sometimes) { eosio::session_tests::perform_session_level_test("/tmp/session22"); }

BOOST_AUTO_TEST_CASE(session_level_test_undo_always) { eosio::session_tests::perform_session_level_test("/tmp/session23", true); }

BOOST_AUTO_TEST_CASE(session_level_test_attach_detach) {
   size_t key_count      = 10;
   auto   root_session   = eosio::session_tests::make_session("/tmp/session15");
   using session_type    = eosio::session::session<decltype(root_session)>;
   auto root_session_kvs = generate_kvs(key_count);
   write(root_session, root_session_kvs);
   verify_equal(root_session, root_session_kvs, int_t{});
   verify_session_key_order(root_session);

   auto block_sessions    = std::vector<session_type>{};
   auto block_session_kvs = std::vector<std::unordered_map<uint16_t, uint16_t>>{};
   for (size_t i = 0; i < 3; ++i) {
      block_sessions.emplace_back(session_type(root_session));
      block_session_kvs.emplace_back(generate_kvs(key_count));
      write(block_sessions.back(), block_session_kvs.back());
      verify_equal(block_sessions.back(), collapse({ root_session_kvs, block_session_kvs.back() }), int_t{});
      verify_session_key_order(block_sessions.back());
      block_sessions.back().detach();
   }

   // Root session should not have changed.
   verify_equal(root_session, root_session_kvs, int_t{});

   auto transaction_sessions    = std::vector<session_type>{};
   auto transaction_session_kvs = std::vector<std::unordered_map<uint16_t, uint16_t>>{};
   for (size_t i = 0; i < 3; ++i) {
      auto& block_session = block_sessions[i];

      block_session.attach(root_session);
      auto& kvs = block_session_kvs[i];

      for (size_t j = 0; j < 3; ++j) {
         transaction_sessions.emplace_back(session_type(block_session));
         transaction_session_kvs.emplace_back(generate_kvs(key_count));
         write(transaction_sessions.back(), transaction_session_kvs.back());
         verify_equal(transaction_sessions.back(), collapse({ root_session_kvs, kvs, transaction_session_kvs.back() }),
                      int_t{});
         verify_session_key_order(transaction_sessions.back());
         transaction_sessions.back().detach();
      }

      // Block session should not have changed.
      verify_equal(block_session, collapse({ root_session_kvs, kvs }), int_t{});
      block_session.detach();
   }

   // Root session should not have changed.
   verify_equal(root_session, root_session_kvs, int_t{});

   // Attach each block and transaction, attach and commit.
   auto session_kvs = std::vector<decltype(root_session_kvs)>{};
   session_kvs.emplace_back(root_session_kvs);
   for (size_t i = 0; i < block_sessions.size(); ++i) {
      auto& block_session = block_sessions[i];
      block_session.attach(root_session);
      session_kvs.emplace_back(block_session_kvs[i]);

      for (size_t j = 3 * i; j < 3 * i + 3; ++j) {
         auto& transaction_session = transaction_sessions[j];
         transaction_session.attach(block_session);
         transaction_session.commit();
         session_kvs.emplace_back(transaction_session_kvs[j]);
      }

      block_session.commit();
   }

   // Verify contents of root session
   verify_equal(root_session, collapse(session_kvs), int_t{});
   verify_session_key_order(root_session);
}

BOOST_AUTO_TEST_CASE(session_overwrite_key_in_child) {
   auto verify_key_value = [](auto& ds, uint16_t key, uint16_t expected_value) {
      auto key_       = eosio::session::shared_bytes(&key, 1);
      auto value      = eosio::session::shared_bytes(&expected_value, 1);
      auto value_read = ds.read(key_);
      BOOST_REQUIRE(value_read == value);

      auto begin = std::begin(ds);
      auto it    = std::begin(ds);
      auto end   = std::end(ds);
      do {
         if (it == end) {
            ++it;
            continue;
         }

         auto key_value = *it;
         if (key_value.first == key_) {
            BOOST_REQUIRE(key_value.second == value);
            break;
         }
         ++it;
      } while (it != begin);
   };

   auto root_session  = eosio::session_tests::make_session("/tmp/session16");
   using session_type = eosio::session::session<decltype(root_session)>;
   auto root_session_kvs =
         std::unordered_map<uint16_t, uint16_t>{ { 0, 10 }, { 1, 9 }, { 2, 8 }, { 3, 7 }, { 4, 6 }, { 5, 5 },
                                                 { 6, 4 },  { 7, 3 }, { 8, 2 }, { 9, 1 }, { 10, 0 } };
   write(root_session, root_session_kvs);
   verify_equal(root_session, root_session_kvs, int_t{});
   verify_session_key_order(root_session);

   auto block_session     = session_type(root_session);
   auto block_session_kvs = std::unordered_map<uint16_t, uint16_t>{
      { 0, 1000 }, { 1, 1001 }, { 2, 1002 }, { 3, 1003 }, { 4, 1004 },
   };
   write(block_session, block_session_kvs);
   // verify_equal(root_session, root_session_kvs, int_t{});
   verify_equal(block_session, collapse({ root_session_kvs, block_session_kvs }), int_t{});
   verify_session_key_order(block_session);
   verify_key_value(block_session, 0, 1000);
   verify_key_value(block_session, 1, 1001);
   verify_key_value(block_session, 2, 1002);
   verify_key_value(block_session, 3, 1003);
   verify_key_value(block_session, 4, 1004);

   auto transaction_session = session_type(block_session);
   auto transaction_session_kvs =
         std::unordered_map<uint16_t, uint16_t>{ { 0, 2000 }, { 1, 2001 }, { 2, 2002 }, { 3, 2003 },
                                                 { 4, 2004 }, { 9, 2005 }, { 10, 2006 } };
   write(transaction_session, transaction_session_kvs);
   // verify_equal(root_session, root_session_kvs, int_t{});
   // verify_equal(block_session, collapse({root_session_kvs, block_session_kvs}), int_t{});
   verify_equal(transaction_session, collapse({ root_session_kvs, block_session_kvs, transaction_session_kvs }),
                int_t{});
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
         auto key_ = eosio::session::shared_bytes(&key, 1);
         BOOST_REQUIRE(ds.read(key_) == eosio::session::shared_bytes::invalid());
         BOOST_REQUIRE(ds.find(key_) == std::end(ds));
         BOOST_REQUIRE(ds.contains(key_) == false);
      }

      auto begin = std::begin(ds);
      auto it    = std::begin(ds);
      auto end   = std::end(ds);
      do {
         if (it != end) {
            auto key_value = *it;
            BOOST_REQUIRE(keys.find(*reinterpret_cast<const uint16_t*>(key_value.first.data())) == std::end(keys));
         }
         ++it;
      } while (it != begin);
   };

   auto verify_keys_exist = [](auto& ds, const auto& key_values) {
      for (const auto& key_value : key_values) {
         auto key   = eosio::session::shared_bytes(&key_value.first, 1);
         auto value = eosio::session::shared_bytes(&key_value.second, 1);
         BOOST_REQUIRE(ds.read(key) == value);
         BOOST_REQUIRE(ds.find(key) != std::end(ds));
         BOOST_REQUIRE(ds.contains(key) == true);
      }

      auto found = size_t{ 0 };
      auto begin = std::begin(ds);
      auto it    = std::begin(ds);
      auto end   = std::end(ds);
      do {
         if (it == end) {
            ++it;
            continue;
         }
         auto key_value = *it;
         auto key       = *reinterpret_cast<const uint16_t*>(key_value.first.data());
         auto value     = *reinterpret_cast<const uint16_t*>(key_value.second.data());

         auto kv_it = key_values.find(key);
         if (kv_it != std::end(key_values)) {
            BOOST_REQUIRE(value == kv_it->second);
            ++found;
         }

         ++it;
      } while (it != begin);
      BOOST_REQUIRE(found == key_values.size());
   };

   auto delete_key = [](auto& ds, uint16_t key) {
      auto key_ = eosio::session::shared_bytes(&key, 1);
      ds.erase(key_);
   };

   auto root_session  = eosio::session_tests::make_session("/tmp/session17");
   using session_type = eosio::session::session<decltype(root_session)>;
   auto root_session_kvs =
         std::unordered_map<uint16_t, uint16_t>{ { 0, 10 }, { 1, 9 }, { 2, 8 }, { 3, 7 }, { 4, 6 }, { 5, 5 },
                                                 { 6, 4 },  { 7, 3 }, { 8, 2 }, { 9, 1 }, { 10, 0 } };
   write(root_session, root_session_kvs);
   verify_equal(root_session, root_session_kvs, int_t{});
   verify_session_key_order(root_session);

   auto block_session = session_type(root_session);
   delete_key(block_session, 2);
   delete_key(block_session, 4);
   delete_key(block_session, 6);
   verify_keys_deleted(block_session, std::unordered_set<uint16_t>{ 2, 4, 6 });
   // verify_equal(root_session, root_session_kvs, int_t{});

   auto transaction_session     = session_type(block_session);
   auto transaction_session_kvs = std::unordered_map<uint16_t, uint16_t>{ { 2, 2003 }, { 4, 2004 } };
   write(transaction_session, transaction_session_kvs);
   verify_keys_deleted(transaction_session, std::unordered_set<uint16_t>{ 6 });
   verify_keys_exist(transaction_session, std::unordered_map<uint16_t, uint16_t>{ { 2, 2003 }, { 4, 2004 } });
   // verify_equal(root_session, root_session_kvs, int_t{});

   transaction_session.commit();
   verify_keys_deleted(block_session, std::unordered_set<uint16_t>{ 6 });
   verify_keys_exist(block_session, std::unordered_map<uint16_t, uint16_t>{ { 2, 2003 }, { 4, 2004 } });
   block_session.commit();
   verify_keys_deleted(root_session, std::unordered_set<uint16_t>{ 6 });
   verify_keys_exist(root_session, std::unordered_map<uint16_t, uint16_t>{ { 2, 2003 }, { 4, 2004 } });
   root_session.commit();
}

// BOOST_AUTO_TEST_CASE(session_iteration) {
//     using rocks_db_type = rocks_data_store<>;
//     using cache_type = cache<>;
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
//     for (size_t i = 0; i < 500000; ++i) {
//         auto begin = std::begin(transaction_session);
//         auto current = std::begin(transaction_session);
//         do {
//         } while (++current != begin);
//     }
// }

BOOST_AUTO_TEST_SUITE_END();
