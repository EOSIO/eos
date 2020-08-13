#include <boost/test/unit_test.hpp>
#include <b1/session/key_value.hpp>

#include <string>
#include <type_traits>
#include <unordered_map>

#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/slice_transform.h>

namespace b1::session_tests
{

inline auto make_rocks_db(const std::string& name = "testdb") -> std::shared_ptr<rocksdb::DB>
{
    rocksdb::DestroyDB(name.c_str(), rocksdb::Options{});

    rocksdb::DB* cache_ptr{nullptr};
    auto cache = std::shared_ptr<rocksdb::DB>{};

    auto options = rocksdb::Options{};
    options.create_if_missing = true;
    options.level_compaction_dynamic_level_bytes = true;
    options.bytes_per_sync = 1048576;
    options.OptimizeLevelStyleCompaction(256ull << 20);

    auto status = rocksdb::DB::Open(options, name.c_str(), &cache_ptr);
    cache.reset(cache_ptr);

    return cache;
}

static const std::unordered_map<std::string, std::string> char_key_values
{
  {"a", "123456789"},
  {"b", "abcdefghi"},
  {"c", "987654321"},
  {"d", "ABCDEFGHI"},
  {"e", "HELLO WORLD"},
  {"f", "Hello World"},
  {"aa", "Foo"},
  {"bb", "Bar"},
  {"cc", "FooBar"},
  {"dd", "Fizz"},
  {"ee", "Buzz"},
  {"ff", "FizzBuzz"},
  {"aaa", "qwerty"},
  {"bbb", "QWERTY"},
  {"ccc", "10101010101010"},
  {"ddd", "00000000000000"},
  {"eee", "01010101010101"},
  {"fff", "11111111111111"},
  {"aaaaa", "000000001111111"},
  {"bbbbb", "111111110000000"},
  {"ccccc", "1"},
  {"ddddd", "2"},
  {"eeeee", "3"},
  {"fffff", "5"},
  {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"},
  {"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
  {"cccccccccccccccccccccccccccccccccccccccccccccccccccccc", "dddddddddddddddddddddddddddddddddddddddddddddddddddddd"},
  {"dddddddddddddddddddddddddddddddddddddddddddddddddddddd", "cccccccccccccccccccccccccccccccccccccccccccccccccccccc"},
  {"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee", "ffffffffffffffffffffffffffffffffffffffffffffffffffffff"},
  {"ffffffffffffffffffffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffffffffffffffffffffff"},
};

static const std::vector<b1::session::key_value> char_batch_values
{
  b1::session::make_kv("hello0", 6, "world0", 6),
  b1::session::make_kv("hello1", 6, "world1", 6),
  b1::session::make_kv("hello2", 6, "world2", 6),
  b1::session::make_kv("hello3", 6, "world3", 6),
  b1::session::make_kv("hello4", 6, "world4", 6),
  b1::session::make_kv("hello5", 6, "world5", 6),
  b1::session::make_kv("hello6", 6, "world6", 6),
  b1::session::make_kv("hello7", 6, "world7", 6),
  b1::session::make_kv("hello8", 6, "world8", 6),
  b1::session::make_kv("hello9", 6, "world9", 6),
};

static const std::unordered_map<int32_t, int32_t> int_key_values
{
  {1, 1},
  {3, 2},
  {5, 3},
  {7, 4},
  {9, 5},
  {11, 6},
  {13, 7},
  {15, 8},
  {14, 9},
  {12, 10},
  {10, 11},
  {8, 12},
  {6, 13},
  {4, 14},
  {2, 15},
};

static const std::vector<int32_t> int_keys
{
  100, 
  200,
  300,
  400,
  500,
  600,
  700,
  800,
  900,
  1000,
};

static const std::vector<int32_t> int_values
{
  1000, 
  2000,
  3000,
  4000,
  5000,
  6000,
  7000,
  8000,
  9000,
  10000,
};

static const std::vector<b1::session::key_value> int_batch_values
{
  b1::session::make_kv<int32_t, int32_t>(&int_keys[0], 1, &int_values[0], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[1], 1, &int_values[1], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[2], 1, &int_values[2], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[3], 1, &int_values[3], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[4], 1, &int_values[4], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[5], 1, &int_values[5], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[6], 1, &int_values[6], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[7], 1, &int_values[7], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[8], 1, &int_values[8], 1),
  b1::session::make_kv<int32_t, int32_t>(&int_keys[9], 1, &int_values[9], 1),
};

struct string_t{};
struct int_t{};

template <typename T, typename Key, typename Value> 
auto make_data_store(T& ds, const std::unordered_map<Key, Value>& kvs, string_t) -> void
{
  for (const auto& kv : kvs)
  {
    ds.write(b1::session::make_kv(kv.first.c_str(), kv.first.size(), kv.second.c_str(), kv.second.size(), ds.memory_allocator()));
  }
}

template <typename T, typename Key, typename Value> 
auto make_data_store(T& ds, const std::unordered_map<Key, Value>& kvs, int_t) -> void
{
  for (const auto& kv : kvs)
  {
    ds.write(b1::session::make_kv(&kv.first, 1, &kv.second, 1, ds.memory_allocator()));
  }
}

template <typename T, typename Key, typename Value>
auto verify_equal(T& ds, const std::unordered_map<Key, Value>& container, string_t) -> void
{
  for (const auto& kv : ds)
  {
    auto key = std::string{reinterpret_cast<const char*>(kv.key().data()), kv.key().length()};
    auto it = container.find(key);
    BOOST_REQUIRE(it != std::end(container));
    BOOST_REQUIRE(std::memcmp(it->second.c_str(), reinterpret_cast<const char*>(kv.value().data()), it->second.size()) == 0);
  }

  for (const auto& it : container)
  {
    auto kv = ds.read(b1::session::make_bytes(it.first.c_str(), it.first.size()));
    BOOST_REQUIRE(ds.contains(kv.key()) == true);
    BOOST_REQUIRE(kv != b1::session::key_value::invalid);
    BOOST_REQUIRE(std::memcmp(it.second.c_str(), reinterpret_cast<const char*>(kv.value().data()), it.second.size()) == 0);
  }
}

template <typename T, typename Key, typename Value>
auto verify_equal(T& ds, const std::unordered_map<Key, Value>& container, int_t) -> void
{
  for (const auto& kv : ds)
  {
    auto it = container.find(*reinterpret_cast<const Key*>(kv.key().data()));
    BOOST_REQUIRE(it != std::end(container));
    BOOST_REQUIRE(std::memcmp(reinterpret_cast<const Value*>(&it->second), reinterpret_cast<const Value*>(kv.value().data()), sizeof(Value)) == 0);
  }

  for (const auto& it : container)
  {
    auto kv = ds.read(b1::session::make_bytes(&it.first, 1));
    BOOST_REQUIRE(kv != b1::session::key_value::invalid);
    BOOST_REQUIRE(ds.contains(kv.key()) == true);
    BOOST_REQUIRE(std::memcmp(reinterpret_cast<const Value*>(&it.second), reinterpret_cast<const Value*>(kv.value().data()), sizeof(Value)) == 0);
  }
}

template <typename T>
auto verify_iterators(T& ds, string_t) -> void
{
  BOOST_REQUIRE(ds.find(b1::session::make_bytes("g", 1)) == std::end(ds));
  BOOST_REQUIRE(ds.find(b1::session::make_bytes("a", 1)) != std::end(ds));
  BOOST_REQUIRE(*ds.find(b1::session::make_bytes("a", 1)) == b1::session::make_kv("a", 1, "123456789", 9));
  BOOST_REQUIRE(*std::begin(ds) == b1::session::make_kv("a", 1, "123456789", 9));
  BOOST_REQUIRE(std::begin(ds) != std::end(ds));
  BOOST_REQUIRE(*ds.lower_bound(b1::session::make_bytes("fffff", 5)) == b1::session::make_kv("fffff", 5, "5", 1));
  BOOST_REQUIRE(*ds.upper_bound(b1::session::make_bytes("fffff", 5)) 
    == b1::session::make_kv("ffffffffffffffffffffffffffffffffffffffffffffffffffffff", 54, "ffffffffffffffffffffffffffffffffffffffffffffffffffffff", 54));
}

template <typename T>
auto verify_iterators(T& ds, int_t) -> void
{
  auto search_key = int32_t{16};
  BOOST_REQUIRE(ds.find(b1::session::make_bytes(&search_key, 1)) == std::end(ds));
  search_key = 15;
  auto search_value = 8;
  BOOST_REQUIRE(ds.find(b1::session::make_bytes(&search_key, 1)) != std::end(ds));
  BOOST_REQUIRE(*ds.find(b1::session::make_bytes(&search_key, 1)) == b1::session::make_kv(&search_key, 1, &search_value, 1));
  search_key = 1;
  search_value = 1;
  BOOST_REQUIRE(*std::begin(ds) == b1::session::make_kv(&search_key, 1, &search_value, 1));
  BOOST_REQUIRE(std::begin(ds) != std::end(ds));
  search_key = 14;
  search_value = 9;
  auto result_key = int32_t{14};
  auto result_value = int32_t{9};
  BOOST_REQUIRE(*ds.lower_bound(b1::session::make_bytes(&search_key, 1)) == b1::session::make_kv(&result_key, 1, &result_value, 1));
  result_key = int32_t{15};
  result_value = int32_t{8};
  BOOST_REQUIRE(*ds.upper_bound(b1::session::make_bytes(&search_key, 1)) == b1::session::make_kv(&result_key, 1, &result_value, 1));
}

template <typename T>
auto verify_key_order(T& ds) -> void
{
  auto current_key = b1::session::bytes::invalid;
  auto compare = std::less<b1::session::bytes>{};
  for (const auto& kv : ds)
  {
    if (current_key == b1::session::bytes::invalid)
    {
      current_key = kv.key();
      continue;
    }

    BOOST_REQUIRE(compare(current_key, kv.key()) == true);
    current_key = kv.key();
  }
}

template <typename T>
auto verify_rwd(T& ds, const b1::session::key_value& kv) -> void
{
  BOOST_REQUIRE(ds.read(kv.key()) == b1::session::key_value::invalid);
  BOOST_REQUIRE(ds.contains(kv.key()) == false);

  ds.write(kv);
  BOOST_REQUIRE(ds.read(kv.key()) == kv);
  BOOST_REQUIRE(ds.contains(kv.key()) == true);

  ds.erase(kv.key());
  BOOST_REQUIRE(ds.read(kv.key()) == b1::session::key_value::invalid);
  BOOST_REQUIRE(ds.contains(kv.key()) == false);
}

template <typename T, typename iterable>
auto verify_rwd_batch(T& ds, const iterable& kvs) -> void
{
  auto keys = std::vector<b1::session::bytes>{};
  for (const auto& kv : kvs)
  {
    keys.emplace_back(kv.key());
  }

  auto [read_batch1, not_found1] = ds.read(keys);
  BOOST_REQUIRE(read_batch1.empty() == true);
  for (const auto& kv : kvs)
  {
    BOOST_REQUIRE(ds.read(kv.key()) == b1::session::key_value::invalid);
    BOOST_REQUIRE(ds.contains(kv.key()) == false);
    BOOST_REQUIRE(not_found1.find(kv.key()) != std::end(not_found1));
  }

  ds.write(kvs);
  auto [read_batch2, not_found2] = ds.read(keys);
  BOOST_REQUIRE(read_batch2.empty() == false);
  for (const auto& kv : kvs)
  {
    BOOST_REQUIRE(ds.read(kv.key()) != b1::session::key_value::invalid);
    BOOST_REQUIRE(ds.contains(kv.key()) == true);
    BOOST_REQUIRE(not_found2.find(kv.key()) == std::end(not_found2));
  }

  ds.erase(keys);
  auto [read_batch3, not_found3] = ds.read(keys);
  BOOST_REQUIRE(read_batch3.empty() == true);
  for (const auto& kv : kvs)
  {
    BOOST_REQUIRE(ds.read(kv.key()) == b1::session::key_value::invalid);
    BOOST_REQUIRE(ds.contains(kv.key()) == false);
    BOOST_REQUIRE(not_found3.find(kv.key()) != std::end(not_found3));
  }
}

template <typename T, typename U>
auto verify_read_from_datastore(T& ds, U& other_ds) -> void
{
  auto compare_ds = [](auto& left, auto& right)
  {
    // The data stores are equal if all the key_values in left are in right
    // and all the key_values in right are in left.
    for (const auto& kv : left)
    {
      BOOST_REQUIRE(right.contains(kv.key()) == true);
      BOOST_REQUIRE(right.read(kv.key()) == kv);
    }

    for (const auto& kv : right)
    {
      BOOST_REQUIRE(left.contains(kv.key()) == true);
      BOOST_REQUIRE(left.read(kv.key()) == kv);
    }
  };

  auto keys = std::vector<b1::session::bytes>{};
  for(const auto& kv : ds)
  {
    keys.emplace_back(kv.key());
  }

  other_ds.read_from(ds, keys);
  compare_ds(other_ds, ds);
}

template <typename T, typename U>
auto verify_write_to_datastore(T& ds, U& other_ds) -> void
{
  auto compare_ds = [](auto& left, auto& right)
  {
    // The data stores are equal if all the key_values in left are in right
    // and all the key_values in right are in left.
    for (const auto& kv : left)
    {
      BOOST_REQUIRE(right.contains(kv.key()) == true);
      BOOST_REQUIRE(right.read(kv.key()) == kv);
    }

    for (const auto& kv : right)
    {
      BOOST_REQUIRE(left.contains(kv.key()) == true);
      BOOST_REQUIRE(left.read(kv.key()) == kv);
    }
  };

  auto keys = std::vector<b1::session::bytes>{};
  for(const auto& kv : ds)
  {
    keys.emplace_back(kv.key());
  }

  ds.write_to(other_ds, keys);
  compare_ds(other_ds, ds);
}

}