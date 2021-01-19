#pragma once

#include <b1/chain_kv/chain_kv.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <b1/session/shared_bytes.hpp>
#include <memory>
#include <stdint.h>

namespace eosio {
   namespace session {
      class shared_bytes;
   }

namespace chain { namespace backing_store { namespace db_key_value_format {
   using key256_t = std::array<eosio::chain::uint128_t, 2>;

   // NOTE: very limited use till redesign
   constexpr uint64_t db_type_and_code_size = sizeof(char) + sizeof(name); // 1 (db type) + 8 (contract)

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

      template<typename CharKey>
      struct associated;

      template<>
      struct associated<eosio::session::shared_bytes> {
         using stored_key = eosio::session::shared_bytes;
      };

      template<typename CharKey>
      struct associated {
         using stored_key = b1::chain_kv::bytes;
      };

      template<typename CharKey>
      constexpr std::size_t prefix_size() {
         constexpr uint64_t legacy_prefix_size = sizeof(name) * 2; // 8 (scope) + 8 (table)
         if constexpr (std::is_same_v<CharKey, eosio::session::shared_bytes>) {
            constexpr uint64_t db_type_and_code_size = sizeof(char) + sizeof(name); // 1 (db type) + 4 (contract)
            return db_type_and_code_size + legacy_prefix_size;
         }
         else if constexpr (std::is_same_v<CharKey, rocksdb::Slice>) {
            return legacy_prefix_size;
         }
         else {
            static_assert(std::is_same_v<CharKey, b1::chain_kv::bytes>);
            return legacy_prefix_size;
         }
      }

      template<typename CharKey1, typename CharKey2>
      constexpr void consistent_keys() {
         if constexpr (std::is_same_v<CharKey1, CharKey2>) {
            return;
         }
         else {
            static_assert(!std::is_same_v<CharKey1, eosio::session::shared_bytes>);
            static_assert(!std::is_same_v<CharKey2, eosio::session::shared_bytes>);
         }
      }

      constexpr std::size_t key_size(key_type kt);

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
         // NOTE: db_key_value_any_lookup end_of_prefix::at_prim_to_sec_type tied to this layout
         const key_type kt = determine_sec_type<Key>::kt;
         composite_key.push_back(static_cast<const char>(kt));
      }

      template<typename Key>
      void append_primary_and_secondary_keys(b1::chain_kv::bytes& composite_key, uint64_t primary_key, const Key& sec_key) {
         append_primary_and_secondary_key_types<Key>(composite_key);
         // NOTE: db_key_value_any_lookup end_of_prefix::at_prim_to_sec_primary_key tied to this layout
         b1::chain_kv::append_key(composite_key, primary_key);
         b1::chain_kv::append_key(composite_key, sec_key);
      }

      template<typename CharKey>
      bool validate_primary_to_sec_type_prefix(const CharKey& type_prefix, std::size_t start_prefix_size, key_type expected_sec_kt) {
         // possibly add the extra_prefix_size (to cover db_type and contract for a full key)
         const auto prefix_minimum_size = start_prefix_size + sizeof(key_type);
         const auto both_types_size = prefix_minimum_size + sizeof(key_type);
         const auto both_types_and_primary_size = both_types_size + sizeof(uint64_t);
         const bool has_sec_type = type_prefix.size() >= both_types_size;
         EOS_ASSERT(type_prefix.size() == prefix_minimum_size || type_prefix.size() == both_types_size ||
                    type_prefix.size() == both_types_and_primary_size, bad_composite_key_exception,
                    "DB intrinsic key-value validate_primary_to_sec_type_prefix was passed a prefix key size: ${s1} "
                    "bytes that was not either: ${s2}, ${s3}, or ${s4} bytes long",
                    ("s1", type_prefix.size())("s2", prefix_minimum_size)("s3", both_types_and_primary_size)
                    ("s4", both_types_and_primary_size));
         const char* prefix_ptr = type_prefix.data() + start_prefix_size;
         const key_type prefix_type_kt {*(prefix_ptr++)};
         EOS_ASSERT(key_type::primary_to_sec == prefix_type_kt, bad_composite_key_exception,
                    "DB intrinsic key-value validate_primary_to_sec_type_prefix was passed a prefix that was the wrong "
                    "type: ${type}, it should have been of type: ${type2}",
                    ("type", to_string(prefix_type_kt))("type2", to_string(key_type::primary_to_sec)));
         if (has_sec_type) {
            const key_type prefix_sec_kt {*(prefix_ptr)};
            EOS_ASSERT(expected_sec_kt == prefix_sec_kt, bad_composite_key_exception,
                       "DB intrinsic key-value validate_primary_to_sec_type_prefix was passed a prefix that was the "
                       "wrong secondary type: ${type}, it should have been of type: ${type2}",
                       ("type", to_string(prefix_sec_kt))("type2", to_string(expected_sec_kt)));
         }
         return has_sec_type;
      }

