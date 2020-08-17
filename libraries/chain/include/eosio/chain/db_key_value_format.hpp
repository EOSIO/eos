#pragma once

#include <eosio/chain/name.hpp>
#include <chain_kv/chain_kv.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <memory>
#include <stdint.h>

namespace eosio { namespace chain {
   class db_key_value_format {
   public:
      using key256_t = std::array<uint128_t, 2>;
      static b1::chain_kv::bytes create_primary_key(name scope, name table, uint64_t db_key);

      static void get_primary_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table, uint64_t& db_key);

      template<typename Key>
      static b1::chain_kv::bytes create_secondary_key(name scope, name table, const Key& db_key, uint64_t primary_key) {
         b1::chain_kv::bytes composite_key;
         prepare_composite_key<Key>(composite_key, scope, table, sizeof(Key));
         b1::chain_kv::append_key(composite_key, db_key);
         b1::chain_kv::append_key(composite_key, primary_key);
         return composite_key;
      }

      template<typename Key>
      static bool get_secondary_key(const b1::chain_kv::bytes& composite_key, name& scope, name& table, Key& db_key, uint64_t& primary_key) {
         b1::chain_kv::bytes::const_iterator composite_loc = composite_key.cbegin();
         auto kt = extract_from_composite_key(composite_loc, composite_key.cend(), scope, table);
         const auto expected_kt = determine_sec_type<Key>::kt;
         if (kt != expected_kt) {
            return false;
         }
         EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), db_key), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed, it is supposed to have an underlying: " +
                    to_string(kt));
         EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.cend(), primary_key), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed, it is supposed to have a trailing primary key");
         return true;
      }

      enum class key_type : char {
         primary = 0,
         sec_i64 = 1,
         sec_i128 = 2,
         sec_i256 = 3,
         sec_double = 4,
         sec_long_double = 5
      };

   private:

      template<typename Key>
      struct determine_sec_type {
      };

      template<>
      struct determine_sec_type<uint64_t> { static constexpr key_type kt = key_type::sec_i64; };

      template<>
      struct determine_sec_type<eosio::chain::uint128_t> { static constexpr key_type kt = key_type::sec_i128; };

      template<>
      struct determine_sec_type<key256_t> { static constexpr key_type kt = key_type::sec_i256; };

      template<>
      struct determine_sec_type<float64_t> {static constexpr key_type kt = key_type::sec_double; };

      template<>
      struct determine_sec_type<float128_t> { static constexpr key_type kt = key_type::sec_long_double; };

      static std::string to_string(const key_type& kt);

      template<typename Key, key_type kt = determine_sec_type<Key>::kt>
      static void prepare_composite_key(b1::chain_kv::bytes& key_storage, name scope, name table, std::size_t key_size) {
         const auto scope_size = sizeof(scope);
         const auto table_size = sizeof(table);
         const auto type_size = sizeof(key_type);
         key_storage.reserve(scope_size + table_size + type_size + key_size);
         b1::chain_kv::append_key(key_storage, scope.to_uint64_t());
         b1::chain_kv::append_key(key_storage, table.to_uint64_t());
         key_storage.push_back(static_cast<char>(kt));
      }

      static key_type extract_from_composite_key(b1::chain_kv::bytes::const_iterator& key_loc, b1::chain_kv::bytes::const_iterator key_end, name& scope, name& table);

   };

}} // ns eosio::chain
