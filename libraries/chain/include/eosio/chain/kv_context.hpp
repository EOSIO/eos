#pragma once

#include <eosio/chain/name.hpp>
#include <eosio/chain/kv_config.hpp>
#include <eosio/chain/types.hpp>
#include <memory>
#include <stdint.h>

namespace chainbase {
class database;
}

namespace b1::chain_kv {
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
      virtual bool       is_kv_rocksdb_context_iterator() const                                       = 0;
      virtual kv_it_stat kv_it_status()                                                                 = 0;
      virtual int32_t    kv_it_compare(const kv_iterator& rhs)                                          = 0;
      virtual int32_t    kv_it_key_compare(const char* key, uint32_t size)                              = 0;
      virtual kv_it_stat kv_it_move_to_end()                                                            = 0;
      virtual kv_it_stat kv_it_next(uint32_t* found_key_size, uint32_t* found_value_size)               = 0;
      virtual kv_it_stat kv_it_prev(uint32_t* found_key_size, uint32_t* found_value_size)               = 0;
      virtual kv_it_stat kv_it_lower_bound(const char* key, uint32_t size,
                                          uint32_t* found_key_size, uint32_t* found_value_size)         = 0;
      virtual kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size)   = 0;
      virtual kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) = 0;
   };

   struct kv_resource_trace {
      enum class operation {
         create  = 0, // The operation represents the creation of a key-value pair
         update  = 1, // The operation represents the update of a key-value pair
         erase   = 2, // The operation represents the deletion of a key-value pair
      };

      kv_resource_trace(const char* key, uint32_t key_size, operation op): key(key, key_size), op(op) {}

      std::string_view key;
      operation op;

      const char* op_to_string() const {
         if (op == operation::create) return "create";
         if (op == operation::update) return "update";
         if (op == operation::erase) return "erase";

         return "unknown";
      }
   };

   struct kv_resource_manager {
      void     update_table_usage(account_name payer, int64_t delta, const kv_resource_trace& trace) { return _update_table_usage(*_context, delta, trace, payer); }
      apply_context* _context;
      uint64_t       billable_size;
      void (*_update_table_usage)(apply_context&, int64_t delta, const kv_resource_trace& trace, account_name payer);
   };

   kv_resource_manager create_kv_resource_manager(apply_context& context);

   struct kv_context {
      virtual ~kv_context() {}

      virtual int64_t  kv_erase(uint64_t contract, const char* key, uint32_t key_size)                     = 0;
      virtual int64_t  kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                              uint32_t value_size, account_name payer)                                     = 0;
      virtual bool     kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) = 0;
      virtual uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size)                        = 0;

      virtual std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) = 0;

     protected:
      // Updates resource usage for payer and returns resource delta
      int64_t create_table_usage(kv_resource_manager& resource_manager, const account_name& payer, const char* key, const uint32_t key_size, const uint32_t value_size);
      int64_t erase_table_usage(kv_resource_manager& resource_manager, const account_name& payer, const char* key, const uint32_t key_size, const uint32_t value_size);
      int64_t update_table_usage(kv_resource_manager&  resource_manager, const account_name& old_payer, const account_name& new_payer, const char* key, const uint32_t key_size, const uint32_t old_value_size, const uint32_t new_value_size);
   };

   std::unique_ptr<kv_context> create_kv_chainbase_context(chainbase::database& db, name receiver,
                                                           kv_resource_manager resource_manager, const kv_database_config& limits);

   std::unique_ptr<kv_context> create_kv_rocksdb_context(b1::chain_kv::database& kv_database, b1::chain_kv::undo_stack& kv_undo_stack, name receiver, kv_resource_manager resource_manager, const kv_database_config& limits);

}} // namespace eosio::chain
