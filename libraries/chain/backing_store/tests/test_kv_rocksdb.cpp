#define BOOST_TEST_MODULE kv_rocksdb_unittests

#include <boost/test/included/unit_test.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/backing_store/kv_context_rocksdb.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>

using namespace eosio;
using namespace eosio::chain;

// Global test data
constexpr account_name receiver = N(kvrdb);
constexpr uint64_t contract = receiver.to_uint64_t();
constexpr account_name payer = N(payernam);
std::string prefix = "prefix";
constexpr uint32_t it_key_size = 10;
constexpr uint64_t default_billable_size = 12;

// limits in kv_context is initialzed as a reference.
// Declare it globally to avoid going out of scope
constexpr uint32_t max_key_size   = 100;
constexpr uint32_t max_value_size = 1020*1024;
constexpr uint32_t max_iterators  = 3;
kv_database_config limits { max_key_size, max_value_size, max_iterators };

using namespace eosio::chain::backing_store;

struct kv_rocksdb_fixture {
   struct mock_view_provider {
      mock_view_provider(kv_rocksdb_fixture& fixture)
      :fixture(fixture)
      {}

      // Must provide actual signature for methods to be mocked.
      // Through fixture, the actual test case sets the behavior
      // of mocked method as desired.
      std::shared_ptr<const bytes> get(uint64_t contract, const rocksdb::Slice& k) {
         return fixture.mock_get(contract, k);
      }

      // Those we don't care
      void set(uint64_t contract, const rocksdb::Slice& k, const rocksdb::Slice& v) {
         return;
      }
      void erase(uint64_t contract, const rocksdb::Slice& k) {
         return;
      }

      kv_rocksdb_fixture& fixture;

      class iterator {
        public:
         iterator(mock_view_provider& view, uint64_t contract, const rocksdb::Slice& prefix): view(view) {};

         // Those we need to mock
         bool is_end() const {
            return view.fixture.mock_is_end();
         }
         bool is_valid() {
            return view.fixture.mock_is_valid();
         } 
         bool is_erased() const {
            return view.fixture.mock_is_erased();
         }
         std::optional<b1::chain_kv::key_value> get_kv() const {
            return view.fixture.mock_get_kv();
         }

         // Those we don't care
         friend int compare(const iterator& a, const iterator& b) { return true; }
         iterator& operator++() { return *this; }
         iterator& operator--() { return *this; }
         void move_to_begin() { return; }
         void move_to_end() { return; }
         void lower_bound(const char* key, size_t size) { return; }
         void lower_bound(const bytes& key) { return; }

         mock_view_provider& view;
      };
   };

   // Dummy
   struct mock_write_session {
      b1::chain_kv::database& db;
      mock_write_session(b1::chain_kv::database& db): db{ db } {}

      void write_changes(b1::chain_kv::undo_stack& u) { return; }
   };

   // Dummy
   struct mock_resource_manager {
      uint64_t billable_size = default_billable_size;

      int64_t update_table_usage(account_name payer, int64_t delta, const kv_resource_trace& trace) {
         return 0;
      }
   };

   kv_rocksdb_fixture()
   {
      b1::chain_kv::database    kv_database{"kvrdb-tmp", true/*create_if_missing*/};
      b1::chain_kv::undo_stack  kv_undo_stack{kv_database, vector<char>{make_rocksdb_undo_prefix()}};
      account_name              receiver{N(kvrdb)};
      mock_resource_manager     resource_manager;
      my_kv_context = std::make_unique<kv_context_rocksdb<mock_view_provider, mock_write_session, mock_resource_manager>>(mock_view_provider(*this), kv_database, kv_undo_stack, receiver, resource_manager, limits);
   }

   // mock methods
   std::function<int64_t (uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size, account_name payer)> mock_kv_set;
   std::function<std::shared_ptr<const bytes> (uint64_t contract, const rocksdb::Slice& k)>mock_get;
   std::function<std::optional<b1::chain_kv::key_value> const ()>mock_get_kv;
   std::function<bool ()>mock_is_end;
   std::function<bool ()>mock_is_valid;
   std::function<bool ()>mock_is_erased;

   // object being tested
   std::unique_ptr<kv_context_rocksdb<mock_view_provider, mock_write_session, mock_resource_manager>> my_kv_context;

