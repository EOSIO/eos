#pragma once

#include <fc/reflect/variant.hpp>
#include <stdexcept>

namespace eosio { namespace chain {

   enum class backing_store_type {
      NATIVE, // A name for regular users. Uses Chainbase.
      ROCKSDB
   };

}} // namespace eosio::chain

namespace fc {
template <>
inline void to_variant(const eosio::chain::backing_store_type& store, fc::variant& v) {
   switch (store) {
      case eosio::chain::backing_store_type::NATIVE: v = "NATIVE"; break;
      case eosio::chain::backing_store_type::ROCKSDB: v = "ROCKSDB";
   }
}
template <>
inline void from_variant(const fc::variant& v, eosio::chain::backing_store_type& store) {
   const std::string& val = v.as_string();
   if (val == "NATIVE") {
      store = eosio::chain::backing_store_type::NATIVE;
   } else if (val == "ROCKSDB") {
      store = eosio::chain::backing_store_type::ROCKSDB;
   } else {
      throw std::runtime_error("Invalid backing store name: " + val);
   }
}
} // namespace fc
