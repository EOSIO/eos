#include <eosio/chain/backing_store/kv_context.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {

   namespace {
      void kv_resource_manager_update_ram(apply_context& context, int64_t delta, const kv_resource_trace& trace, account_name payer) {
         std::string event_id;
         if (context.control.get_deep_mind_logger() != nullptr) {
            event_id = STORAGE_EVENT_ID("${id}", ("id", fc::to_hex(trace.key.data(), trace.key.size())));
         }

         context.update_db_usage(payer, delta, storage_usage_trace(context.get_action_id(), std::move(event_id), "kv", trace.op_to_string()));
      }
   }
   kv_resource_manager create_kv_resource_manager(apply_context& context) {
      return {&context, config::billable_size_v<kv_object>, &kv_resource_manager_update_ram};
   }

   /*
   int64_t kv_context::create_table_usage(kv_resource_manager& resource_manager, const account_name& payer, const char* key, const uint32_t key_size, const uint32_t value_size) {
      const int64_t resource_delta = (static_cast<int64_t>(resource_manager.billable_size) + key_size + value_size);
      resource_manager.update_table_usage(payer, resource_delta, kv_resource_trace(key, key_size, kv_resource_trace::operation::create));
      return resource_delta;
   }

   int64_t kv_context::erase_table_usage(kv_resource_manager& resource_manager, const account_name& payer, const char* key, const uint32_t key_size, const uint32_t value_size) {
      const int64_t resource_delta = -(static_cast<int64_t>(resource_manager.billable_size) + key_size + value_size);
      resource_manager.update_table_usage(payer, resource_delta, kv_resource_trace(key, key_size, kv_resource_trace::operation::erase));
      return resource_delta;
   }

   int64_t kv_context::update_table_usage(kv_resource_manager& resource_manager, const account_name& old_payer, const account_name& new_payer, const char* key, const uint32_t key_size, const uint32_t old_value_size, const uint32_t new_value_size) {
      // 64-bit arithmetic cannot overflow, because both the key and value are limited to 32-bits
      const int64_t old_size = key_size + old_value_size;
      const int64_t new_size = key_size + new_value_size;
      const int64_t resource_delta = new_size - old_size;

      if (old_payer != new_payer) {
         // refund the existing payer
         resource_manager.update_table_usage(old_payer, -(old_size + resource_manager.billable_size), kv_resource_trace(key, key_size, kv_resource_trace::operation::update));
         // charge the new payer for full amount
         resource_manager.update_table_usage(new_payer, new_size + resource_manager.billable_size, kv_resource_trace(key, key_size, kv_resource_trace::operation::update));
      } else if (old_size != new_size) {
         // adjust delta for the existing payer
         resource_manager.update_table_usage(new_payer, resource_delta, kv_resource_trace(key, key_size, kv_resource_trace::operation::update));
      } // No need for a final "else" as usage does not change

      return resource_delta;
   }
   */
}}
