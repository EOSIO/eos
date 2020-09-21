#pragma once

#include <chain_kv/chain_kv.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/types.hpp>
#include <memory>
#include <stdint.h>

#include <session/shared_bytes.hpp>

namespace eosio { namespace chain { namespace backing_store { namespace db_key_value_format {
   using key256_t = std::array<uint128_t, 2>;

   eosio::session::shared_bytes create_primary_key(name scope, name table, uint64_t db_key);

   void get_primary_key(const eosio::session::shared_bytes& composite_key, name& scope, name& table, uint64_t& db_key);

   enum class key_type : char {
      primary         = 0,
      sec_i64         = 1,
      sec_i128        = 2,
      sec_i256        = 3,
      sec_double      = 4,
      sec_long_double = 5
   };

   namespace detail {

      template <typename Key>
      struct determine_sec_type {};

      template <>
      struct determine_sec_type<uint64_t> {
         static constexpr key_type kt = key_type::sec_i64;
      };

      template <>
      struct determine_sec_type<eosio::chain::uint128_t> {
         static constexpr key_type kt = key_type::sec_i128;
      };

      template <>
      struct determine_sec_type<key256_t> {
         static constexpr key_type kt = key_type::sec_i256;
      };

      template <>
      struct determine_sec_type<float64_t> {
         static constexpr key_type kt = key_type::sec_double;
      };

      template <>
      struct determine_sec_type<float128_t> {
         static constexpr key_type kt = key_type::sec_long_double;
      };

      std::string to_string(const key_type& kt);

      template <typename Key, key_type kt = determine_sec_type<Key>::kt>
      std::vector<char> prepare_composite_key(name scope, name table, std::size_t key_size) {
         constexpr static auto scope_size = sizeof(scope);
         constexpr static auto table_size = sizeof(table);
         constexpr static auto type_size  = sizeof(key_type);
         static_assert(type_size == 1, "");

         auto buffer = std::vector<char>{};
         buffer.reserve(scope_size + table_size + type_size + key_size);
         b1::chain_kv::append_key(buffer, scope.to_uint64_t());
         b1::chain_kv::append_key(buffer, table.to_uint64_t());
         buffer.push_back(static_cast<char>(kt));
         return buffer;
      }

      using intermittent_decomposed_values = std::tuple<name, name, eosio::session::shared_bytes::iterator, key_type>;

      intermittent_decomposed_values extract_from_composite_key(eosio::session::shared_bytes::iterator begin,
                                                                eosio::session::shared_bytes::iterator key_end);

   } // namespace detail

   template <typename Key>
   eosio::session::shared_bytes create_secondary_key(name scope, name table, const Key& db_key, uint64_t primary_key) {
      auto buffer = std::vector<char>{};
      b1::chain_kv::append_key(buffer, db_key);
      b1::chain_kv::append_key(buffer, primary_key);
      return eosio::session::make_shared_bytes(buffer.data(), buffer.size());
   }

   template <typename Key>
   bool get_secondary_key(const eosio::session::shared_bytes& composite_key, name& scope, name& table, Key& db_key,
                          uint64_t& primary_key) {
      eosio::session::shared_bytes::iterator composite_loc;
      key_type                               kt = key_type::sec_i64;
      std::tie(scope, table, composite_loc, kt) =
            detail::extract_from_composite_key(composite_key.begin(), composite_key.end());
      constexpr static auto expected_kt = detail::determine_sec_type<Key>::kt;
      if (kt != expected_kt) {
         return false;
      }
      EOS_ASSERT(b1::chain_kv::extract_key(composite_loc, composite_key.end(), db_key), bad_composite_key_exception,
                 "DB intrinsic key-value store composite key is malformed, it is supposed to have an underlying: " +
                       detail::to_string(kt));
      EOS_ASSERT(
            b1::chain_kv::extract_key(composite_loc, composite_key.end(), primary_key), bad_composite_key_exception,
            "DB intrinsic key-value store composite key is malformed, it is supposed to have a trailing primary key");
      return true;
   }

}}}} // namespace eosio::chain::backing_store::db_key_value_format