      template<typename Key, typename CharKey>
      void validate_primary_to_sec_type_size(const CharKey& full_key, std::size_t start_prefix_size) {
         const auto prefix_minimum_size = start_prefix_size + sizeof(key_type);
         const auto both_types_size = prefix_minimum_size + sizeof(key_type);
         const auto full_size = both_types_size + sizeof(uint64_t) + sizeof(Key);
         EOS_ASSERT(full_key.size() == full_size, bad_composite_key_exception,
                    "DB intrinsic key-value validate_primary_to_sec_type_size was passed a full key size: ${s1} bytes that was not at least "
                    "larger than the size that would include the secondary type: ${s2}",
                    ("s1", full_key.size())("s2", full_size));
      }

      template<typename Key, typename CharKey1, typename CharKey2>
      bool verify_primary_to_sec_type(const CharKey1& key, const CharKey2& type_prefix) {
         detail::consistent_keys<CharKey1, CharKey2>();

         const std::size_t start_prefix_size = prefix_size<CharKey2>();
         // >> validate that the type prefix is actually of primary_to_sec key type
         const key_type expected_sec_kt = determine_sec_type<Key>::kt;
         const auto check_sec_type = !validate_primary_to_sec_type_prefix(type_prefix, start_prefix_size, expected_sec_kt);

         // check if key is prefixed with type_prefix
         if (memcmp(type_prefix.data(), key.data(), type_prefix.size()) != 0) {
            return false;
         }
         if (check_sec_type) {
            const key_type prefix_sec_kt {*(key.data() + start_prefix_size + sizeof(key_type))};
            // key was not the right sec type
            if (expected_sec_kt != prefix_sec_kt) {
               return false;
            }
         }

         validate_primary_to_sec_type_size<Key>(key, start_prefix_size);
         return true;
      }

      template<typename Key, typename CharKey>
      std::pair<rocksdb::Slice, rocksdb::Slice> locate_trailing_primary_and_secondary_keys(const CharKey& key) {
         // this method just points to the data, it expects validation to be done prior to calling
         const auto both_type_size = prefix_size<CharKey>() + sizeof(key_type) + sizeof(key_type);
         const auto primary_offset = key.data() + both_type_size;
         return { rocksdb::Slice{primary_offset, sizeof(uint64_t)},
                  rocksdb::Slice{primary_offset + sizeof(uint64_t), sizeof(Key)}};
      }