   // test case helpers
   void check_get_payer(account_name p) {
      char buf[backing_store::payer_in_value_size];
      memcpy(buf, &p, backing_store::payer_in_value_size); // copy payer to buffer
      BOOST_CHECK(get_payer(buf) == p); // read payer from the buffer and should be equal to the original
   }

   struct kv_pair {
      std::string key;
      std::string value;
   };

   enum class it_dir {
      NEXT,
      PREV
   };

   enum class it_pos {
      NOT_END,
      END
   };

   enum class it_state {
      NOT_ERASED,
      ERASED
   };

   enum class it_keys_equal {
      YES,
      NO
   };

   enum class it_key_existing {
      YES,
      NO
   };

   enum class it_exception_expected {
      YES,
      NO
   };

   void check_set_new_value(const kv_pair& kv) {
      uint32_t key_size = kv.key.size();
      uint32_t value_size = kv.value.size();
      int64_t resource_delta = default_billable_size + key_size + value_size;

      BOOST_CHECK(my_kv_context->kv_set(contract, kv.key.c_str(), key_size, kv.value.c_str(), value_size, payer) == resource_delta);
   }

   void check_set_existing_value(const kv_pair& kv, uint32_t old_raw_value_size) {
      uint32_t key_size = kv.key.size();
      uint32_t value_size = kv.value.size();
      int64_t resource_delta = value_size - (old_raw_value_size - backing_store::payer_in_value_size);

      BOOST_CHECK(my_kv_context->kv_set(contract, kv.key.c_str(), key_size, kv.value.c_str(), value_size, payer) == resource_delta);
   }

   void check_test_kv_it_status(it_state state, it_pos pos, kv_it_stat expected_status) {
      mock_is_end = [pos]( ) -> bool {
         return pos == it_pos::END;
      };

      mock_is_erased = [state]( ) -> bool {
         return state == it_state::ERASED;
      };

      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      BOOST_CHECK(it->kv_it_status() == expected_status);
   }

   void check_kv_it_move(it_dir dir, it_state state, it_pos pos, kv_it_stat expected_status)
   {
      mock_is_erased = [state]( ) -> bool {
         return state == it_state::ERASED;
      };

      mock_is_end = [pos]() -> bool {
         return pos == it_pos::END;
      };

      mock_is_valid = [pos, state]( ) -> bool {
         return state == it_state::NOT_ERASED && pos == it_pos::NOT_END;
      };

      mock_get_kv = []() -> std::optional<b1::chain_kv::key_value> {
         return b1::chain_kv::key_value {"key", "payernamMyvalue"};
      };

      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      uint32_t found_key_size, found_value_size;
      if (state == it_state::ERASED) {
         if (dir == it_dir::NEXT) {
            BOOST_CHECK_THROW(it->kv_it_next(&found_key_size, &found_value_size), kv_bad_iter);
         } else {
            BOOST_CHECK_THROW(it->kv_it_prev(&found_key_size, &found_value_size), kv_bad_iter);
         }
         return;
      }

      if (dir == it_dir::NEXT) {
         BOOST_CHECK(it->kv_it_next(&found_key_size, &found_value_size) == expected_status);
      } else {
         BOOST_CHECK(it->kv_it_prev(&found_key_size, &found_value_size) == expected_status);
      }

      if (state == it_state::NOT_ERASED && pos == it_pos::NOT_END) { // means valid
         BOOST_CHECK(found_key_size == 3);    // "key"
         BOOST_CHECK(found_value_size == 7);  // "Myvalue"
      } else {
         BOOST_CHECK(found_key_size == 0);
         BOOST_CHECK(found_value_size == 0);
      }
   }

   void check_kv_it_key(it_state state, it_key_existing key_existing, kv_it_stat expected_status)
   {
      mock_is_erased = [state]( ) -> bool {
         return state == it_state::ERASED;
      };

      std::string key = "key";
      constexpr uint32_t key_size = 3;
      b1::chain_kv::key_value sample_key_value {key, "value"};
      mock_get_kv = [key_existing, sample_key_value]( ) -> std::optional<b1::chain_kv::key_value> {
         if (key_existing == it_key_existing::YES) {
            return sample_key_value;
         } else {
            return {};
         }
      };

      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      uint32_t offset = 0;
      char dest[key_size];
      uint32_t size = key_size;
      uint32_t actual_size;

      if (state == it_state::ERASED) {
         BOOST_CHECK_THROW(it->kv_it_key(offset, dest, key_size, actual_size), kv_bad_iter);
         return;
      }

      BOOST_CHECK(it->kv_it_key(offset, dest, size, actual_size) == expected_status);

      if (expected_status == kv_it_stat::iterator_ok) {
         BOOST_CHECK(actual_size == key_size);
         BOOST_CHECK(memcmp(key.c_str(), dest, key_size) == 0);
      } else {
         BOOST_CHECK(actual_size == 0);
      }
   }

