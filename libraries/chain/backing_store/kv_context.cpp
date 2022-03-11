#include <eosio/chain/backing_store/kv_context.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {

   kv_iterator::kv_iterator(chainbase::database& db, tracker_type& tracker, uint32_t& itr_count, name contract, std::vector<char> prefix)
      : db{ db }, tracker{ tracker }, itr_count(itr_count), contract{ contract }, prefix{ std::move(prefix) } {

      ++itr_count;
   }

   kv_iterator::~kv_iterator() { --itr_count; }

   template <typename It>
   kv_it_stat kv_iterator::move_to(const It& it, uint32_t* found_key_size, uint32_t* found_value_size) {
      if (it != idx.end() && it->contract == contract &&
            it->kv_key.size() >= prefix.size() && !memcmp(it->kv_key.data(), prefix.data(), prefix.size())) {
         current = &*it;
         *found_key_size = current->kv_key.size();
         *found_value_size = current->kv_value.size();
         return kv_it_stat::iterator_ok;
      } else {
         current = nullptr;
         *found_key_size = 0;
         *found_value_size = 0;
         return kv_it_stat::iterator_end;
      }
   }

   kv_it_stat kv_iterator::kv_it_status() {
      if (!current)
         return kv_it_stat::iterator_end;
      else if (!tracker.is_removed(*current))
         return kv_it_stat::iterator_ok;
      else
         return kv_it_stat::iterator_erased;
   }

   int32_t kv_iterator::kv_it_compare(const kv_iterator& rhs) {
      EOS_ASSERT(contract == rhs.contract, kv_bad_iter, "Incompatible key-value iterators");
      EOS_ASSERT(!current || !tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
      EOS_ASSERT(!rhs.current || !tracker.is_removed(*rhs.current), kv_bad_iter, "Iterator to erased element");
      if (!rhs.current) {
         if (!current)
            return 0;
         else {
            return -1;
         }
      }
      if (!current) {
         return 1;
      }
      
      return compare_blob(current->kv_key, rhs.current->kv_key);
   }

   int32_t kv_iterator::kv_it_key_compare(const char* key, uint32_t size) {
      if (!current)
         return 1;
      EOS_ASSERT(!tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
      return compare_blob(current->kv_key, std::string_view{ key, size });
   }

   kv_it_stat kv_iterator::kv_it_move_to_end() {
      current = nullptr;
      return kv_it_stat::iterator_end;
   }

   kv_it_stat kv_iterator::kv_it_next(uint32_t* found_key_size, uint32_t* found_value_size) {
      if (current) {
         EOS_ASSERT(!tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
         auto it = idx.iterator_to(*current);
         ++it;
         return move_to(it, found_key_size, found_value_size);
      }
      return move_to(idx.lower_bound(boost::make_tuple(contract, prefix)), found_key_size, found_value_size);
   }

   kv_it_stat kv_iterator::kv_it_prev(uint32_t* found_key_size, uint32_t* found_value_size) {
      std::decay_t<decltype(idx.end())> it;
      if (current) {
         EOS_ASSERT(!tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
         it = idx.iterator_to(*current);
      } else
         it = idx.upper_bound(boost::make_tuple(contract, blob_prefix{{prefix.data(), prefix.size()}}));
      if (it != idx.begin())
         return move_to(--it, found_key_size, found_value_size);
      current = nullptr;
      *found_key_size = 0;
      *found_value_size = 0;
      return kv_it_stat::iterator_end;
   }

   kv_it_stat kv_iterator::kv_it_lower_bound(const char* key, uint32_t size, uint32_t* found_key_size, uint32_t* found_value_size) {
      auto clamped_key = std::max(std::string_view{ key, size }, std::string_view{ prefix.data(), prefix.size() }, unsigned_blob_less{});
      return move_to(idx.lower_bound(boost::make_tuple(contract, clamped_key)), found_key_size, found_value_size);
   }

   kv_it_stat kv_iterator::kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) {
      if (!current) {
         actual_size = 0;
         return kv_it_stat::iterator_end;
      }
      EOS_ASSERT(!tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
      if (offset < current->kv_key.size())
         memcpy(dest, current->kv_key.data() + offset, std::min((size_t)size, current->kv_key.size() - offset));
      actual_size = current->kv_key.size();
      return kv_it_stat::iterator_ok;
   }

   kv_it_stat kv_iterator::kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) {
      if (!current) {
         actual_size = 0;
         return kv_it_stat::iterator_end;
      }
      EOS_ASSERT(!tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
      if (offset < current->kv_value.size())
         memcpy(dest, current->kv_value.data() + offset, std::min((size_t)size, current->kv_value.size() - offset));
      actual_size = current->kv_value.size();
      return kv_it_stat::iterator_ok;
   }

   std::optional<name> kv_iterator::kv_it_payer() {
      if (!current) return {};
      return current->payer;
   }

   kv_context::kv_context(chainbase::database& db, name receiver,
                        const kv_resource_manager& resource_manager, const kv_database_config& limits)
      : db{ db }, receiver{ receiver }, resource_manager{ resource_manager }, limits{limits} {}

   int64_t kv_context::kv_erase(uint64_t contract, const char* key, uint32_t key_size) {
      EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
      temp_data_buffer.reset();
      auto* kv = db.find<kv_object, by_kv_key>(boost::make_tuple(name{ contract }, std::string_view{ key, key_size }));
      if (!kv)
         return 0;
      const int64_t resource_delta = erase_table_usage(resource_manager, kv->payer, key, kv->kv_key.size(), kv->kv_value.size());

      if (auto dm_logger = resource_manager._context->control.get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "KV_OP REM ${action_id} ${db} ${payer} ${key} ${odata}",
            ("action_id", resource_manager._context->get_action_id())
            ("contract", name{ contract })
            ("payer", kv->payer)
            ("key", fc::to_hex(kv->kv_key.data(), kv->kv_key.size()))
            ("odata", fc::to_hex(kv->kv_value.data(), kv->kv_value.size()))
         );
      }

      tracker.remove(*kv);
      return resource_delta;
   }

   int64_t kv_context::kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                  uint32_t value_size, account_name payer) {
      EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
      EOS_ASSERT(key_size <= limits.max_key_size, kv_limit_exceeded, "Key too large");
      EOS_ASSERT(value_size <= limits.max_value_size, kv_limit_exceeded, "Value too large");

      temp_data_buffer.reset();
      auto* kv = db.find<kv_object, by_kv_key>(
            boost::make_tuple(name{ contract }, std::string_view{ key, key_size }));
      if (kv) {
         const auto resource_delta = update_table_usage(resource_manager, kv->payer, payer, key, key_size, kv->kv_value.size(), value_size);

         if (auto dm_logger = resource_manager._context->control.get_deep_mind_logger()) {
            fc_dlog(*dm_logger, "KV_OP UPD ${action_id} ${db} ${payer} ${key} ${odata}:${ndata}",
               ("action_id", resource_manager._context->get_action_id())
               ("contract", name{ contract })
               ("payer", payer)
               ("key", fc::to_hex(kv->kv_key.data(), kv->kv_key.size()))
               ("odata", fc::to_hex(kv->kv_value.data(), kv->kv_value.size()))
               ("ndata", fc::to_hex(value, value_size))
            );
         }

         db.modify(*kv, [&](auto& obj) {
            obj.kv_value.assign(value, value_size);
            obj.payer = payer;
         });
         return resource_delta;
      } else {
         const int64_t resource_delta = create_table_usage(resource_manager, payer, key, key_size, value_size);
         db.create<kv_object>([&](auto& obj) {
            obj.contract    = name{ contract };
            obj.kv_key.assign(key, key_size);
            obj.kv_value.assign(value, value_size);
            obj.payer       = payer;
         });

         if (auto dm_logger = resource_manager._context->control.get_deep_mind_logger()) {
            fc_dlog(*dm_logger, "KV_OP INS ${action_id} ${db} ${payer} ${key} ${ndata}",
               ("action_id", resource_manager._context->get_action_id())
               ("contract", name{ contract })
               ("payer", payer)
               ("key", fc::to_hex(key, key_size))
               ("ndata", fc::to_hex(value, value_size))
            );
         }

         return resource_delta;
      }
   }

   bool kv_context::kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) {
      auto* kv = db.find<kv_object, by_kv_key>(boost::make_tuple(name{ contract }, std::string_view{ key, key_size }));
      if (kv) {
         temp_data_buffer.emplace(kv->kv_value);
         value_size = temp_data_buffer->size();
         return true;
      } else {
         temp_data_buffer.reset();
         value_size = 0;
         return false;
      }
   }

   uint32_t kv_context::kv_get_data(uint32_t offset, char* data, uint32_t data_size) {
      const char* temp      = nullptr;
      uint32_t    temp_size = 0;
      if (temp_data_buffer) {
         temp      = temp_data_buffer->data();
         temp_size = temp_data_buffer->size();
      }
      if (offset < temp_size)
         memcpy(data, temp + offset, std::min(data_size, temp_size - offset));
      return temp_size;
   }

   std::unique_ptr<kv_iterator> kv_context::kv_it_create(uint64_t contract, const char* prefix, uint32_t size) {
      EOS_ASSERT(num_iterators < limits.max_iterators, kv_bad_iter, "Too many iterators");
      EOS_ASSERT(size <= limits.max_key_size, kv_bad_iter, "Prefix too large");
      return std::make_unique<kv_iterator>(db, tracker, num_iterators, name{ contract },
                                                      std::vector<char>{ prefix, prefix + size });
   }

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
}}