      template<typename Key, typename CharKey>
      void extract_trailing_primary_and_secondary_keys(const CharKey& key, uint64_t& primary_key, Key& sec_key) {
         const auto key_offsets = locate_trailing_primary_and_secondary_keys<Key>(key);
         const auto keys_sizes = key_offsets.first.size() + key_offsets.second.size();
         const auto start_offset = key_offsets.first.data();
         const b1::chain_kv::bytes composite_trailing_prim_sec_key {start_offset, start_offset + keys_sizes};
         auto composite_loc = composite_trailing_prim_sec_key.cbegin();
         EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_trailing_prim_sec_key.cend(), primary_key), bad_composite_key_exception,
                    "DB intrinsic key-value store invariant has changed, extract_key should only fail if the remaining "
                    "string size is less than the sizeof(uint64_t)");
         EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_trailing_prim_sec_key.cend(), sec_key), bad_composite_key_exception,
                    "DB intrinsic key-value store invariant has changed, extract_key should only fail if the remaining "
                    "string size is less than: ${s} bytes, which was already verified", ("s", key_offsets.second.size()));
         EOS_ASSERT(composite_loc == composite_trailing_prim_sec_key.cend(), bad_composite_key_exception,
                    "DB intrinsic key-value store invariant has changed, calls to extract the pimary and secondary "
                    "keys should have resulted in the passed in buffer being consumed fully");
      }

      template<std::size_t N, typename CharKey>
      auto assemble(std::array<rocksdb::Slice, N>&& data) {
         if constexpr (std::is_same_v<CharKey, eosio::session::shared_bytes>) {
            return session::make_shared_bytes<rocksdb::Slice, N>(std::move(data));
         }
         else {
            b1::chain_kv::bytes key_data;
            const std::size_t init = 0;
            const std::size_t length = std::accumulate(data.begin(), data.end(), init,
                                                       [](std::size_t a, const rocksdb::Slice& b) { return a + b.size(); });
            key_data.reserve(length);
            for (const auto& d : data) {
               key_data.insert(key_data.end(), d.data(), d.data() + d.size());
            }
            return key_data;
         }
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

   b1::chain_kv::bytes create_prefix_type_key(name scope, name table, key_type kt);

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
      b1::chain_kv::bytes primary_to_secondary_key(secondary_key.begin(), secondary_key.begin() + detail::prefix_size<b1::chain_kv::bytes>());
      detail::append_primary_and_secondary_keys(primary_to_secondary_key, primary_key, sec_key);
      return { std::move(secondary_key), std::move(primary_to_secondary_key) };
   }

   template<typename Key, typename CharKey>
   CharKey create_secondary_key_from_primary_to_sec_key(const CharKey& primary_to_sec_key) {
      // checking size first, to validate that the key is the correct size before it is split
      const auto start_prefix_size = detail::prefix_size<CharKey>();
      detail::validate_primary_to_sec_type_size<Key>(primary_to_sec_key, start_prefix_size);
      const auto both_types_and_primary_size = detail::prefix_size<CharKey>() + sizeof(key_type) + sizeof(key_type) + sizeof(uint64_t);
      const rocksdb::Slice prefix {primary_to_sec_key.data(), both_types_and_primary_size};
      // now validating the front end of the key
      const key_type expected_sec_kt = detail::determine_sec_type<Key>::kt;
      // NOTE: using Slice, but it has the new db type and contract prefix
      const auto sec_kt_checked = detail::validate_primary_to_sec_type_prefix(prefix, start_prefix_size, expected_sec_kt);
      EOS_ASSERT(sec_kt_checked, bad_composite_key_exception,
                 "DB intrinsic invariant failure, key-value create_secondary_key_from_primary_to_sec_key was passed a "
                 "key that was verified as long enough but validate_primary_to_sec_type_prefix indicated that it wasn't "
                 "long enough to verify the secondary type");
      const auto key_offsets = detail::locate_trailing_primary_and_secondary_keys<Key>(primary_to_sec_key);
      using return_type = typename detail::associated<CharKey>::stored_key;
      const auto new_kt = static_cast<char>(expected_sec_kt);
      const char* new_kt_ptr = &new_kt;
      // already have everything in char format/order, so just need to assemble it
      return detail::assemble<4, return_type>({rocksdb::Slice{primary_to_sec_key.data(), start_prefix_size},
                                               rocksdb::Slice{new_kt_ptr, 1},
                                               key_offsets.second,
                                               key_offsets.first});
   }

   template<typename Key, typename CharKey1, typename CharKey2>
   bool get_trailing_sec_prim_keys(const CharKey1& full_key, const CharKey2& secondary_type_prefix, Key& sec_key, uint64_t& primary_key) {
      detail::consistent_keys<CharKey1, CharKey2>();

      // >> verify that secondary_type_prefix is really a valid secondary type prefix

      // possibly add the extra_prefix_size (to cover db_type and contract for a full key)
      const auto sec_type_prefix_size = secondary_type_prefix.size();
      const auto prefix_type_size = detail::prefix_size<CharKey1>() + sizeof(key_type);
      EOS_ASSERT(sec_type_prefix_size == prefix_type_size, bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_sec_prim_keys was passed a secondary_type_prefix with key size: ${s1} bytes "
                 "which is not equal to the expected size: ${s2}", ("s1", sec_type_prefix_size)("s2", prefix_type_size));
      const key_type expected_kt = detail::determine_sec_type<Key>::kt;
      const key_type actual_kt {*(secondary_type_prefix.data() + sec_type_prefix_size - 1)};
      EOS_ASSERT(expected_kt == actual_kt, bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_sec_prim_keys was passed a key that was the wrong type: ${type},"
                 " it should have been of type: ${type2}",
                 ("type", detail::to_string(actual_kt))("type2", detail::to_string(expected_kt)));
      const auto keys_size = sizeof(sec_key) + sizeof(primary_key);
      EOS_ASSERT(full_key.size() == sec_type_prefix_size + keys_size, bad_composite_key_exception,
                 "DB intrinsic key-value get_trailing_sec_prim_keys was passed a full key size: ${s1} bytes that was not exactly "
                 "${s2} bytes (the size of the secondary key and a primary key) larger than the secondary_type_prefix "
                 "size: ${s3}", ("s1", full_key.size())("s2", keys_size)("s3", sec_type_prefix_size));

      // >> identify if the passed in key matches the prefix

      const auto comp = memcmp(secondary_type_prefix.data(), full_key.data(), sec_type_prefix_size);
      if (comp != 0) {
         return false;
      }
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
      detail::consistent_keys<CharKey1, CharKey2>();

      // >> verify that secondary_type_prefix is really a valid secondary type prefix

      // possibly add the extra_prefix_size (to cover db_type and contract for a full key)
      if (!detail::verify_primary_to_sec_type<Key>(full_key, primary_to_sec_type_prefix)) {
         return false;
      }

      detail::extract_trailing_primary_and_secondary_keys(full_key, primary_key, sec_key);
      return true;
   }

   template<typename Key, typename CharKey1, typename CharKey2>
   bool get_trailing_secondary_key(const CharKey1& full_key, const CharKey2& sec_type_trailing_prefix, Key& sec_key) {
      detail::consistent_keys<CharKey1, CharKey2>();
      // possibly add the extra_prefix_size (to cover db_type and contract for a full key)
      const auto sec_type_trailing_prefix_size = sec_type_trailing_prefix.size();
      const auto prefix_with_primary_key_size = detail::prefix_size<CharKey1>() + sizeof(key_type) * 2 + sizeof(uint64_t);
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

   eosio::session::shared_bytes create_table_key(const eosio::session::shared_bytes& prefix_key);

   bool get_table_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table);

   b1::chain_kv::bytes create_prefix_key(name scope, name table);

   void get_prefix_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table);

   template<typename CharKey>
   rocksdb::Slice prefix_slice(const CharKey& composite_key) {
      detail::consistent_keys<rocksdb::Slice, CharKey>();
      const auto prefix_size = detail::prefix_size<rocksdb::Slice>();
      EOS_ASSERT(composite_key.size() >= prefix_size, bad_composite_key_exception,
                 "Cannot extract a prefix from a key that is not big enough to contain a prefix");
      return rocksdb::Slice(composite_key.data(), prefix_size);
   }

   template<typename CharKey>
   rocksdb::Slice prefix_type_slice(const CharKey& composite_key) {
      detail::consistent_keys<rocksdb::Slice, CharKey>();
      const auto prefix_type_size = detail::prefix_size<rocksdb::Slice>() + sizeof(key_type);
      EOS_ASSERT(composite_key.size() >= prefix_type_size, bad_composite_key_exception,
                 "Cannot extract a prefix from a key that is not big enough to contain a prefix");
      return rocksdb::Slice(composite_key.data(), prefix_type_size);
   }

   template<typename CharKey>
   rocksdb::Slice prefix_primary_to_secondary_slice(const CharKey& composite_key, bool with_primary_key) {
      detail::consistent_keys<rocksdb::Slice, CharKey>();
      // possibly add the extra_prefix_size (to cover db_type and contract for a full key)
      static constexpr auto prefix_size = detail::prefix_size<CharKey>();
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

   b1::chain_kv::bytes extract_legacy_key(const eosio::session::shared_bytes& complete_key);

   rocksdb::Slice extract_legacy_slice(const eosio::session::shared_bytes& complete_key);

   eosio::session::shared_bytes create_full_key(const b1::chain_kv::bytes& ck, name code);
   enum class end_of_prefix { pre_type, at_type, at_prim_to_sec_type, at_prim_to_sec_primary_key };
   eosio::session::shared_bytes create_full_key_prefix(const eosio::session::shared_bytes& ck, end_of_prefix prefix_end);

   struct full_key_data {
      char                               db_type;
      std::optional<name>                contract;
      std::optional<b1::chain_kv::bytes> legacy_key;
      std::optional<name>                scope;
      std::optional<name>                table;
      std::optional<key_type>            kt;
      rocksdb::Slice                     type_prefix;
   };
   full_key_data parse_full_key(const eosio::session::shared_bytes& full_key);

   template<typename CharKey1, typename CharKey2>
   bool get_primary_key(const CharKey1& full_key, const CharKey2& type_prefix, uint64_t& primary_key) {
      detail::consistent_keys<CharKey1, CharKey2>();
      // possibly add the extra_prefix_size (to cover db_type and contract for a full key)
      const auto type_prefix_size = type_prefix.size();
      const auto prefix_type_size = detail::prefix_size<CharKey1>() + sizeof(key_type);
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

   eosio::session::shared_bytes create_full_primary_key(name code, name scope, name table, uint64_t primary_key);
   eosio::session::shared_bytes create_full_prefix_key(name code, name scope, name table, std::optional<key_type> kt = std::optional<key_type>{});
   eosio::session::shared_bytes create_full_primary_key(const eosio::session::shared_bytes& prefix, uint64_t primary_key);

   template<typename Key>
   eosio::session::shared_bytes create_full_secondary_key(name code, name scope, name table, const Key& sec_key, uint64_t primary_key) {
      bytes composite_key = create_secondary_key(scope, table, sec_key, primary_key);
      return create_full_key(composite_key, code);
   }

   template<typename Key>
   eosio::session::shared_bytes create_full_prefix_secondary_key(name code, name scope, name table, const Key& sec_key) {
      bytes composite_key = create_prefix_secondary_key(scope, table, sec_key);
      return create_full_key(composite_key, code);
   }

}}}} // ns eosio::chain::backing_store::db_key_value_format