   void check_kv_it_value(it_state state, it_key_existing key_existing, kv_it_stat expected_status)
   {
      mock_is_erased = [state]( ) -> bool {
         return state == it_state::ERASED;
      };

      std::string value = "payernamMyvalue";
      constexpr uint32_t value_size = 7;  // Myvalue
      b1::chain_kv::key_value sample_key_value {"key", value};
      mock_get_kv = [key_existing, sample_key_value]( ) -> std::optional<b1::chain_kv::key_value> {
         if (key_existing == it_key_existing::YES) {
            return sample_key_value;
         } else {
            return {};
         }
      };

      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      uint32_t offset = 0;
      char dest[value_size];
      uint32_t actual_size;

      if (state == it_state::ERASED) {
         BOOST_CHECK_THROW(it->kv_it_value(offset, dest, value_size, actual_size), kv_bad_iter);
         return;
      }

      BOOST_CHECK(it->kv_it_value(offset, dest, value_size, actual_size) == expected_status);

      if (expected_status == kv_it_stat::iterator_ok) {
         BOOST_CHECK(actual_size == value_size);
         BOOST_CHECK(memcmp(value.c_str()+payer_in_value_size, dest, value_size) == 0);
      } else {
         BOOST_CHECK(actual_size == 0);
      }
   }

   void check_kv_it_compare(it_state state, it_exception_expected exception_expected, uint64_t rhs_contract)
   {
      mock_is_erased = [state]( ) -> bool {
         return state == it_state::ERASED;
      };

      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);

      uint32_t num_iterators = 5;
      uint32_t size = 10;
      kv_iterator_rocksdb<mock_view_provider> rhs {num_iterators, my_kv_context->view, rhs_contract, prefix.c_str(), size};

      if (exception_expected == it_exception_expected::YES) {
         BOOST_CHECK_THROW(it->kv_it_compare(rhs), kv_bad_iter);
      } else {
         BOOST_CHECK(it->kv_it_compare(rhs));
      }
   }

   void check_kv_it_key_compare(it_state state, it_keys_equal keys_equal, const std::string& mykey)
   {
      mock_is_erased = [state]( ) -> bool {
         return state == it_state::ERASED;
      };

      mock_get_kv = []( ) -> std::optional<b1::chain_kv::key_value> {
         return b1::chain_kv::key_value {{"key", 3}, {}};
      };

      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      if (state == it_state::ERASED) {
         BOOST_CHECK_THROW(it->kv_it_key_compare(mykey.c_str(), mykey.size()), kv_bad_iter);
      } else {
         if (keys_equal == it_keys_equal::YES) {
            BOOST_CHECK(it->kv_it_key_compare(mykey.c_str(), mykey.size()) == 0);
         } else {
            BOOST_CHECK(it->kv_it_key_compare(mykey.c_str(), mykey.size()) != 0);
         }
      }
   }
};

