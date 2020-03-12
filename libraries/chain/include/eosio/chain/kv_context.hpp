#pragma once

#include <eosio/chain/name.hpp>
#include <eosio/chain/kv_config.hpp>
#include <memory>
#include <stdint.h>

namespace chainbase {
class database;
}

namespace chain_kv {
struct database;
class undo_stack;
}

namespace eosio { namespace chain {

   class apply_context;

   inline constexpr name kvram_id  = N(eosio.kvram);
   inline constexpr name kvdisk_id = N(eosio.kvdisk);

   enum class kv_it_stat {
      iterator_ok     = 0,  // Iterator is positioned at a key-value pair
      iterator_erased = -1, // The key-value pair that the iterator used to be positioned at was erased
      iterator_end    = -2, // Iterator is out-of-bounds
   };

   struct kv_iterator {
      virtual ~kv_iterator() {}

      virtual bool       is_kv_chainbase_context_iterator() const                                       = 0;
      virtual bool       is_kv_rocksdb_context_iterator() const                                         = 0;
      virtual kv_it_stat kv_it_status()                                                                 = 0;
      virtual int32_t    kv_it_compare(const kv_iterator& rhs)                                          = 0;
      virtual int32_t    kv_it_key_compare(const char* key, uint32_t size)                              = 0;
      virtual kv_it_stat kv_it_move_to_end()                                                            = 0;
      virtual kv_it_stat kv_it_next()                                                                   = 0;
      virtual kv_it_stat kv_it_prev()                                                                   = 0;
      virtual kv_it_stat kv_it_lower_bound(const char* key, uint32_t size)                              = 0;
      virtual kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size)   = 0;
      virtual kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) = 0;
   };

   struct kv_resource_manager {
      void     update_table_usage(int64_t delta) { return _update_table_usage(*_context, delta); }
      apply_context* _context;
      uint64_t       billable_size;
      void (*_update_table_usage)(apply_context&, int64_t delta);
   };

   kv_resource_manager create_kv_resource_manager_ram(apply_context& context);
   kv_resource_manager create_kv_resource_manager_disk(apply_context& context);

   struct kv_context {
      virtual ~kv_context() {}

      virtual void     flush()                                                                             = 0;

      virtual int64_t  kv_erase(uint64_t contract, const char* key, uint32_t key_size)                     = 0;
      virtual int64_t  kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                              uint32_t value_size)                                                         = 0;
      virtual bool     kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) = 0;
      virtual uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size)                        = 0;

      virtual std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) = 0;
   };

   std::unique_ptr<kv_context> create_kv_chainbase_context(chainbase::database& db, name database_id, name receiver,
                                                           kv_resource_manager resource_manager, const kv_database_config& limits);

   std::unique_ptr<kv_context> create_kv_rocksdb_context(chain_kv::database&   kv_database,
                                                         chain_kv::undo_stack& kv_undo_stack, name database_id,
                                                         name receiver, kv_resource_manager resource_manager,
                                                         const kv_database_config& limits);

}} // namespace eosio::chain
