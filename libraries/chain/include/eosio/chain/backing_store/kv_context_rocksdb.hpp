#include <eosio/chain/combined_database.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/backing_store/kv_context.hpp>

namespace eosio { namespace chain {
   static constexpr auto kv_payer_size = sizeof(account_name);

   inline static uint32_t actual_value_size(const uint32_t raw_value_size) {
      EOS_ASSERT(raw_value_size >= kv_payer_size, kv_rocksdb_bad_value_size_exception , "The size of value returned from RocksDB is less than payer's size");
      return (raw_value_size - kv_payer_size);
   }

   inline static account_name get_payer(const char* data) {
      account_name payer;
      memcpy(&payer, data, kv_payer_size); // Before this method is called, data was checked to be at least kv_payer_size long
      return payer;
   }

   inline static const char* actual_value_start(const char* data) {
      return data + kv_payer_size;
   }

   // Need to store payer so that this account is properly
   // credited when storage is removed or changed
   // to another payer
   inline static void build_value(const char* value, uint32_t value_size, const account_name& payer, bytes& final_kv_value) {
      const uint32_t final_value_size = kv_payer_size + value_size;
      final_kv_value.reserve(final_value_size);

      char buf[kv_payer_size];
      memcpy(buf, &payer, kv_payer_size);
      final_kv_value.insert(final_kv_value.end(), std::begin(buf), std::end(buf));

      final_kv_value.insert(final_kv_value.end(), value, value + value_size); 
   }

   template <typename View>
   struct kv_iterator_rocksdb : kv_iterator {
      uint32_t&                num_iterators;
      View&                    view;
      uint64_t                 contract;
      typename View::iterator  kv_it;

      kv_iterator_rocksdb(uint32_t& num_iterators, View& view, uint64_t contract, const char* prefix,
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
               return b1::chain_kv::compare_key(kv_it.get_kv(), b1::chain_kv::key_value{ { key, size }, {} });
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

      kv_it_stat kv_it_next(uint32_t* found_key_size, uint32_t* found_value_size) override {
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               ++kv_it;
               get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_prev(uint32_t* found_key_size, uint32_t* found_value_size) override {
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               --kv_it;
               get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_lower_bound(const char* key, uint32_t size, uint32_t* found_key_size, uint32_t* found_value_size) override {
         try {
            try {
               kv_it.lower_bound(key, size);
               get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         EOS_ASSERT(!kv_it.is_erased(), kv_bad_iter, "Iterator to erased element");

         std::optional<b1::chain_kv::key_value> kv;
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

         std::optional<b1::chain_kv::key_value> kv;
         try {
            try {
               kv = kv_it.get_kv();
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         if (kv) {
            actual_size = actual_value_size( kv->value.size() );
            if (offset < actual_size)
               memcpy(dest, actual_value_start(kv->value.data()) + offset, std::min(size, actual_size - offset));
            return kv_it_stat::iterator_ok;
         } else {
            actual_size = 0;
            return kv_it_stat::iterator_end;
         }
      }

    private:
      void get_current_key_value_sizes(uint32_t* found_key_size, uint32_t* found_value_size) {
         if (kv_it.is_valid()) {
            // Not end or at an erased kv pair
            auto kv = kv_it.get_kv();

            // kv is always non-null due to the check of is_valid()
            *found_value_size = actual_value_size( kv->value.size() ); // This must be before *found_key_size in case actual_value_size throws
            *found_key_size = kv->key.size();
         } else {
            *found_key_size = 0;
            *found_value_size = 0;
         }
      }
   }; // kv_iterator_rocksdb

   template<typename View, typename Write_session, typename Resource_manager>
   struct kv_context_rocksdb : kv_context {
      b1::chain_kv::database&                  database;
      b1::chain_kv::undo_stack&                undo_stack;
      Write_session                            write_session;
      View                                     view;
      name                                     receiver;
      Resource_manager                         resource_manager;
      const kv_database_config&                limits;
      uint32_t                                 num_iterators = 0;
      std::shared_ptr<const std::vector<char>> temp_data_buffer;

      kv_context_rocksdb(b1::chain_kv::database& database, b1::chain_kv::undo_stack& undo_stack,
                         name receiver, Resource_manager /*kv_resource_manager*/ resource_manager, const kv_database_config& limits)
          : database{ database }, undo_stack{ undo_stack },
            write_session{ database }, view{ write_session, make_rocksdb_contract_kv_prefix() }, receiver{ receiver },
            resource_manager{ resource_manager }, limits{ limits } {}

      // A hook for unit testing. Do not use this for any other purpose.
      kv_context_rocksdb(View&& view_, b1::chain_kv::database& database, b1::chain_kv::undo_stack& undo_stack, name receiver, Resource_manager resource_manager, const kv_database_config& limits)
         : database{ database }, undo_stack{ undo_stack },
           write_session{ database }, view(std::move(view_)), 
           receiver{ receiver },
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

      int64_t kv_erase(uint64_t contract, const char* key, uint32_t key_size) override {
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         temp_data_buffer = nullptr;

         std::shared_ptr<const bytes> old_value;
         uint32_t actual_old_value_size;
         try {
            try {
               old_value = view.get(contract, { key, key_size });
               if (!old_value)
                  return 0;
               actual_old_value_size = actual_value_size(old_value->size()); 
               view.erase(contract, { key, key_size });
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         account_name payer = get_payer(old_value->data());

         return erase_table_usage(resource_manager, payer, key, key_size, actual_old_value_size);
      }

      int64_t kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                  uint32_t value_size, account_name payer) override {
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         EOS_ASSERT(key_size <= limits.max_key_size, kv_limit_exceeded, "Key too large");
         EOS_ASSERT(value_size <= limits.max_value_size, kv_limit_exceeded, "Value too large");
         temp_data_buffer = nullptr;

         std::shared_ptr<const bytes> old_value;
         int64_t old_value_size = 0;
         try {
            try {
               old_value = view.get(contract, { key, key_size });
               if (old_value) {
                  old_value_size = actual_value_size(old_value->size());
               }

               bytes final_value;
               build_value(value, value_size, payer,final_value);
               view.set(contract, { key, key_size }, { final_value.data(), final_value.size() });
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         int64_t resource_delta;
         if (old_value) {
            account_name old_payer = get_payer(old_value->data());

            resource_delta = update_table_usage(resource_manager, old_payer, payer, key, key_size, old_value_size, value_size);
         } else {
            resource_delta = create_table_usage(resource_manager, payer, key, key_size, value_size);
         }

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
            value_size = actual_value_size(temp_data_buffer->size());
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
            temp      = actual_value_start(temp_data_buffer->data());
            temp_size = actual_value_size(temp_data_buffer->size());
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
               return std::make_unique<kv_iterator_rocksdb<View>>(num_iterators, view, contract, prefix, size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }; // kv_context_rocksdb

   template <typename View, typename Write_session, typename Resource_manager>
   std::unique_ptr<kv_context> create_kv_rocksdb_context(b1::chain_kv::database&   kv_database,
                                                         b1::chain_kv::undo_stack& kv_undo_stack,
                                                         name receiver, Resource_manager resource_manager,
                                                         const kv_database_config& limits) {
      try {
         try {
            return std::make_unique<kv_context_rocksdb<View, Write_session, Resource_manager>>(kv_database, kv_undo_stack, receiver,
                                                        resource_manager, limits);
         }
         FC_LOG_AND_RETHROW()
      }
      CATCH_AND_EXIT_DB_FAILURE()
   }

}} // namespace eosio::chain