// Test cases started
//
BOOST_AUTO_TEST_SUITE(kv_rocksdb_unittests)

   BOOST_AUTO_TEST_CASE(test_actual_value_size)
   {
      BOOST_CHECK(actual_value_size(payer_in_value_size) == 0);
      BOOST_CHECK(actual_value_size(payer_in_value_size + 1) == 1);
      BOOST_CHECK(actual_value_size(payer_in_value_size + 10) == 10);
   }

   BOOST_AUTO_TEST_CASE(test_actual_value_size_small_value)
   {
      // any raw size less than payer_in_value_size should throw
      BOOST_CHECK_THROW(actual_value_size(payer_in_value_size - 1), kv_rocksdb_bad_value_size_exception);
   }

   BOOST_FIXTURE_TEST_CASE(test_get_payer_1_char, kv_rocksdb_fixture)
   {
      check_get_payer(N(a));
   }

   BOOST_FIXTURE_TEST_CASE(test_get_payer_4_chars, kv_rocksdb_fixture)
   {
      check_get_payer(N(abcd));
   }

   BOOST_FIXTURE_TEST_CASE(test_get_payer_8_chars, kv_rocksdb_fixture)
   {
      check_get_payer(N(abcdefg));
   }

   BOOST_AUTO_TEST_CASE(test_actual_value_start)
   {
      char buf[10]; // any size of buffer will work
      BOOST_CHECK(actual_value_start(buf) == (buf + payer_in_value_size));
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_erase, kv_rocksdb_fixture)
   {
      std::string v = "111111119876543210"; // "1111111" is payer
      mock_get = [v]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return std::make_shared<bytes>(v.data(), v.data() + v.size());
      };

      std::string key = "key";
      int64_t resource_delta = -(default_billable_size + key.size() + actual_value_size(v.size()));

      BOOST_CHECK(my_kv_context->kv_erase(contract, key.c_str(), key.size()) == resource_delta);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_erase_zero_value, kv_rocksdb_fixture)
   {
      std::string v = "11111111"; // "1111111" is payer, no value existing
      mock_get = [v]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return std::make_shared<bytes>(v.data(), v.data() + v.size());
      };

      std::string key = "key";
      int64_t resource_delta = -(default_billable_size + key.size() + actual_value_size(v.size()));

      BOOST_CHECK(my_kv_context->kv_erase(contract, key.c_str(), key.size()) == resource_delta);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_erase_key_not_exist, kv_rocksdb_fixture)
   {
      mock_get = [ ]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return nullptr;
      };

      std::string key = "key";
      BOOST_CHECK(my_kv_context->kv_erase(contract, key.c_str(), key.size()) == 0);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_erase_key_contract_not_match, kv_rocksdb_fixture)
   {
      mock_get = [ ]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return nullptr;
      };

      std::string key = "key";
      BOOST_CHECK_THROW(my_kv_context->kv_erase(contract + 1, key.c_str(), key.size()), table_operation_not_permitted);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_set_new_val, kv_rocksdb_fixture)
   {
      mock_get = []( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return nullptr; // Not found means we need to create new key
      };

      check_set_new_value({.key="1", .value=""});
      check_set_new_value({.key="1", .value="2"});
      check_set_new_value({.key="1", .value="1111222233334444"});
      check_set_new_value({.key="1234567890", .value="1111222233334444"});
      check_set_new_value({.key="1234567890", .value="1111222233334444"});
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_set_bigger_existing_val, kv_rocksdb_fixture)
   {
      std::string v = "111111112345";
      mock_get = [v]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return std::make_shared<bytes>(v.data(), v.data() + v.size());
      };

      check_set_existing_value({.key="1234567890", .value="23"}, v.size());
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_set_smaller_existing_val, kv_rocksdb_fixture)
   {
      std::string v = "1111111123";
      mock_get = [v]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return std::make_shared<bytes>(v.data(), v.data() + v.size());
      };

      check_set_existing_value({.key="1234567890", .value="2345678"}, v.size());
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_set_same_existing_val, kv_rocksdb_fixture)
   {
      std::string v = "1111111123"; // First 8 bytes are the payer
      mock_get = [v]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return std::make_shared<bytes>(v.data(), v.data() + v.size());
      };

      check_set_existing_value({.key="1234567890", .value="23"}, v.size());
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_set_key_too_large, kv_rocksdb_fixture)
   {
      std::string key = "key";
      std::string value = "value";
      uint32_t key_size = max_key_size + 1;
      uint32_t value_size = value.size();

      BOOST_CHECK_THROW(my_kv_context->kv_set(contract, key.c_str(), key_size, value.c_str(), value_size, payer), kv_limit_exceeded);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_set_value_too_large, kv_rocksdb_fixture)
   {
      std::string key = "key";
      std::string value = "value";
      uint32_t key_size = key.size();
      uint32_t value_size = max_value_size + 1; // 1 larger than max

      BOOST_CHECK_THROW(my_kv_context->kv_set(contract, key.c_str(), key_size, value.c_str(), value_size, payer), kv_limit_exceeded);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_get, kv_rocksdb_fixture)
   {
      std::string v = "1111111123"; // First 8 bytes are the payer
      mock_get = [v]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return std::make_shared<bytes>(v.data(), v.data() + v.size());
      };

      uint32_t value_size;
      BOOST_CHECK(my_kv_context->kv_get(contract, "key", 3, value_size));
      BOOST_CHECK(value_size == 2);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_get_not_existing, kv_rocksdb_fixture)
   {
      mock_get = []( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return nullptr;
      };

      uint32_t value_size;
      BOOST_CHECK(my_kv_context->kv_get(contract, "key", 3, value_size) == false);
      BOOST_CHECK(value_size == 0);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_get_data, kv_rocksdb_fixture)
   {
      std::string v = "111111119876543210"; // First 8 bytes (11111111) are the payer
      mock_get = [v]( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return std::make_shared<bytes>(v.data(), v.data() + v.size());
      };

      uint32_t value_size;
      constexpr uint32_t data_size = 10;
      char data[data_size];

      // Get the key-value first
      BOOST_CHECK(my_kv_context->kv_get(contract, "key", 3, value_size) == true);

      // offset starts at 0
      BOOST_CHECK(my_kv_context->kv_get_data(0, data, data_size) == data_size);
      BOOST_CHECK(memcmp(data, v.c_str() + payer_in_value_size, data_size) == 0);

      // offset less than actual data size
      BOOST_CHECK(my_kv_context->kv_get_data(5, data, data_size) == data_size);
      BOOST_CHECK(memcmp(data, v.c_str() + backing_store::payer_in_value_size + 5, data_size - 5) == 0);

      // offset greater than actual data size. Data should not be modified
      std::string pattern = "mypattern1";
      memcpy(data, pattern.c_str(), data_size);
      BOOST_CHECK(my_kv_context->kv_get_data(data_size+1, data, data_size) == data_size);
      BOOST_CHECK(memcmp(data, pattern.c_str(), data_size) == 0);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_get_data_no_key, kv_rocksdb_fixture)
   {
      mock_get = []( uint64_t contract, const rocksdb::Slice& k ) -> std::shared_ptr<const bytes> {
         return nullptr;
      };

      uint32_t value_size;
      constexpr uint32_t data_size = 10;
      char data[data_size];
      std::string pattern = "mypattern1";
      memcpy(data, pattern.c_str(), data_size);

      // Get the key-value first
      BOOST_CHECK(my_kv_context->kv_get(contract, "key", 3, value_size) == false);

      BOOST_CHECK(my_kv_context->kv_get_data(0, data, data_size) == 0);
      BOOST_CHECK(memcmp(data, pattern.c_str(), data_size) == 0);
      BOOST_CHECK(my_kv_context->kv_get_data(100, data, data_size) == 0); // offset too big
      BOOST_CHECK(memcmp(data, pattern.c_str(), data_size) == 0);
   }

