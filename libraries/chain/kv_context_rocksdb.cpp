#include <eosio/chain/combined_database.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/kv_context.hpp>

namespace eosio { namespace chain {

   struct kv_iterator_rocksdb : kv_iterator {
      uint32_t&                num_iterators;
      chain_kv::view&          view;
      uint64_t                 contract;
      chain_kv::view::iterator kv_it;

      kv_iterator_rocksdb(uint32_t& num_iterators, chain_kv::view& view, uint64_t contract, const char* prefix,
                          uint32_t size)
          : num_iterators(num_iterators), view{ view }, contract{ contract }, //
            kv_it{ view, contract, { prefix, size } } {
         ++num_iterators;
      }

      ~kv_iterator_rocksdb() override { --num_iterators; }

      bool is_kv_chainbase_context_iterator() const override { return false; }
      bool is_kv_rocksdb_context_iterator() const override { return true; }

      kv_it_stat kv_it_status() override {
         if (kv_it.is_end())
            return kv_it_stat::iterator_end;
         else if (kv_it.is_erased())
            return kv_it_stat::iterator_erased;
         else
            return kv_it_stat::iterator_ok;
      }

      int32_t kv_it_compare(const kv_iterator& rhs) override {
         EOS_ASSERT(rhs.is_kv_rocksdb_context_iterator(), kv_bad_iter, "Incompatible key-value iterators");
         auto& r = static_cast<const kv_iterator_rocksdb&>(rhs);
         EOS_ASSERT(&view == &r.view && contract == r.contract, kv_bad_iter, "Incompatible key-value iterators");
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");
         EOS_ASSERT(!r.kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               return compare(kv_it, r.kv_it);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      int32_t kv_it_key_compare(const char* key, uint32_t size) override {
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               return chain_kv::compare_key(kv_it.get_kv(), chain_kv::key_value{ { key, size }, {} });
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      kv_it_stat kv_it_move_to_end() override {
         try {
            try {
               kv_it.move_to_end();
               return kv_it_stat::iterator_end;
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      kv_it_stat kv_it_next() override {
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               ++kv_it;
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_prev() override {
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               --kv_it;
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_lower_bound(const char* key, uint32_t size) override {
         try {
            try {
               kv_it.lower_bound(key, size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");

         std::optional<chain_kv::key_value> kv;
         try {
            try {
               kv = kv_it.get_kv();
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         if (kv) {
            if (offset < kv->key.size())
               memcpy(dest, kv->key.data() + offset, std::min((size_t)size, kv->key.size() - offset));
            actual_size = kv->key.size();
            return kv_it_stat::iterator_ok;
         } else {
            actual_size = 0;
            return kv_it_stat::iterator_end;
         }
      }

      kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");

         std::optional<chain_kv::key_value> kv;
         try {
            try {
               kv = kv_it.get_kv();
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         if (kv) {
            if (offset < kv->value.size())
               memcpy(dest, kv->value.data() + offset, std::min((size_t)size, kv->value.size() - offset));
            actual_size = kv->value.size();
            return kv_it_stat::iterator_ok;
         } else {
            actual_size = 0;
            return kv_it_stat::iterator_end;
         }
      }
   }; // kv_iterator_rocksdb

   struct kv_context_rocksdb : kv_context {
      chain_kv::database&                      database;
      chain_kv::undo_stack&                    undo_stack;
      name                                     database_id;
      chain_kv::write_session                  write_session;
      chain_kv::view                           view;
      name                                     receiver;
      kv_resource_manager                      resource_manager;
      const kv_database_config&                limits;
      uint32_t                                 num_iterators = 0;
      std::shared_ptr<const std::vector<char>> temp_data_buffer;

      kv_context_rocksdb(chain_kv::database& database, chain_kv::undo_stack& undo_stack, name database_id,
                         name receiver, kv_resource_manager resource_manager, const kv_database_config& limits)
          : database{ database }, undo_stack{ undo_stack }, database_id{ database_id },
            write_session{ database }, view{ write_session, make_prefix() }, receiver{ receiver },
            resource_manager{ resource_manager }, limits{ limits } {}

      ~kv_context_rocksdb() override {
         try {
            try {
               write_session.write_changes(undo_stack);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      std::vector<char> make_prefix() {
         vector<char> prefix = rocksdb_contract_kv_prefix;
         chain_kv::append_key(prefix, database_id.to_uint64_t());
         return prefix;
      }

      int64_t kv_erase(uint64_t contract, const char* key, uint32_t key_size) override {
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         temp_data_buffer = nullptr;

         std::shared_ptr<const bytes> old_value;
         try {
            try {
               old_value = view.get(contract, { key, key_size });
               if (!old_value)
                  return 0;
               view.erase(contract, { key, key_size });
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         int64_t resource_delta = -(static_cast<int64_t>(resource_manager.billable_size) + key_size + old_value->size());
         resource_manager.update_table_usage(resource_delta);
         return resource_delta;
      }

      int64_t kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                  uint32_t value_size) override {
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         EOS_ASSERT(key_size <= limits.max_key_size, kv_limit_exceeded, "Key too large");
         EOS_ASSERT(value_size <= limits.max_value_size, kv_limit_exceeded, "Value too large");
         temp_data_buffer = nullptr;

         std::shared_ptr<const bytes> old_value;
         try {
            try {
               old_value = view.get(contract, { key, key_size });
               view.set(contract, { key, key_size }, { value, value_size });
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         int64_t resource_delta;
         if (old_value) {
            // 64-bit arithmetic cannot overflow, because both the key and value are limited to 32-bits
            resource_delta = static_cast<int64_t>(value_size) - static_cast<int64_t>(old_value->size());
         } else {
            resource_delta = static_cast<int64_t>(resource_manager.billable_size) + key_size + value_size;
         }
         resource_manager.update_table_usage(resource_delta);
         return resource_delta;
      }

      bool kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) override {
         try {
            try {
               temp_data_buffer = view.get(contract, { key, key_size });
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         if (temp_data_buffer) {
            value_size = temp_data_buffer->size();
            return true;
         } else {
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
         try {
            try {
               return std::make_unique<kv_iterator_rocksdb>(num_iterators, view, contract, prefix, size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }; // kv_context_rocksdb

   std::unique_ptr<kv_context> create_kv_rocksdb_context(chain_kv::database&   kv_database,
                                                         chain_kv::undo_stack& kv_undo_stack, name database_id,
                                                         name receiver, kv_resource_manager resource_manager,
                                                         const kv_database_config& limits) {
      try {
         try {
            return std::make_unique<kv_context_rocksdb>(kv_database, kv_undo_stack, database_id, receiver,
                                                        resource_manager, limits);
         }
         FC_LOG_AND_RETHROW()
      }
      CATCH_AND_EXIT_DB_FAILURE()
   }

}} // namespace eosio::chain
