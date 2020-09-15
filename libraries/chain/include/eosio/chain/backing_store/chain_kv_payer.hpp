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

   struct payer_payload {
      payer_payload(const char* data, std::size_t size)
      : payer(get_payer(data)), value(actual_value_start(data)), value_size(actual_value_size(size)) {}

      template<typename CharCont>
      payer_payload(const CharCont& data)
            : payer(get_payer(data.data())), value(actual_value_start(data.data())), value_size(actual_value_size(data.size())) {}

      payer_payload(name payer, const char* val, std::size_t val_size) : payer(payer), value(val), value_size(val_size) {}

      bytes as_payload() const {
         bytes total_payload;
         const uint32_t total_payload_size = payer_in_value_size + value_size;
         total_payload.reserve(total_payload_size);

         char payer_buf[payer_in_value_size];
         memcpy(payer_buf, &payer, payer_in_value_size);
         total_payload.insert(total_payload.end(), payer_buf, payer_buf + payer_in_value_size);

         total_payload.insert(total_payload.end(), value, value + value_size);
         return total_payload;
      }

      const account_name payer;
      // pointer to the actual value portion of the payload
      const char* const value;
      // size of the actual value portion of the payload
      const std::size_t value_size;
   };
}}} // ns eosio::chain::backing_store
