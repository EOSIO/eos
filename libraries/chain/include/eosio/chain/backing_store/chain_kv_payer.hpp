#pragma once

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain { namespace backing_store {
   static constexpr auto payer_in_value_size = sizeof(account_name);

   inline static uint32_t actual_value_size(const uint32_t raw_value_size) {
      EOS_ASSERT(raw_value_size >= payer_in_value_size, kv_rocksdb_bad_value_size_exception , "The size of value returned from RocksDB is less than payer's size");
      return (raw_value_size - payer_in_value_size);
   }

   inline static account_name get_payer(const char* data) {
      account_name payer;
      memcpy(&payer, data, payer_in_value_size);
      return payer;
   }

   inline static const char* actual_value_start(const char* data) {
      return data + payer_in_value_size;
   }

}}} // ns eosio::chain::backing_store
