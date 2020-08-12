#include <eosio/chain/kv_context.hpp>
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

         context.update_db_usage(payer, delta, storage_usage_trace(context.get_action_id(), event_id.c_str(), "kv", trace.op_to_string()));
      }
   }
   kv_resource_manager create_kv_resource_manager_ram(apply_context& context) {
      return {&context, config::billable_size_v<kv_object>, &kv_resource_manager_update_ram};
   }

}}
