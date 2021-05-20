#pragma once

#include <eosio/chain/types.hpp>
#include <b1/session/shared_bytes.hpp>

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

   // used to create a payload of payer and char* buffer, or to take a char* buffer and extract the payload from it
   // and a char* to the value portion of the payload.  NOTE: this is meant to be a short
   struct payer_payload {
      payer_payload(const char* data, std::size_t size)
      : value(actual_value_start(data)), value_size(actual_value_size(size)), payer(get_payer(data)) {}

      template<typename CharCont>
      payer_payload(const CharCont& data)
      : value(actual_value_start(data.data())), value_size(actual_value_size(data.size())),
              payer(get_payer(data.data())) {}

      payer_payload(name payer, const char* val, std::size_t val_size)
      : value(val), value_size(val_size), payer(payer) {}

      eosio::session::shared_bytes as_payload() const {
         char payer_buf[payer_in_value_size];
         memcpy(payer_buf, &payer, payer_in_value_size);
         return eosio::session::make_shared_bytes<std::string_view, 2>({std::string_view{payer_buf,
                                                                                         payer_in_value_size},
                                                                        std::string_view{value, value_size}});
      }

      // pointer to the actual value portion of the payload
      const char* value;
      // size of the actual value portion of the payload
      std::size_t value_size;
      account_name payer;
   };
}}} // ns eosio::chain::backing_store
