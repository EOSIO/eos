#pragma once

#include <eosio/chain/name.hpp>
#include <chain_kv/chain_kv.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <memory>
#include <stdint.h>

namespace eosio { namespace chain { namespace backing_store { namespace db_key_value_format {
   using key256_t = std::array<eosio::chain::uint128_t, 2>;
   constexpr uint64_t prefix_size = sizeof(name) * 2; // 8 (scope) + 8 (table)
   b1::chain_kv::bytes create_primary_key(name scope, name table, uint64_t primary_key);

   bool get_primary_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table, uint64_t& primary_key);

   // ordering affects key ordering
   enum class key_type : char {
      primary = 0,
      primary_to_sec = 1, // used to lookup the secondary keys for a specific primary key
      sec_i64 = 2,
      sec_i128 = 3,
      sec_i256 = 4,
      sec_double = 5,
      sec_long_double = 6, // changes needed in determine_sec_type to accommodate new types
      table = std::numeric_limits<char>::max() // require to be highest value for a given scope/table
   };

   enum class decomposed_types { scope = 0, table, key_loc, type_of_key};
   using intermittent_decomposed_values = std::tuple<name, name, b1::chain_kv::bytes::const_iterator, key_type>;

   namespace detail {

      template<typename Key>
      struct determine_sec_type {};

      template<>
      struct determine_sec_type<uint64_t> { static constexpr key_type kt = key_type::sec_i64; };

      template<>
      struct determine_sec_type<eosio::chain::uint128_t> { static constexpr key_type kt = key_type::sec_i128; };

      template<>
      struct determine_sec_type<key256_t> { static constexpr key_type kt = key_type::sec_i256; };

      template<>
      struct determine_sec_type<float64_t> { static constexpr key_type kt = key_type::sec_double; };

      template<>
      struct determine_sec_type<float128_t> { static constexpr key_type kt = key_type::sec_long_double; };

      std::string to_string(const key_type& kt);

      b1::chain_kv::bytes prepare_composite_key_prefix(name scope, name table, std::size_t type_size, std::size_t key_size, std::size_t extension_size);

      b1::chain_kv::bytes prepare_composite_key(name scope, name table, std::size_t key_size, key_type kt, std::size_t extension_size);

      using intermittent_decomposed_prefix_values = std::tuple<name, name, b1::chain_kv::bytes::const_iterator>;

      intermittent_decomposed_prefix_values extract_from_composite_key_prefix(b1::chain_kv::bytes::const_iterator begin, b1::chain_kv::bytes::const_iterator key_end);

      intermittent_decomposed_values extract_from_composite_key(b1::chain_kv::bytes::const_iterator begin, b1::chain_kv::bytes::const_iterator key_end);

      template<typename Key>
      void append_identifier_and_key(b1::chain_kv::bytes& composite_key, const Key& some_key, key_type kt = detail::determine_sec_type<Key>::kt) {
         composite_key.push_back(static_cast<const char>(kt));
         b1::chain_kv::append_key(composite_key, some_key);
      }

      template<typename Key>
      void append_primary_and_secondary_key_types(b1::chain_kv::bytes& composite_key) {
         composite_key.push_back(static_cast<const char>(key_type::primary_to_sec));
         const key_type kt = determine_sec_type<Key>::kt;
         composite_key.push_back(static_cast<const char>(kt));
      }

      template<typename Key>
      void append_primary_and_secondary_keys(b1::chain_kv::bytes& composite_key, uint64_t primary_key, const Key& sec_key) {
         append_primary_and_secondary_key_types<Key>(composite_key);
         b1::chain_kv::append_key(composite_key, primary_key);
         b1::chain_kv::append_key(composite_key, sec_key);
      }

