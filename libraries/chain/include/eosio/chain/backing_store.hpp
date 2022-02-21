#pragma once

#include <fc/reflect/variant.hpp>
#include <stdexcept>

namespace eosio { namespace chain {

   enum class backing_store_type {
      CHAINBASE, // A name for regular users. Uses Chainbase.
      ROCKSDB
   };

}} // namespace eosio::chain

namespace fc {
template <>
inline void to_variant(const eosio::chain::backing_store_type& store, fc::variant& v) {
   v = (uint64_t)store;
}
template <>
inline void from_variant(const fc::variant& v, eosio::chain::backing_store_type& store) {
   switch (store = (eosio::chain::backing_store_type)v.as_uint64()) {
      case eosio::chain::backing_store_type::CHAINBASE:
      case eosio::chain::backing_store_type::ROCKSDB:
         return;
   }
   throw std::runtime_error("Invalid backing store name: " + v.as_string());
}
} // namespace fc
