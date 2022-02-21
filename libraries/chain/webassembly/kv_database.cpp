#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/apply_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int64_t  interface::kv_erase(uint64_t contract, span<const char> key) {
      return context.kv_erase(contract, key.data(), key.size());
   }

   int64_t  interface::kv_set(uint64_t contract, span<const char> key, span<const char> value, account_name payer) {
      return context.kv_set(contract, key.data(), key.size(), value.data(), value.size(), payer);
   }

   bool     interface::kv_get(uint64_t contract, span<const char> key, uint32_t* value_size) {
      return context.kv_get(contract, key.data(), key.size(), *value_size);
   }
   
   uint32_t interface::kv_get_data(uint32_t offset, span<char> data) {
      return context.kv_get_data(offset, data.data(), data.size());
   }

   uint32_t interface::kv_it_create(uint64_t contract, span<const char> prefix) {
      return context.kv_it_create(contract, prefix.data(), prefix.size());
   }

   void     interface::kv_it_destroy(uint32_t itr) {
      return context.kv_it_destroy(itr);
   }

   int32_t  interface::kv_it_status(uint32_t itr) {
      return context.kv_it_status(itr);
   }

   int32_t  interface::kv_it_compare(uint32_t itr_a, uint32_t itr_b) {
      return context.kv_it_compare(itr_a, itr_b);
   }

   int32_t  interface::kv_it_key_compare(uint32_t itr, span<const char> key) {
      return context.kv_it_key_compare(itr, key.data(), key.size());
   }

   int32_t  interface::kv_it_move_to_end(uint32_t itr) {
      return context.kv_it_move_to_end(itr);
   }

   int32_t  interface::kv_it_next(uint32_t itr, uint32_t* found_key_size, uint32_t* found_value_size) {
      return context.kv_it_next(itr, found_key_size, found_value_size);
   }

   int32_t  interface::kv_it_prev(uint32_t itr, uint32_t* found_key_size, uint32_t* found_value_size) {
      return context.kv_it_prev(itr, found_key_size, found_value_size);
   }

   int32_t  interface::kv_it_lower_bound(uint32_t itr, span<const char> key, uint32_t* found_key_size, uint32_t* found_value_size) {
      return context.kv_it_lower_bound(itr, key.data(), key.size(), found_key_size, found_value_size);
   }

   int32_t  interface::kv_it_key(uint32_t itr, uint32_t offset, span<char> dest, uint32_t* actual_size) {
      return context.kv_it_key(itr, offset, dest.data(), dest.size(), *actual_size);
   }

   int32_t  interface::kv_it_value(uint32_t itr, uint32_t offset, span<char> dest, uint32_t* actual_size) {
      return context.kv_it_value(itr, offset, dest.data(), dest.size(), *actual_size);
   }
}}} // ns eosio::chain::webassembly