      template<typename Key, typename CharKey1, typename CharKey2>
      bool verify_primary_to_sec_type(const CharKey1& key, const CharKey2& type_prefix) {
         const auto prefix_minimum_size = prefix_size + sizeof(key_type);
         const auto both_types_size = prefix_minimum_size + sizeof(key_type);
         const auto both_types_and_primary_size = both_types_size + sizeof(uint64_t);
         const bool has_sec_type = type_prefix.size() >= both_types_size;
         EOS_ASSERT(type_prefix.size() == prefix_minimum_size || type_prefix.size() == both_types_size ||
                    type_prefix.size() == both_types_and_primary_size, bad_composite_key_exception,
                    "DB intrinsic key-value verify_primary_to_sec_type was passed a prefix key size: ${s1} bytes that was not either: "
                    "${s2}, ${s3}, or ${s4} bytes long",
                    ("s1", type_prefix.size())("s2", prefix_minimum_size)("s3", both_types_and_primary_size)
                    ("s4", both_types_and_primary_size));
         const char* prefix_ptr = type_prefix.data() + prefix_size;
         const key_type prefix_type_kt {*(prefix_ptr++)};
         EOS_ASSERT(key_type::primary_to_sec == prefix_type_kt, bad_composite_key_exception,
                    "DB intrinsic key-value verify_primary_to_sec_type was passed a prefix that was the wrong type: ${type},"
                    " it should have been of type: ${type2}",
                    ("type", to_string(prefix_type_kt))("type2", to_string(key_type::primary_to_sec)));
         if (has_sec_type) {
            const key_type expected_sec_kt = determine_sec_type<Key>::kt;
            const key_type prefix_sec_kt {*(prefix_ptr)};
            EOS_ASSERT(expected_sec_kt == prefix_sec_kt, bad_composite_key_exception,
                       "DB intrinsic key-value verify_primary_to_sec_type was passed a prefix that was the wrong secondary type: ${type},"
                       " it should have been of type: ${type2}",
                       ("type", to_string(prefix_sec_kt))("type2", to_string(expected_sec_kt)));
         }
         if (memcmp(type_prefix.data(), key.data(), type_prefix.size()) != 0) {
            return false;
         }

         const auto minimum_size = prefix_size + sizeof(key_type) + sizeof(key_type) + sizeof(uint64_t);
         EOS_ASSERT(key.size() >= minimum_size, bad_composite_key_exception,
                    "DB intrinsic key-value verify_primary_to_sec_type was passed a full key size: ${s1} bytes that was not at least "
                    "larger than the size that would include the secondary type: ${s2}",
                    ("s1", key.size())("s2", minimum_size));
         return true;
      }
   }

   template<typename Key>
   constexpr key_type derive_secondary_key_type() {
      return detail::determine_sec_type<Key>::kt;
   }
   template<typename Key>
   b1::chain_kv::bytes create_secondary_key(name scope, name table, const Key& sec_key, uint64_t primary_key) {
      const key_type kt = detail::determine_sec_type<Key>::kt;
      b1::chain_kv::bytes composite_key = detail::prepare_composite_key(scope, table, sizeof(Key), kt, sizeof(primary_key));
      b1::chain_kv::append_key(composite_key, sec_key);
      b1::chain_kv::append_key(composite_key, primary_key);
      return composite_key;
   }

   template<key_type kt>
   b1::chain_kv::bytes create_prefix_type_key(name scope, name table) {
      static constexpr std::size_t no_key_size = 0;
      static constexpr std::size_t no_extension_size = 0;
      b1::chain_kv::bytes composite_key = detail::prepare_composite_key(scope, table, no_key_size, kt, no_extension_size);
      return composite_key;
   }

   template<typename Key>
   b1::chain_kv::bytes create_prefix_secondary_key(name scope, name table, const Key& sec_key) {
      const key_type kt = detail::determine_sec_type<Key>::kt;
      b1::chain_kv::bytes composite_key = detail::prepare_composite_key(scope, table, sizeof(Key), kt, 0);
      b1::chain_kv::append_key(composite_key, sec_key);
      return composite_key;
   }

