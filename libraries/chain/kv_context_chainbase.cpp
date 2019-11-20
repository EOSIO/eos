#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/kv_context.hpp>

namespace eosio { namespace chain {

   struct kv_iterator_chainbase : kv_iterator {
      using index_type = std::decay_t<decltype(std::declval<chainbase::database>().get_index<kv_index, by_kv_key>())>;

      chainbase::database&             db;
      const index_type&                idx = db.get_index<kv_index, by_kv_key>();
      name                             database_id;
      name                             contract;
      std::vector<char>                prefix;
      std::optional<std::vector<char>> next_prefix;
      std::optional<shared_blob>       kv_key;

      kv_iterator_chainbase(chainbase::database& db, name database_id, name contract, std::vector<char> prefix)
          : db{ db }, database_id{ database_id }, contract{ contract }, prefix{ std::move(prefix) } {

         next_prefix.emplace(this->prefix);
         bool have_next = false;
         for (auto it = next_prefix->rbegin(); it != next_prefix->rend(); ++it) {
            if (++*it) {
               have_next = true;
               break;
            }
         }
         if (!have_next)
            next_prefix.reset();
      }

      ~kv_iterator_chainbase() override {}

      bool is_kv_chainbase_context_iterator() const override { return true; }

      template <typename It>
      kv_it_stat move_to(const It& it) {
         if (it != idx.end() && it->database_id == database_id && it->contract == contract &&
             it->kv_key.size() >= prefix.size() && !memcmp(it->kv_key.data(), prefix.data(), prefix.size())) {
            kv_key.emplace(it->kv_key);
            return kv_it_stat::iterator_ok;
         } else {
            kv_key.reset();
            return kv_it_stat::iterator_end;
         }
      }

      kv_it_stat kv_it_status() override {
         if (!kv_key)
            return kv_it_stat::iterator_end;
         auto* kv = db.find<kv_object, by_kv_key>(boost::make_tuple(database_id, contract, *kv_key));
         if (kv)
            return kv_it_stat::iterator_ok;
         else
            return kv_it_stat::iterator_erased;
      }

      int32_t kv_it_compare(const kv_iterator& rhs) override {
         EOS_ASSERT(rhs.is_kv_chainbase_context_iterator(), kv_bad_iter, "Incompatible key-value iterators");
         auto& r = static_cast<const kv_iterator_chainbase&>(rhs);
         EOS_ASSERT(database_id == r.database_id && contract == r.contract, kv_bad_iter,
                    "Incompatible key-value iterators");
         if (!r.kv_key) {
            if (!kv_key)
               return 0;
            else
               return -1;
         }
         if (!kv_key)
            return 1;
         return compare_blob(*kv_key, *r.kv_key);
      }

      int32_t kv_it_key_compare(const char* key, uint32_t size) override {
         if (!kv_key)
            return 1;
         return compare_blob(*kv_key, std::string_view{ key, size });
      }

      kv_it_stat kv_it_move_to_end() override {
         kv_key.reset();
         return kv_it_stat::iterator_end;
      }

      kv_it_stat kv_it_next() override {
         if (kv_key) {
            auto it = idx.lower_bound(boost::make_tuple(database_id, contract, *kv_key));
            if (it != idx.end()) {
               if (it->kv_key == *kv_key)
                  ++it;
               return move_to(it);
            }
            kv_key.reset();
            return kv_it_stat::iterator_end;
         }
         return move_to(idx.lower_bound(boost::make_tuple(database_id, contract, prefix)));
      }

      kv_it_stat kv_it_prev() override {
         std::decay_t<decltype(idx.end())> it;
         if (kv_key)
            it = idx.lower_bound(boost::make_tuple(database_id, contract, *kv_key));
         else
            it = next_prefix ? idx.lower_bound(boost::make_tuple(database_id, contract, *next_prefix)) : idx.end();
         if (it != idx.begin())
            return move_to(--it);
         kv_key.reset();
         return kv_it_stat::iterator_end;
      }

      kv_it_stat kv_it_lower_bound(const char* key, uint32_t size) override {
         return move_to(idx.lower_bound(boost::make_tuple(database_id, contract, std::string_view{ key, size })));
      }

      kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         if (kv_key) {
            actual_size = kv_key->size();
            EOS_ASSERT(offset <= actual_size, table_access_violation, "Offset is out of range");
            memcpy(dest, kv_key->data() + offset, std::min(size, actual_size - offset));
            return kv_it_status();
         } else {
            actual_size = 0;
            EOS_ASSERT(offset <= actual_size, table_access_violation, "Offset is out of range");
            return kv_it_stat::iterator_end;
         }
      }

      kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         if (!kv_key) {
            actual_size = 0;
            EOS_ASSERT(offset <= actual_size, table_access_violation, "Offset is out of range");
            return kv_it_stat::iterator_end;
         }
         auto* kv = db.find<kv_object, by_kv_key>(boost::make_tuple(database_id, contract, *kv_key));
         if (!kv) {
            actual_size = 0;
            EOS_ASSERT(offset <= actual_size, table_access_violation, "Offset is out of range");
            return kv_it_stat::iterator_erased;
         }
         actual_size = kv->kv_value.size();
         EOS_ASSERT(offset <= actual_size, table_access_violation, "Offset is out of range");
         memcpy(dest, kv->kv_value.data() + offset, std::min(size, actual_size - offset));
         return kv_it_stat::iterator_ok;
      }
   }; // kv_iterator_chainbase

   struct kv_context_chainbase : kv_context {
      chainbase::database&       db;
      name                       database_id;
      name                       receiver;
      std::optional<shared_blob> temp_data_buffer;

      kv_context_chainbase(chainbase::database& db, name database_id, name receiver)
          : db{ db }, database_id{ database_id }, receiver{ receiver } {}

      ~kv_context_chainbase() override {}

      void kv_erase(uint64_t contract, const char* key, uint32_t key_size) override {
         // KV-TODO: resources
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         temp_data_buffer.reset();
         auto* kv = db.find<kv_object, by_kv_key>(
               boost::make_tuple(database_id, name{ contract }, std::string_view{ key, key_size }));
         if (!kv)
            return;
         db.remove(*kv);
      }

      void kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                  uint32_t value_size) override {
         // KV-TODO: resources
         // KV-TODO: restrict key_size, value_size
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         temp_data_buffer.reset();
         auto* kv = db.find<kv_object, by_kv_key>(
               boost::make_tuple(database_id, name{ contract }, std::string_view{ key, key_size }));
         if (kv) {
            db.modify(*kv, [&](auto& obj) { obj.kv_value.assign(value, value_size); });
         } else {
            db.create<kv_object>([&](auto& obj) {
               obj.database_id = database_id;
               obj.contract    = name{ contract };
               obj.kv_key.assign(key, key_size);
               obj.kv_value.assign(value, value_size);
            });
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
         EOS_ASSERT(offset <= temp_size, table_access_violation, "Offset is out of range");
         memcpy(data + offset, temp, std::min(data_size, temp_size - offset));
         return temp_size;
      }

      std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) override {
         return std::make_unique<kv_iterator_chainbase>(db, database_id, name{ contract },
                                                        std::vector<char>{ prefix, prefix + size });
      }
   }; // kv_context_chainbase

   std::unique_ptr<kv_context> create_kv_chainbase_context(chainbase::database& db, name database_id, name receiver) {
      return std::make_unique<kv_context_chainbase>(db, database_id, receiver);
   }

}} // namespace eosio::chain
