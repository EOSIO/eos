#include <eosio/chain/backing_store/kv_context.hpp>
#include <eosio/chain/combined_database.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>

#include <b1/session/session.hpp>

namespace eosio { namespace chain {

   static inline uint32_t actual_key_size(const uint32_t raw_key_size) {
      static auto rocks_prefix = make_rocksdb_contract_kv_prefix();
      static auto prefix_size = rocks_prefix.size() + sizeof(uint64_t); // kv.db prefix + contract size.
      EOS_ASSERT(raw_key_size >= prefix_size, kv_rocksdb_bad_value_size_exception,
                 "The size of key returned from RocksDB is less than prefix size");
      return raw_key_size - prefix_size;
   }


   static inline const char* actual_key_start(const char* key) {
      static auto rocks_prefix = make_rocksdb_contract_kv_prefix();
      static auto prefix_size = rocks_prefix.size() + sizeof(uint64_t);
      return key + prefix_size;
   }

   // Need to store payer so that this account is properly
   // credited when storage is removed or changed
   // to another payer
   inline static eosio::session::shared_bytes build_value(const char* value, uint32_t value_size, const account_name& payer) {
      const uint32_t final_value_size = backing_store::payer_in_value_size + value_size;
      auto result = eosio::session::shared_bytes(final_value_size);

      char buf[backing_store::payer_in_value_size];
      memcpy(buf, &payer, backing_store::payer_in_value_size);
      std::memcpy(result.data(), buf, backing_store::payer_in_value_size);
      std::memcpy(result.data() + backing_store::payer_in_value_size, value, value_size);
      return result;
   }


   static inline eosio::session::shared_bytes make_prefix_key(uint64_t contract, const char* user_prefix, uint32_t user_prefix_size) {
      static auto rocks_prefix = make_rocksdb_contract_kv_prefix();

      auto result = eosio::session::shared_bytes(rocks_prefix.size() + sizeof(contract) + user_prefix_size);
      std::memcpy(result.data(), rocks_prefix.data(), rocks_prefix.size());
      b1::chain_kv::insert_key(result, rocks_prefix.size(), contract);
      std::memcpy(result.data() + rocks_prefix.size() + sizeof(contract), user_prefix, user_prefix_size);
      return result;
   }

   static inline eosio::session::shared_bytes
   make_composite_key(uint64_t contract, const char* user_prefix, uint32_t user_prefix_size, const char* key, uint32_t key_size) {
      static auto rocks_prefix = make_rocksdb_contract_kv_prefix(); 

      auto result = eosio::session::shared_bytes(rocks_prefix.size() + sizeof(contract) + user_prefix_size + key_size);
      std::memcpy(result.data(), rocks_prefix.data(), rocks_prefix.size());
      b1::chain_kv::insert_key(result, rocks_prefix.size(), contract);
      std::memcpy(result.data() + rocks_prefix.size() + sizeof(contract), user_prefix, user_prefix_size);
      std::memcpy(result.data() + rocks_prefix.size() + sizeof(contract) + user_prefix_size, key, key_size);
      return result;
   }

   static inline eosio::session::shared_bytes make_composite_key(const eosio::session::shared_bytes& prefix,
                                                                 const char* key, uint32_t key_size) {
      auto result = eosio::session::shared_bytes(prefix.size() + key_size);
      std::memcpy(result.data(), prefix.data(), prefix.size());
      std::memcpy(result.data() + prefix.size(), key, key_size);
      return result;
   }

   static inline int32_t compare_bytes(const eosio::session::shared_bytes& left, const eosio::session::shared_bytes& right) {
      if (left < right) {
          return -1;
      }

      if (left > right) {
          return 1;
      }

      return 0;
   };

   template <typename Session>
   struct kv_iterator_rocksdb : kv_iterator {
      using session_type = std::remove_pointer_t<Session>;

      uint32_t&                       num_iterators;
      uint64_t                        kv_contract{0};
      eosio::session::shared_bytes    kv_prefix; // Format: [contract, prefix]
      session_type*                   kv_session{nullptr};
      typename session_type::iterator kv_begin;
      typename session_type::iterator kv_end;
      typename session_type::iterator kv_current;

