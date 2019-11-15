#pragma once

#include <memory>
#include <stdint.h>

namespace chainbase {
class database;
}

namespace eosio { namespace chain {

   enum class kv_it_stat {
      iterator_ok     = 0,  // Iterator is positioned at a key-value pair
      iterator_erased = -1, // The key-value pair that the iterator used to be positioned at was erased
      iterator_oob    = -2, // Iterator is out-of-bounds
   };

   struct kv_iterator {
      virtual ~kv_iterator() {}

      virtual int32_t kv_it_status()                                                                 = 0;
      virtual int32_t kv_it_compare(const kv_iterator& rhs)                                          = 0;
      virtual int32_t kv_it_key_compare(const char* key, uint32_t size)                              = 0;
      virtual int32_t kv_it_move_to_oob()                                                            = 0;
      virtual int32_t kv_it_increment()                                                              = 0;
      virtual int32_t kv_it_decrement()                                                              = 0;
      virtual int32_t kv_it_lower_bound(const char* key, uint32_t size)                              = 0;
      virtual int32_t kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size)   = 0;
      virtual int32_t kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) = 0;
   };

   struct kv_context {
      virtual ~kv_context() {}

      virtual void     kv_erase(uint64_t contract, const char* key, uint32_t key_size)                     = 0;
      virtual void     kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                              uint32_t value_size)                                                         = 0;
      virtual bool     kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) = 0;
      virtual uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size)                        = 0;

      virtual std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) = 0;
   };

   std::unique_ptr<kv_context> create_kv_chainbase_context(chainbase::database& db, uint8_t database_id, name receiver);

}} // namespace eosio::chain
