#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/kv_context.hpp>

namespace eosio { namespace chain {

   struct kv_iterator_chainbase : kv_iterator {
      using index_type = std::decay_t<decltype(std::declval<chainbase::database>().get_index<kv_index, by_kv_key>())>;
      using tracker_type = std::decay_t<decltype(std::declval<chainbase::database>().get_mutable_index<kv_index>().track_removed())>;

      chainbase::database&       db;
      const index_type&          idx = db.get_index<kv_index, by_kv_key>();
      tracker_type&              tracker;
      uint32_t&                  itr_count;
      name                       database_id;
      name                       contract;
      std::vector<char>          prefix;
      const kv_object*           current = nullptr;

      kv_iterator_chainbase(chainbase::database& db, tracker_type& tracker, uint32_t& itr_count, name database_id, name contract, std::vector<char> prefix)
         : db{ db }, tracker{ tracker }, itr_count(itr_count), database_id{ database_id }, contract{ contract }, prefix{ std::move(prefix) } {

         ++itr_count;
      }

      ~kv_iterator_chainbase() override { --itr_count; }

      bool is_kv_chainbase_context_iterator() const override { return true; }

      template <typename It>
      kv_it_stat move_to(const It& it) {
         if (it != idx.end() && it->database_id == database_id && it->contract == contract &&
             it->kv_key.size() >= prefix.size() && !memcmp(it->kv_key.data(), prefix.data(), prefix.size())) {
            current = &*it;
            return kv_it_stat::iterator_ok;
         } else {
            current = nullptr;
            return kv_it_stat::iterator_end;
         }
      }

      kv_it_stat kv_it_status() override {
         if (!current)
            return kv_it_stat::iterator_end;
         else if (!tracker.is_removed(*current))
            return kv_it_stat::iterator_ok;
         else
            return kv_it_stat::iterator_erased;
      }