      kv_iterator_rocksdb(uint32_t& num_iterators, session_type& session, uint64_t contract, const char* user_prefix,
                          uint32_t user_prefix_size)
          : num_iterators{ num_iterators }, kv_contract{ contract },
            kv_prefix{ make_prefix_key(contract, user_prefix, user_prefix_size) },
            kv_session{ &session }, kv_begin{ kv_session->lower_bound(kv_prefix) }, 
            kv_end{ [&](){ auto kv_next_prefix = kv_prefix.next(); return kv_session->lower_bound(kv_next_prefix); }() },
            kv_current{ kv_end } {
         ++num_iterators;
      }

      ~kv_iterator_rocksdb() override { --num_iterators; }

      bool is_kv_chainbase_context_iterator() const override { return false; }
      bool is_kv_rocksdb_context_iterator() const override { return true; }

      kv_it_stat kv_it_status() override {
         if (kv_current == kv_end) 
            return kv_it_stat::iterator_end; 
         else if (kv_current.deleted())
            return kv_it_stat::iterator_erased;
         else
            return kv_it_stat::iterator_ok;
      }

      int32_t kv_it_compare(const kv_iterator& rhs) override {
         EOS_ASSERT(rhs.is_kv_rocksdb_context_iterator(), kv_bad_iter, "Incompatible key-value iterators");
         auto& r = const_cast<kv_iterator_rocksdb&>(static_cast<const kv_iterator_rocksdb&>(rhs));
         EOS_ASSERT(kv_session == r.kv_session && kv_prefix == r.kv_prefix, kv_bad_iter,
                    "Incompatible key-value iterators");
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");
         EOS_ASSERT(!r.kv_current.deleted(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               auto left_status = kv_it_status();
               auto right_status = r.kv_it_status();

               if (left_status == kv_it_stat::iterator_end && right_status == kv_it_stat::iterator_end) {
                  return 0;
               }
               if (left_status == kv_it_stat::iterator_end) {
                  return 1;
               }
               if (right_status == kv_it_stat::iterator_end) {
                  return -1;
               }

               auto result = compare_bytes(kv_current.key(), r.kv_current.key());
               if (result) {
                 return result;
               }
               auto left = (*kv_current).second;
               auto right = (*r.kv_current).second;
               if (left && right) {
                  return compare_bytes(*left, *right);
               }
               return left.has_value() == right.has_value();
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      int32_t kv_it_key_compare(const char* key, uint32_t size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               auto current_key = eosio::session::shared_bytes{};
               if (kv_it_status() == kv_it_stat::iterator_ok) {
                  current_key = kv_current.key();
               } else {
                  return 1;
               }
               return compare_bytes(current_key, make_composite_key(kv_contract, nullptr, 0, key, size));
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      kv_it_stat kv_it_move_to_end() override {
         try {
            try {
               kv_current = kv_end;
               return kv_it_stat::iterator_end;
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      kv_it_stat kv_it_next(uint32_t* found_key_size, uint32_t* found_value_size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");
         kv_it_stat status;
         try {
            try {
               if (kv_current == kv_end) {
                  kv_current = kv_begin;
               } else {
                  ++kv_current;
               }
               status = get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return status;
      }

      kv_it_stat kv_it_prev(uint32_t* found_key_size, uint32_t* found_value_size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");
         kv_it_stat status;
         try {
            try {
               if (kv_current == kv_begin) {
                  kv_current = kv_end;
               } else {
                  --kv_current;
               }
               status = get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return status;
      }

      kv_it_stat kv_it_lower_bound(const char* key, uint32_t size, uint32_t* found_key_size,
                                   uint32_t* found_value_size) override {
         kv_it_stat status;
         try {
            try {
               auto key_bytes = make_composite_key(kv_contract, nullptr, 0, key, size);
               if (key_bytes < kv_prefix) {
                 key_bytes = kv_prefix;
               }

               kv_current = kv_session->lower_bound(key_bytes);
               status = get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return status;
      }

      kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");

         auto key = eosio::session::shared_bytes{};
         try {
            try {
               if (kv_it_status() == kv_it_stat::iterator_ok) {
                  key = kv_current.key();
               }
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         if (key) {
            actual_size = actual_key_size(key.size());
            if (offset < actual_size) {
               std::memcpy(dest, actual_key_start(key.data()) + offset, std::min(size, actual_size - offset));
            }
            return kv_it_stat::iterator_ok;
         } else {
            actual_size = 0;
            return kv_it_stat::iterator_end;
         }
      }

      kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");

         auto kv = std::pair{ eosio::session::shared_bytes{}, std::optional<eosio::session::shared_bytes>{} };
         try {
            try {
               if (kv_it_status() == kv_it_stat::iterator_ok) {
                 kv = *kv_current;
               }
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         auto& value = kv.second;
         if (value) {
            actual_size = backing_store::actual_value_size(value->size());
            if (offset < actual_size) {
               std::memcpy(dest, backing_store::actual_value_start(value->data()) + offset, std::min(size, actual_size - offset));
            }
            return kv_it_stat::iterator_ok;
         } else {
            actual_size = 0;
            return kv_it_stat::iterator_end;
         }
      }

    private:
      kv_it_stat get_current_key_value_sizes(uint32_t* found_key_size, uint32_t* found_value_size) {
         auto status = kv_it_status();
         if (status == kv_it_stat::iterator_ok) {
            // Not end or at an erased kv pair
            auto kv = *kv_current;
            *found_value_size = backing_store::actual_value_size(
                  kv.second->size()); // This must be before *found_key_size in case actual_value_size throws
            *found_key_size = actual_key_size(kv.first.size());
         } else {
            *found_key_size   = 0;
            *found_value_size = 0;
         }
         return status;
      }
   }; // kv_iterator_rocksdb

   template <typename Session, typename Resource_manager>
   struct kv_context_rocksdb : kv_context {
      using session_type = std::remove_pointer_t<Session>;

      session_type*                               session;
      name                                        receiver;
      Resource_manager                            resource_manager;
      const kv_database_config&                   limits;
      uint32_t                                    num_iterators = 0;
      eosio::session::shared_bytes                current_key;
      std::optional<eosio::session::shared_bytes> current_value;

      kv_context_rocksdb(session_type& the_session, name receiver, Resource_manager /*kv_resource_manager*/ resource_manager,
                         const kv_database_config& limits)
          : session{ &the_session }, receiver{ receiver },
            resource_manager{ resource_manager }, limits{ limits } {}

      int64_t kv_erase(uint64_t contract, const char* key, uint32_t key_size) override {
         const name contract_name {contract};
         EOS_ASSERT(contract_name == receiver, table_operation_not_permitted, "Can not write to this key");
         
         current_value.reset();
         current_key                = eosio::session::shared_bytes{};
         auto old_value             = std::optional<eosio::session::shared_bytes>{};
         std::optional<backing_store::payer_payload> pp;
         try {
            try {
               auto composite_key = make_composite_key(contract, nullptr, 0, key, key_size);
               const auto old_value = session->read(composite_key);
               if (!old_value)
                  return 0;
               pp = backing_store::payer_payload(*old_value);
               session->erase(composite_key);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         const int64_t resource_delta = erase_table_usage(resource_manager, pp->payer, key, key_size, pp->value_size);

         if (auto dm_logger = resource_manager._context->control.get_deep_mind_logger()) {
            fc_dlog(*dm_logger, "KV_OP REM ${action_id} ${db} ${payer} ${key} ${odata}",
                    ("action_id", resource_manager._context->get_action_id())
                    ("contract", contract_name)
                    ("payer", pp->payer)
                    ("key", fc::to_hex(key, key_size))
                    ("odata", fc::to_hex(pp->value, pp->value_size))
            );
         }
         return resource_delta;
      }

      int64_t kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size,
                     account_name payer) override {
         const name contract_name {contract};
         EOS_ASSERT(contract_name == receiver, table_operation_not_permitted, "Can not write to this key");
         EOS_ASSERT(key_size <= limits.max_key_size, kv_limit_exceeded, "Key too large");
         EOS_ASSERT(value_size <= limits.max_value_size, kv_limit_exceeded, "Value too large");

         current_value.reset();
         current_key                = eosio::session::shared_bytes{};
         auto old_value             = std::optional<eosio::session::shared_bytes>{};
         std::optional<backing_store::payer_payload> old_pp;
         try {
            try {
               auto composite_key = make_composite_key(contract, nullptr, 0, key, key_size);
               old_value          = session->read(composite_key);
               if (old_value) {
                  old_pp = backing_store::payer_payload(*old_value);
               }

               backing_store::payer_payload new_value (payer, value, value_size);
               session->write(composite_key, new_value.as_payload());
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         auto resource_delta = int64_t{ 0 };
         if (old_value) {
            resource_delta =
                  update_table_usage(resource_manager, old_pp->payer, payer, key, key_size, old_pp->value_size, value_size);

            if (auto dm_logger = resource_manager._context->control.get_deep_mind_logger()) {
               fc_dlog(*dm_logger, "KV_OP UPD ${action_id} ${db} ${payer} ${key} ${odata}:${ndata}",
                       ("action_id", resource_manager._context->get_action_id())
                       ("contract", contract_name)
                       ("payer", payer)
                       ("key", fc::to_hex(key, key_size))
                       ("odata", fc::to_hex(old_pp->value, old_pp->value_size))
                       ("ndata", fc::to_hex(value, value_size))
               );
            }
         } else {
            resource_delta = create_table_usage(resource_manager, payer, key, key_size, value_size);

            if (auto dm_logger = resource_manager._context->control.get_deep_mind_logger()) {
               fc_dlog(*dm_logger, "KV_OP INS ${action_id} ${db} ${payer} ${key} ${ndata}",
                       ("action_id", resource_manager._context->get_action_id())
                       ("contract", contract_name)
                       ("payer", payer)
                       ("key", fc::to_hex(key, key_size))
                       ("ndata", fc::to_hex(value, value_size))
               );
            }
         }

         return resource_delta;
      }

      bool kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) override {
         current_key = eosio::session::shared_bytes{};
         current_value.reset();
         try {
            try {
               current_key   = make_composite_key(contract, nullptr, 0, key, key_size);
               current_value = session->read(current_key);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         if (current_value) {
            value_size = backing_store::actual_value_size(current_value->size());
            return true;
         } else {
            value_size = 0;
            return false;
         }
      }

      uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) override {
         uint32_t temp_size = 0;
         if (current_value) {
            temp_size = backing_store::actual_value_size(current_value->size());
         }
         if (offset < temp_size) {
            const char* temp = backing_store::actual_value_start(current_value->data());
            std::memcpy(data, temp + offset, std::min(data_size, temp_size - offset));
         }
         return temp_size;
      }

      std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) override {
         EOS_ASSERT(num_iterators < limits.max_iterators, kv_bad_iter, "Too many iterators");
         EOS_ASSERT(size <= limits.max_key_size, kv_bad_iter, "Prefix too large");
         try {
            try {
               return std::make_unique<kv_iterator_rocksdb<decltype(session)>>(num_iterators, *session, contract, prefix,
                                                                               size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }; // kv_context_rocksdb

   template <typename Session, typename Resource_manager>
   std::unique_ptr<kv_context> create_kv_rocksdb_context(std::remove_pointer_t<Session>& session, name receiver,
                                                         Resource_manager          resource_manager,
                                                         const kv_database_config& limits) {
      try {
         try {
            return std::make_unique<kv_context_rocksdb<Session, Resource_manager>>(session, receiver,
                                                                                   resource_manager, limits);
         }
         FC_LOG_AND_RETHROW()
      }
      CATCH_AND_EXIT_DB_FAILURE()
   }

}} // namespace eosio::chain
