#pragma once

namespace eosio { namespace chain {

   enum class backing_store_type {
      NATIVE, // A name for regular users. Uses Chainbase.
      ROCKSDB
   };

}} // namespace eosio::chain
