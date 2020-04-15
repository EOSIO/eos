#include <eosio/chain/kv_context.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {

   namespace {
      void kv_resource_manager_update_ram(apply_context& context, int64_t delta) {
         context.add_ram_usage(account_name(context.get_receiver()), delta, generic_ram_trace(0));
      }
      void kv_resource_manager_update_disk(apply_context& context, int64_t delta) {
         context.add_disk_usage(account_name(context.get_receiver()), delta);
      }
   }
   kv_resource_manager create_kv_resource_manager_ram(apply_context& context) {
      return {&context, config::billable_size_v<kv_object>, &kv_resource_manager_update_ram};
   }
   kv_resource_manager create_kv_resource_manager_disk(apply_context& context) {
      return {&context, config::billable_size_v<kv_object>, &kv_resource_manager_update_disk};
   }

}}