   template<typename Key>
   bool get_secondary_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table, Key& sec_key, uint64_t& primary_key) {
      b1::chain_kv::bytes::const_iterator composite_loc;
      key_type kt = key_type::sec_i64;
      std::tie(scope, table, composite_loc, kt) = detail::extract_from_composite_key(composite_key.cbegin(), composite_key.cend());
      constexpr static auto expected_kt = detail::determine_sec_type<Key>::kt;
      if (kt != expected_kt) {
         return false;
      }
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), sec_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have an underlying: " +
                 detail::to_string(kt));
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have a trailing primary key");
      return true;
   }

   struct secondary_key_pair {
      b1::chain_kv::bytes secondary_key;
      b1::chain_kv::bytes primary_to_secondary_key;
   };

   template<typename Key>
   secondary_key_pair create_secondary_key_pair(name scope, name table, const Key& sec_key, uint64_t primary_key) {
      b1::chain_kv::bytes secondary_key = create_secondary_key(scope, table, sec_key, primary_key);
      b1::chain_kv::bytes primary_to_secondary_key(secondary_key.begin(), secondary_key.begin() + prefix_size);
      detail::append_primary_and_secondary_keys(primary_to_secondary_key, primary_key, sec_key);
      return { std::move(secondary_key), std::move(primary_to_secondary_key) };
   }

   template<typename Key, typename CharKey1, typename CharKey2>
   bool get_trailing_sec_prim_keys(const CharKey1& full_key, const CharKey2& secondary_type_prefix, Key& sec_key, uint64_t& primary_key) {
      const auto sec_type_prefix_size = secondary_type_prefix.size();
      const auto prefix_type_size = prefix_size + sizeof(key_type);
      EOS_ASSERT(sec_type_prefix_size == prefix_type_size, bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_sec_prim_keys was passed a secondary_type_prefix with key size: ${s1} bytes "
                 "which is not equal to the expected size: ${s2}", ("s1", sec_type_prefix_size)("s2", prefix_type_size));
      const auto keys_size = sizeof(sec_key) + sizeof(primary_key);
      EOS_ASSERT(full_key.size() == sec_type_prefix_size + keys_size, bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_sec_prim_keys was passed a full key size: ${s1} bytes that was not exactly "
                 "${s2} bytes (the size of the secondary key and a primary key) larger than the secondary_type_prefix "
                 "size: ${s3}", ("s1", full_key.size())("s2", keys_size)("s3", sec_type_prefix_size));
      const auto comp = memcmp(secondary_type_prefix.data(), full_key.data(), sec_type_prefix_size);
      if (comp != 0) {
         return false;
      }
      const key_type expected_kt = detail::determine_sec_type<Key>::kt;
      const key_type actual_kt {*(secondary_type_prefix.data() + sec_type_prefix_size - 1)};
      EOS_ASSERT(expected_kt == actual_kt, bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_sec_prim_keys was passed a key that was the wrong type: ${type},"
                 " it should have been of type: ${type2}",
                 ("type", detail::to_string(actual_kt))("type2", detail::to_string(expected_kt)));
      auto start_offset = full_key.data() + sec_type_prefix_size;
      const b1::chain_kv::bytes composite_trailing_sec_prim_key {start_offset, start_offset + keys_size};
      auto composite_loc = composite_trailing_sec_prim_key.cbegin();
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_trailing_sec_prim_key.cend(), sec_key), bad_composite_key_exception,
                 "DB intrinsic key-value store invariant has changed, extract_key should only fail if the remaining "
                 "string size is less than: ${s} bytes, which was already verified", ("s", keys_size));
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_trailing_sec_prim_key.cend(), primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store invariant has changed, extract_key should only fail if the remaining "
                 "string size is less than the sizeof(uint64_t)");
      return true;
   }

   bool get_trailing_primary_key(const rocksdb::Slice& full_key, const rocksdb::Slice& secondary_key_prefix, uint64_t& primary_key);

   template<typename Key>
   b1::chain_kv::bytes create_primary_to_secondary_key(name scope, name table, uint64_t primary_key, const Key& sec_key) {
      constexpr static auto type_size = sizeof(key_type);
      auto composite_key = detail::prepare_composite_key_prefix(scope, table, type_size * 2, sizeof(primary_key), sizeof(sec_key));
      detail::append_primary_and_secondary_keys(composite_key, primary_key, sec_key);
      return composite_key;
   }

   template<typename Key>
   b1::chain_kv::bytes create_prefix_primary_to_secondary_key(name scope, name table, uint64_t primary_key) {
      constexpr static auto type_size = sizeof(key_type);
      constexpr static auto zero_size = 0;
      auto composite_key = detail::prepare_composite_key_prefix(scope, table, type_size * 2, sizeof(primary_key), zero_size);
      detail::append_primary_and_secondary_key_types<Key>(composite_key);
      b1::chain_kv::append_key(composite_key, primary_key);
      return composite_key;
   }

   template<typename Key>
   b1::chain_kv::bytes create_prefix_primary_to_secondary_key(name scope, name table) {
      constexpr static auto type_size = sizeof(key_type);
      constexpr static auto zero_size = 0;
      auto composite_key = detail::prepare_composite_key_prefix(scope, table, type_size * 2, zero_size, zero_size);
      detail::append_primary_and_secondary_key_types<Key>(composite_key);
      return composite_key;
   }

   enum class prim_to_sec_type_result { invalid_type, wrong_sec_type, valid_type };
   template<typename Key>
   prim_to_sec_type_result get_primary_to_secondary_keys(const b1::chain_kv::bytes& composite_key, name& scope, name& table, uint64_t& primary_key, Key& sec_key) {
      b1::chain_kv::bytes::const_iterator composite_loc;
      key_type kt = key_type::sec_i64;
      std::tie(scope, table, composite_loc, kt) = detail::extract_from_composite_key(composite_key.cbegin(), composite_key.cend());
      if (kt != key_type::primary_to_sec) {
         return prim_to_sec_type_result::invalid_type;
      }
      EOS_ASSERT(composite_loc != composite_key.cend(), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it does not contain an indication of the "
                 "type of the secondary-key");
      const key_type sec_kt{*composite_loc++};
      constexpr static auto expected_kt = detail::determine_sec_type<Key>::kt;
      if (sec_kt != expected_kt) {
         return prim_to_sec_type_result::wrong_sec_type;
      }
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have a trailing primary key");

      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), sec_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have an underlying: " +
                 detail::to_string(sec_kt));
      return prim_to_sec_type_result::valid_type;
   }

   template<typename Key, typename CharKey1, typename CharKey2>
   bool get_trailing_prim_sec_keys(const CharKey1& full_key, const CharKey2& primary_to_sec_type_prefix,
                                   uint64_t& primary_key, Key& sec_key) {
      const auto primary_to_sec_type_prefix_size = primary_to_sec_type_prefix.size();
      const auto prefix_type_size = prefix_size + sizeof(key_type);
      const bool extract_sec_type = primary_to_sec_type_prefix_size == prefix_type_size;
      const auto prefix_sec_type_size = prefix_type_size + sizeof(key_type);
      EOS_ASSERT(extract_sec_type || primary_to_sec_type_prefix_size == prefix_sec_type_size, bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_prim_sec_keys was passed a primary_to_sec_type_prefix with key size: ${s1} bytes "
                 "which is not equal to the expected size: ${s2}",
                 ("s1", primary_to_sec_type_prefix_size)("s2", prefix_sec_type_size));
      if (!detail::verify_primary_to_sec_type<Key>(full_key, primary_to_sec_type_prefix)) {
         return false;
      }
      const auto keys_size = sizeof(uint64_t) + sizeof(Key);
      auto start_offset = full_key.data() + prefix_sec_type_size;
      const b1::chain_kv::bytes composite_trailing_prim_sec_key {start_offset, start_offset + keys_size};
      auto composite_loc = composite_trailing_prim_sec_key.cbegin();
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_trailing_prim_sec_key.cend(), primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store invariant has changed, extract_key should only fail if the remaining "
                 "string size is less than the sizeof(uint64_t)");
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_trailing_prim_sec_key.cend(), sec_key), bad_composite_key_exception,
                 "DB intrinsic key-value store invariant has changed, extract_key should only fail if the remaining "
                 "string size is less than: ${s} bytes, which was already verified", ("s", keys_size));
      return true;
   }

   template<typename Key, typename CharKey1, typename CharKey2>
   bool get_trailing_secondary_key(const CharKey1& full_key, const CharKey2& sec_type_trailing_prefix, Key& sec_key) {
      const auto sec_type_trailing_prefix_size = sec_type_trailing_prefix.size();
      const auto prefix_with_primary_key_size = prefix_size + sizeof(key_type) * 2 + sizeof(uint64_t);
      EOS_ASSERT(sec_type_trailing_prefix_size == prefix_with_primary_key_size, bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_secondary_key was passed a sec_type_trailing_prefix with key size: ${s1} bytes "
                 "which is not equal to the expected size: ${s2}", ("s1", sec_type_trailing_prefix_size)("s2", prefix_with_primary_key_size));
      if (!detail::verify_primary_to_sec_type<Key>(full_key, sec_type_trailing_prefix)) {
         return false;
      }
      const auto key_size = sizeof(Key);
      auto start_offset = full_key.data() + sec_type_trailing_prefix_size;
      const b1::chain_kv::bytes composite_trailing_prim_sec_key {start_offset, start_offset + key_size};
      auto composite_loc = composite_trailing_prim_sec_key.cbegin();
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_trailing_prim_sec_key.cend(), sec_key), bad_composite_key_exception,
                 "DB intrinsic key-value store invariant has changed, extract_key should only fail if the remaining "
                 "string size is less than: ${s} bytes, which was already verified", ("s", key_size));
      return true;
   }

   b1::chain_kv::bytes create_table_key(name scope, name table);

   bool get_table_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table);

   b1::chain_kv::bytes create_prefix_key(name scope, name table);

   void get_prefix_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table);

   template<typename CharKey>
   rocksdb::Slice prefix_slice(const CharKey& composite_key) {
      EOS_ASSERT(composite_key.size() >= prefix_size, bad_composite_key_exception,
                 "Cannot extract a prefix from a key that is not big enough to contain a prefix");
      return rocksdb::Slice(composite_key.data(), prefix_size);
   }

   template<typename CharKey>
   rocksdb::Slice prefix_type_slice(const CharKey& composite_key) {
      const auto prefix_type_size = prefix_size + sizeof(key_type);
      EOS_ASSERT(composite_key.size() >= prefix_type_size, bad_composite_key_exception,
                 "Cannot extract a prefix from a key that is not big enough to contain a prefix");
      return rocksdb::Slice(composite_key.data(), prefix_type_size);
   }

   template<typename CharKey>
   rocksdb::Slice prefix_primary_to_secondary_slice(const CharKey& composite_key, bool with_primary_key) {
      static constexpr auto prefix_type_size = prefix_size + sizeof(key_type);
      auto expected_size = prefix_type_size + sizeof(key_type) + (with_primary_key ? sizeof(uint64_t) : 0);
      EOS_ASSERT(composite_key.size() >= expected_size, bad_composite_key_exception,
                 "Cannot extract a primary-to-secondary key prefix from a key that is not big enough to contain a one");
      const char* offset = composite_key.data() + prefix_size;
      EOS_ASSERT(key_type{*offset++} == key_type::primary_to_sec, bad_composite_key_exception,
                 "Cannot extract a primary-to-secondary key prefix from a key that is not of that type");
      const key_type sec_type { *offset };
      EOS_ASSERT(sec_type >= key_type::sec_i64 && sec_type <= key_type::sec_long_double,
                 bad_composite_key_exception,
                 "Invariant failure, composite_key is of the correct type, but the format of the key after that is incorrect");
      return rocksdb::Slice(composite_key.data(), expected_size);
   }

   key_type extract_key_type(const b1::chain_kv::bytes& composite_key);

   key_type extract_primary_to_sec_key_type(const b1::chain_kv::bytes& composite_key);

   intermittent_decomposed_values get_prefix_thru_key_type(const b1::chain_kv::bytes& composite_key);


   template<typename CharKey1, typename CharKey2>
   bool get_primary_key(const CharKey1& full_key, const CharKey2& type_prefix, uint64_t& primary_key) {
      const auto type_prefix_size = type_prefix.size();
      const auto prefix_type_size = prefix_size + sizeof(key_type);
      EOS_ASSERT(type_prefix_size == prefix_type_size, bad_composite_key_exception,
                 "DB intrinsic key-value get_primary_key was passed a type_prefix with key size: ${s1} bytes "
                 "which is not equal to the expected size: ${s2}", ("s1", type_prefix_size)("s2", prefix_type_size));
      const auto key_size = sizeof(primary_key);
      EOS_ASSERT(full_key.size() == type_prefix_size + key_size, bad_composite_key_exception,
                 "DB intrinsic key-value get_primary_key was passed a full key size: ${s1} bytes that was not exactly "
                 "${s2} bytes (the size of a primary key) larger than the type_prefix size: ${s3}",
                 ("s1", full_key.size())("s2", key_size)("s3", type_prefix_size));
      const auto comp = memcmp(type_prefix.data(), full_key.data(), type_prefix_size);
      if (comp != 0) {
         return false;
      }
      const key_type actual_kt {*(type_prefix.data() + type_prefix_size - 1)};
      EOS_ASSERT(key_type::primary == actual_kt, bad_composite_key_exception,
                 "DB intrinsic key-value get_primary_key was passed a key that was the wrong type: ${type},"
                 " it should have been of type: ${type2}",
                 ("type", detail::to_string(actual_kt))("type2", detail::to_string(key_type::primary)));
      auto start_offset = full_key.data() + type_prefix_size;
      const b1::chain_kv::bytes composite_trailing_prim_key {start_offset, start_offset + key_size};
      auto composite_loc = composite_trailing_prim_key.cbegin();
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_trailing_prim_key.cend(), primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value get_primary_key invariant has changed, extract_key should only fail if the "
                 "remaining string size is less than the sizeof(uint64_t)");
      return true;
   }
}}}} // ns eosio::chain::backing_store::db_key_value_format