      int32_t kv_it_compare(const kv_iterator& rhs) override {
         EOS_ASSERT(rhs.is_kv_chainbase_context_iterator(), kv_bad_iter, "Incompatible key-value iterators");
         auto& r = static_cast<const kv_iterator_chainbase&>(rhs);
         EOS_ASSERT(database_id == r.database_id && contract == r.contract, kv_bad_iter,
                    "Incompatible key-value iterators");
         EOS_ASSERT(!current || !tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
         EOS_ASSERT(!r.current || !tracker.is_removed(*r.current), kv_bad_iter, "Iterator to erased element");
         if (!r.current) {
            if (!current)
               return 0;
            else {
               return -1;
            }
         }
         if (!current) {
            return 1;
         }
         return compare_blob(current->kv_key, r.current->kv_key);
      }

      int32_t kv_it_key_compare(const char* key, uint32_t size) override {
         if (!current)
            return 1;
         EOS_ASSERT(!tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
         return compare_blob(current->kv_key, std::string_view{ key, size });
      }

      kv_it_stat kv_it_move_to_end() override {
         current = nullptr;
         return kv_it_stat::iterator_end;
      }

      kv_it_stat kv_it_next() override {
         if (current) {
            EOS_ASSERT(!tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
            auto it = idx.iterator_to(*current);
            ++it;
            return move_to(it);
         }
         return move_to(idx.lower_bound(boost::make_tuple(database_id, contract, prefix)));
      }

      kv_it_stat kv_it_prev() override {
         std::decay_t<decltype(idx.end())> it;
         if (current) {
            EOS_ASSERT(!tracker.is_removed(*current), kv_bad_iter, "Iterator to erased element");
            it = idx.iterator_to(*current);
         } else
            it = idx.upper_bound(boost::make_tuple(database_id, contract, blob_prefix{{prefix.data(), prefix.size()}}));
         if (it != idx.begin())
            return move_to(--it);
         current = nullptr;
         return kv_it_stat::iterator_end;
      }

      kv_it_stat kv_it_lower_bound(const char* key, uint32_t size) override {
         auto clamped_key = std::max(std::string_view{ key, size }, std::string_view{ prefix.data(), prefix.size() }, unsigned_blob_less{});
         return move_to(idx.lower_bound(boost::make_tuple(database_id, contract, clamped_key)));
      }

      kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
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

      kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
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
   }; // kv_iterator_chainbase

   struct kv_context_chainbase : kv_context {
      using tracker_type = std::decay_t<decltype(std::declval<chainbase::database>().get_mutable_index<kv_index>().track_removed())>;
      chainbase::database&       db;
      tracker_type               tracker = db.get_mutable_index<kv_index>().track_removed();
      name                       database_id;
      name                       receiver;
      kv_resource_manager        resource_manager;
      const kv_database_config&  limits;
      uint32_t                   num_iterators = 0;
      std::optional<shared_blob> temp_data_buffer;

      kv_context_chainbase(chainbase::database& db, name database_id, name receiver,
                           kv_resource_manager resource_manager, const kv_database_config& limits)
         : db{ db }, database_id{ database_id }, receiver{ receiver }, resource_manager{ resource_manager }, limits{limits} {}

      ~kv_context_chainbase() override {}

      int64_t kv_erase(uint64_t contract, const char* key, uint32_t key_size) override {
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         temp_data_buffer.reset();
         auto* kv = db.find<kv_object, by_kv_key>(
               boost::make_tuple(database_id, name{ contract }, std::string_view{ key, key_size }));
         if (!kv)
            return 0;
         int64_t resource_delta = -(static_cast<int64_t>(resource_manager.billable_size) + kv->kv_key.size() + kv->kv_value.size());
         resource_manager.update_table_usage(resource_delta);
         tracker.remove(*kv);
         return resource_delta;
      }

      int64_t kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                     uint32_t value_size) override {
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         EOS_ASSERT(key_size <= limits.max_key_size, kv_limit_exceeded, "Key too large");
         EOS_ASSERT(value_size <= limits.max_value_size, kv_limit_exceeded, "Value too large");
         temp_data_buffer.reset();
         auto* kv = db.find<kv_object, by_kv_key>(
               boost::make_tuple(database_id, name{ contract }, std::string_view{ key, key_size }));
         if (kv) {
            // 64-bit arithmetic cannot overflow, because both the key and value are limited to 32-bits
            int64_t old_size = static_cast<int64_t>(kv->kv_key.size()) + kv->kv_value.size();
            int64_t new_size = static_cast<int64_t>(value_size) + key_size;
            int64_t resource_delta = new_size - old_size;
            resource_manager.update_table_usage(resource_delta);
            db.modify(*kv, [&](auto& obj) { obj.kv_value.assign(value, value_size); });
            return resource_delta;
         } else {
            int64_t new_size = static_cast<int64_t>(value_size) + key_size;
            int64_t resource_delta = new_size + resource_manager.billable_size;
            resource_manager.update_table_usage(resource_delta);
            db.create<kv_object>([&](auto& obj) {
               obj.database_id = database_id;
               obj.contract    = name{ contract };
               obj.kv_key.assign(key, key_size);
               obj.kv_value.assign(value, value_size);
            });
            return resource_delta;
         }
      }

      bool kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) override {
         auto* kv = db.find<kv_object, by_kv_key>(
               boost::make_tuple(database_id, name{ contract }, std::string_view{ key, key_size }));
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

      uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) override {
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

      std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) override {
         EOS_ASSERT(num_iterators < limits.max_iterators, kv_bad_iter, "Too many iterators");
         EOS_ASSERT(size <= limits.max_key_size, kv_bad_iter, "Prefix too large");
         return std::make_unique<kv_iterator_chainbase>(db, tracker, num_iterators, database_id, name{ contract },
                                                        std::vector<char>{ prefix, prefix + size });
      }
   }; // kv_context_chainbase

   std::unique_ptr<kv_context> create_kv_chainbase_context(chainbase::database& db, name database_id, name receiver,
                                                           kv_resource_manager resource_manager, const kv_database_config& limits) {
      return std::make_unique<kv_context_chainbase>(db, database_id, receiver, resource_manager, limits);
   }

}} // namespace eosio::chain