// Iterators

   BOOST_FIXTURE_TEST_CASE(test_iter_creation_success, kv_rocksdb_fixture)
   {
      BOOST_CHECK_NO_THROW(my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size));

   }

   BOOST_FIXTURE_TEST_CASE(test_iter_creation_too_many, kv_rocksdb_fixture)
   {
      std::unique_ptr<kv_iterator> its[max_iterators + 1]; // save first max_iterators so they won't be destroyed

      // Creating max_iterators iterators
      for (auto i = 0; i < max_iterators; ++i) {
         its[i] = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      }

      // Creating one more
      BOOST_CHECK_THROW(my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size), kv_bad_iter);

   }

   BOOST_FIXTURE_TEST_CASE(test_iter_creation_key_size_too_big, kv_rocksdb_fixture)
   {
      BOOST_CHECK_NO_THROW(my_kv_context->kv_it_create(contract, prefix.c_str(), max_key_size));  // within the size
      BOOST_CHECK_THROW(my_kv_context->kv_it_create(contract, prefix.c_str(), max_key_size + 1), kv_bad_iter); // outside of the max key size

   }

   BOOST_FIXTURE_TEST_CASE(test_is_kv_chainbase_context_iterator, kv_rocksdb_fixture)
   {
      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      BOOST_CHECK(!it->is_kv_chainbase_context_iterator());
   }

   BOOST_FIXTURE_TEST_CASE(test_is_kv_rocksdb_context_iterator, kv_rocksdb_fixture)
   {
      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      BOOST_CHECK(it->is_kv_rocksdb_context_iterator());
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_compare, kv_rocksdb_fixture)
   {
      check_kv_it_compare(it_state::NOT_ERASED, it_exception_expected::NO, contract);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_compare_different_contract, kv_rocksdb_fixture)
   {
      check_kv_it_compare(it_state::NOT_ERASED, it_exception_expected::YES, contract + 1); // +1 to make contract different
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_compare_erased, kv_rocksdb_fixture)
   {
      check_kv_it_compare(it_state::ERASED, it_exception_expected::YES, contract);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_key_compare_equal, kv_rocksdb_fixture)
   {
      check_kv_it_key_compare(it_state::NOT_ERASED, it_keys_equal::YES, "key");
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_key_compare_non_equal, kv_rocksdb_fixture)
   {
      check_kv_it_key_compare(it_state::NOT_ERASED, it_keys_equal::NO, "randomkey");
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_key_compare_erased, kv_rocksdb_fixture)
   {
      check_kv_it_key_compare(it_state::ERASED, it_keys_equal::YES, "key");
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_status_end, kv_rocksdb_fixture)
   {
      check_test_kv_it_status(it_state::ERASED, it_pos::END, kv_it_stat::iterator_end);
      check_test_kv_it_status(it_state::NOT_ERASED, it_pos::END, kv_it_stat::iterator_end);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_status_erased, kv_rocksdb_fixture)
   {
      check_test_kv_it_status(it_state::ERASED, it_pos::NOT_END, kv_it_stat::iterator_erased);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_status_ok, kv_rocksdb_fixture)
   {
      check_test_kv_it_status(it_state::NOT_ERASED, it_pos::NOT_END, kv_it_stat::iterator_ok);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_move_to_end, kv_rocksdb_fixture)
   {
      mock_is_erased = []( ) -> bool {
         return false;
      };
      std::unique_ptr<kv_iterator> it = my_kv_context->kv_it_create(contract, prefix.c_str(), it_key_size);
      BOOST_CHECK(it->kv_it_move_to_end() == kv_it_stat::iterator_end);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_next_erased, kv_rocksdb_fixture)
   {
      check_kv_it_move(it_dir::NEXT, it_state::ERASED, it_pos::NOT_END, kv_it_stat::iterator_ok);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_next_noterased_notend, kv_rocksdb_fixture)
   {
      check_kv_it_move(it_dir::NEXT, it_state::NOT_ERASED, it_pos::NOT_END, kv_it_stat::iterator_ok);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_next_noterased_end, kv_rocksdb_fixture)
   {
      check_kv_it_move(it_dir::NEXT, it_state::NOT_ERASED, it_pos::END, kv_it_stat::iterator_end);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_prev_erased, kv_rocksdb_fixture)
   {
      check_kv_it_move(it_dir::PREV, it_state::ERASED, it_pos::NOT_END, kv_it_stat::iterator_ok);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_prev_noterased_notend, kv_rocksdb_fixture)
   {
      check_kv_it_move(it_dir::PREV, it_state::NOT_ERASED, it_pos::NOT_END, kv_it_stat::iterator_ok);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_prev_noterased_end, kv_rocksdb_fixture)
   {
      check_kv_it_move(it_dir::PREV, it_state::NOT_ERASED, it_pos::END, kv_it_stat::iterator_end);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_key_has_key, kv_rocksdb_fixture)
   {
      check_kv_it_key(it_state::NOT_ERASED, it_key_existing::YES, kv_it_stat::iterator_ok);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_key_no_key, kv_rocksdb_fixture)
   {
      check_kv_it_key(it_state::NOT_ERASED, it_key_existing::NO, kv_it_stat::iterator_end);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_key_erased, kv_rocksdb_fixture)
   {
      check_kv_it_key(it_state::ERASED, it_key_existing::NO, kv_it_stat::iterator_end);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_value_has_key, kv_rocksdb_fixture)
   {
      check_kv_it_value(it_state::NOT_ERASED, it_key_existing::YES, kv_it_stat::iterator_ok);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_value_no_key, kv_rocksdb_fixture)
   {
      check_kv_it_value(it_state::NOT_ERASED, it_key_existing::NO, kv_it_stat::iterator_end);
   }

   BOOST_FIXTURE_TEST_CASE(test_kv_it_value_erased, kv_rocksdb_fixture)
   {
      check_kv_it_value(it_state::ERASED, it_key_existing::NO, kv_it_stat::iterator_end);
   }

BOOST_AUTO_TEST_SUITE_END()
