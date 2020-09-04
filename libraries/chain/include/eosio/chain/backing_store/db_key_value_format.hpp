#pragma once

#include <eosio/chain/name.hpp>
#include <chain_kv/chain_kv.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <memory>
#include <stdint.h>

namespace eosio { namespace chain { namespace backing_store { namespace db_key_value_format {
   using key256_t = std::array<uint128_t, 2>;
   constexpr uint64_t prefix_size = sizeof(name) * 2; // 8 (scope) + 8 (table)
   b1::chain_kv::bytes create_primary_key(name scope, name table, uint64_t db_key);

   bool get_primary_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table, uint64_t& db_key);

   // ordering affects key ordering
   enum class key_type : char {
      primary = 0,
      sec_i64 = 1,
      sec_i128 = 2,
      sec_i256 = 3,
      sec_double = 4,
      sec_long_double = 5, // changes needed in determine_sec_type to accommodate new types
      table = std::numeric_limits<char>::max() // require to be highest value for a given scope/table
   };

   namespace detail {

      template<typename Key>
      struct determine_sec_type {
      };

      template<>
      struct determine_sec_type<uint64_t> {
         static constexpr key_type kt = key_type::sec_i64;
         static constexpr key_type next_kt = key_type::sec_i128;
      };

      template<>
      struct determine_sec_type<eosio::chain::uint128_t> {
         static constexpr key_type kt = key_type::sec_i128;
         static constexpr key_type next_kt = key_type::sec_i256;
      };

      template<>
      struct determine_sec_type<key256_t> {
         static constexpr key_type kt = key_type::sec_i256;
         static constexpr key_type next_kt = key_type::sec_double;
      };

      template<>
      struct determine_sec_type<float64_t> {
         static constexpr key_type kt = key_type::sec_double;
         static constexpr key_type next_kt = key_type::sec_long_double;
      };

      template<>
      struct determine_sec_type<float128_t> {
         static constexpr key_type kt = key_type::sec_long_double;
         static constexpr key_type next_kt = key_type::table;
      };

      std::string to_string(const key_type& kt);

      b1::chain_kv::bytes prepare_composite_key_prefix(name scope, name table, std::size_t type_size, std::size_t key_size);

      b1::chain_kv::bytes prepare_composite_key(name scope, name table, std::size_t key_size, key_type kt);

      using intermittent_decomposed_prefix_values = std::tuple<name, name, b1::chain_kv::bytes::const_iterator>;

      intermittent_decomposed_prefix_values extract_from_composite_key_prefix(b1::chain_kv::bytes::const_iterator begin, b1::chain_kv::bytes::const_iterator key_end);

      using intermittent_decomposed_values = std::tuple<name, name, b1::chain_kv::bytes::const_iterator, key_type>;

      intermittent_decomposed_values extract_from_composite_key(b1::chain_kv::bytes::const_iterator begin, b1::chain_kv::bytes::const_iterator key_end);

   }

   template<typename Key>
   b1::chain_kv::bytes create_secondary_key(name scope, name table, const Key& db_key, uint64_t primary_key) {
      const key_type kt = detail::determine_sec_type<Key>::kt;
      b1::chain_kv::bytes composite_key = detail::prepare_composite_key(scope, table, sizeof(Key), kt);
      b1::chain_kv::append_key(composite_key, db_key);
      b1::chain_kv::append_key(composite_key, primary_key);
      return composite_key;
   }

   template<typename Key>
   bool get_secondary_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table, Key& db_key, uint64_t& primary_key) {
      b1::chain_kv::bytes::const_iterator composite_loc;
      key_type kt = key_type::sec_i64;
      std::tie(scope, table, composite_loc, kt) = detail::extract_from_composite_key(composite_key.cbegin(), composite_key.cend());
      constexpr static auto expected_kt = detail::determine_sec_type<Key>::kt;
      if (kt != expected_kt) {
         return false;
      }
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), db_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have an underlying: " +
                 detail::to_string(kt));
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), primary_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have a trailing primary key");
      return true;
   }

   b1::chain_kv::bytes create_upper_bound_prim_key(name scope, name table);

   template<typename Key, key_type next_kt = detail::determine_sec_type<Key>::next_kt>
   b1::chain_kv::bytes create_upper_bound_sec_key(name scope, name table) {
      // do all the preparation, but don't allocate any space for the key at the end
      const std::size_t zero_size = 0;
      return detail::prepare_composite_key(scope, table, zero_size, next_kt);
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
}}}} // ns eosio::chain::backing_store::db_key_value_format
