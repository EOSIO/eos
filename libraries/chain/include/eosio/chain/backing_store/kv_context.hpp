#pragma once

#include <eosio/chain/name.hpp>
#include <eosio/chain/kv_config.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <memory>
#include <stdint.h>

namespace chainbase {
class database;
}

namespace eosio { namespace chain {

   class apply_context;

   enum class kv_it_stat {
      iterator_ok     = 0,  // Iterator is positioned at a key-value pair
      iterator_erased = -1, // The key-value pair that the iterator used to be positioned at was erased
      iterator_end    = -2, // Iterator is out-of-bounds
   };

   struct kv_iterator {
      using index_type = std::decay_t<decltype(std::declval<chainbase::database>().get_index<kv_index, by_kv_key>())>;
      using tracker_type = std::decay_t<decltype(std::declval<chainbase::database>().get_mutable_index<kv_index>().track_removed())>;

      chainbase::database&       db;
      const index_type&          idx = db.get_index<kv_index, by_kv_key>();
      tracker_type&              tracker;
      uint32_t&                  itr_count;
      name                       contract;
      std::vector<char>          prefix;
      const kv_object*           current = nullptr;

      kv_iterator(chainbase::database& db, tracker_type& tracker, uint32_t& itr_count, name contract, std::vector<char> prefix);

      ~kv_iterator();

      template <typename It>
      kv_it_stat move_to(const It& it, uint32_t* found_key_size, uint32_t* found_value_size);
      kv_it_stat kv_it_status();
      int32_t    kv_it_compare(const kv_iterator& rhs);
      int32_t    kv_it_key_compare(const char* key, uint32_t size);
      kv_it_stat kv_it_move_to_end();
      kv_it_stat kv_it_next(uint32_t* found_key_size, uint32_t* found_value_size);
      kv_it_stat kv_it_prev(uint32_t* found_key_size, uint32_t* found_value_size);
      kv_it_stat kv_it_lower_bound(const char* key, uint32_t size, uint32_t* found_key_size, uint32_t* found_value_size);
      kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
      kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);

      std::optional<name> kv_it_payer();
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
      using tracker_type = std::decay_t<decltype(std::declval<chainbase::database>().get_mutable_index<kv_index>().track_removed())>;
      chainbase::database&         db;
      tracker_type                 tracker = db.get_mutable_index<kv_index>().track_removed();
      name                         receiver;
      kv_resource_manager          resource_manager;
      const kv_database_config&    limits;
      uint32_t                     num_iterators = 0;
      std::optional<shared_blob>   temp_data_buffer;

      kv_context(chainbase::database& db, name receiver,
                           const kv_resource_manager& resource_manager, const kv_database_config& limits);
      int64_t kv_erase(uint64_t contract, const char* key, uint32_t key_size);
      int64_t kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                     uint32_t value_size, account_name payer);
      bool kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size);
      uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size);
      std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size);

     protected:
      // Updates resource usage for payer and returns resource delta
      int64_t create_table_usage(kv_resource_manager& resource_manager, const account_name& payer, const char* key, const uint32_t key_size, const uint32_t value_size);
      int64_t erase_table_usage(kv_resource_manager& resource_manager, const account_name& payer, const char* key, const uint32_t key_size, const uint32_t value_size);
      int64_t update_table_usage(kv_resource_manager& resource_manager, const account_name& old_payer, const account_name& new_payer, const char* key, const uint32_t key_size, const uint32_t old_value_size, const uint32_t new_value_size);
   };
}} // namespace eosio::chain
