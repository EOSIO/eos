#include <eosio/chain/backing_store/kv_context.hpp>
#include <eosio/chain/combined_database.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>

#include <b1/session/session.hpp>

namespace eosio { namespace chain {
   static constexpr auto kv_payer_size = sizeof(account_name);

   inline static uint32_t actual_value_size(const uint32_t raw_value_size) {
      EOS_ASSERT(raw_value_size >= kv_payer_size, kv_rocksdb_bad_value_size_exception , "The size of value returned from RocksDB is less than payer's size");
      return (raw_value_size - kv_payer_size);
   }

   inline static account_name get_payer(const char* data) {
      account_name payer;
      memcpy(&payer, data,
             kv_payer_size); // Before this method is called, data was checked to be at least kv_payer_size long
      return payer;
   }
   
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


   static inline eosio::session::shared_bytes make_prefix_key(uint64_t contract, const char* user_prefix, uint32_t user_prefix_size) {
      static auto rocks_prefix = make_rocksdb_contract_kv_prefix();

      auto buffer = std::vector<char>{};
      buffer.reserve(rocks_prefix.size() + sizeof(contract) + user_prefix_size);
      buffer.insert(std::end(buffer), std::begin(rocks_prefix), std::end(rocks_prefix));
      b1::chain_kv::append_key(buffer, contract);
      buffer.insert(std::end(buffer), user_prefix, user_prefix + user_prefix_size);
      return eosio::session::make_shared_bytes(buffer.data(), buffer.size());
   }

   static inline eosio::session::shared_bytes
   make_composite_key(uint64_t contract, const char* user_prefix, uint32_t user_prefix_size, const char* key, uint32_t key_size) {
      static auto rocks_prefix = make_rocksdb_contract_kv_prefix(); 

      auto buffer = std::vector<char>{};
      buffer.reserve(rocks_prefix.size() + sizeof(contract) + user_prefix_size + key_size);
      buffer.insert(std::end(buffer), std::begin(rocks_prefix), std::end(rocks_prefix));
      b1::chain_kv::append_key(buffer, contract);
      buffer.insert(std::end(buffer), user_prefix, user_prefix + user_prefix_size);
      buffer.insert(std::end(buffer), key, key + key_size);
      return eosio::session::make_shared_bytes(buffer.data(), buffer.size());
   }

   static inline eosio::session::shared_bytes make_composite_key(const eosio::session::shared_bytes& prefix,
                                                                 const char* key, uint32_t key_size) {
      auto buffer = std::vector<char>{};
      buffer.reserve(prefix.size() + key_size);
      buffer.insert(std::end(buffer), prefix.data(), prefix.data() + prefix.size());
      buffer.insert(std::end(buffer), key, key + key_size);
      return eosio::session::make_shared_bytes(buffer.data(), buffer.size());
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
      const char*                     kv_user_prefix{nullptr};
      uint32_t                        kv_user_prefix_size{0};
      eosio::session::shared_bytes    kv_prefix; // Format: [contract, prefix]
      eosio::session::shared_bytes    kv_next_prefix;
      session_type*                   kv_session{nullptr};
      typename session_type::iterator kv_current;

      kv_iterator_rocksdb(uint32_t& num_iterators, session_type& session, uint64_t contract, const char* user_prefix,
                          uint32_t user_prefix_size)
          : num_iterators{ num_iterators }, kv_contract{ contract }, kv_user_prefix{ user_prefix }, kv_user_prefix_size{ user_prefix_size },
            kv_prefix{ make_prefix_key(contract, user_prefix, user_prefix_size) },
            kv_next_prefix{ kv_prefix++ }, kv_session{ &session }, 
            kv_current{ kv_session->lower_bound(kv_next_prefix) } {
         ++num_iterators;
      }

      ~kv_iterator_rocksdb() override { --num_iterators; }

      bool is_kv_chainbase_context_iterator() const override { return false; }
      bool is_kv_rocksdb_context_iterator() const override { return true; }

      kv_it_stat kv_it_status() override {
         if (kv_current == std::end(*kv_session))
            return kv_it_stat::iterator_end; 
         else if (kv_current == kv_session->lower_bound(kv_next_prefix)) 
            return kv_it_stat::iterator_end; 
         else if ((*kv_current).first < kv_prefix) 
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

               auto result = compare_bytes((*kv_current).first, (*r.kv_current).first);
               if (result) {
                 return result;
               }
               return compare_bytes((*kv_current).second, (*r.kv_current).second);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      int32_t kv_it_key_compare(const char* key, uint32_t size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               auto current_key = eosio::session::shared_bytes::invalid();
               if (kv_it_status() == kv_it_stat::iterator_ok) {
                  current_key = (*kv_current).first;
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
               kv_current = std::end(*kv_session);
               return kv_it_stat::iterator_end;
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }

      kv_it_stat kv_it_next(uint32_t* found_key_size, uint32_t* found_value_size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               if (kv_it_status() == kv_it_stat::iterator_end) {
                  kv_current = kv_session->lower_bound(kv_prefix);
               } else {
                  ++kv_current;
               }
               get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_prev(uint32_t* found_key_size, uint32_t* found_value_size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");
         try {
            try {
               if (kv_current == kv_session->lower_bound(kv_prefix)) {
                  kv_current = kv_session->lower_bound(kv_next_prefix);
               } else {
                  --kv_current;
               }
               get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_lower_bound(const char* key, uint32_t size, uint32_t* found_key_size,
                                   uint32_t* found_value_size) override {
         try {
            try {
               auto key_bytes = make_composite_key(kv_contract, nullptr, 0, key, size);
               if (key_bytes < kv_prefix) {
                 key_bytes = kv_prefix;
               }

               kv_current = kv_session->lower_bound(key_bytes);
               get_current_key_value_sizes(found_key_size, found_value_size);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         return kv_it_status();
      }

      kv_it_stat kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");

         auto kv = std::pair{ eosio::session::shared_bytes::invalid(), eosio::session::shared_bytes::invalid() };
         try {
            try {
               if (kv_it_status() == kv_it_stat::iterator_ok) {
                  kv = *kv_current;
               }
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         auto& key = kv.first;
         if (key != eosio::session::shared_bytes::invalid()) {
            actual_size = actual_key_size(key.size());
            if (offset < actual_size) {
               memcpy(dest, actual_key_start(key.data()) + offset, std::min(size, actual_size - offset));
            }
            return kv_it_stat::iterator_ok;
         } else {
            actual_size = 0;
            return kv_it_stat::iterator_end;
         }
      }

      kv_it_stat kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         EOS_ASSERT(!kv_current.deleted(), kv_bad_iter, "Iterator to erased element");

         auto kv = std::pair{ eosio::session::shared_bytes::invalid(), eosio::session::shared_bytes::invalid() };
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
         if (value != eosio::session::shared_bytes::invalid()) {
            actual_size = actual_value_size(value.size());
            if (offset < actual_size)
               memcpy(dest, actual_value_start(value.data()) + offset, std::min(size, actual_size - offset));
            return kv_it_stat::iterator_ok;
         } else {
            actual_size = 0;
            return kv_it_stat::iterator_end;
         }
      }

    private:
      void get_current_key_value_sizes(uint32_t* found_key_size, uint32_t* found_value_size) {
         if (kv_it_status() == kv_it_stat::iterator_ok) {
            // Not end or at an erased kv pair
            auto kv = *kv_current;
            // kv is always non-null due to the check of is_valid()
            *found_value_size = actual_value_size(
                  kv.second.size()); // This must be before *found_key_size in case actual_value_size throws
            *found_key_size = actual_key_size(kv.first.size());
         } else {
            *found_key_size   = 0;
            *found_value_size = 0;
         }
      }
   }; // kv_iterator_rocksdb

   template <typename Session, typename Resource_manager>
   struct kv_context_rocksdb : kv_context {
      using session_type = std::remove_pointer_t<Session>;

      session_type*                session;
      name                         receiver;
      Resource_manager             resource_manager;
      const kv_database_config&    limits;
      uint32_t                     num_iterators = 0;
      eosio::session::shared_bytes current_key{ eosio::session::shared_bytes::invalid() };
      eosio::session::shared_bytes current_value{ eosio::session::shared_bytes::invalid() };

      kv_context_rocksdb(session_type& the_session, name receiver, Resource_manager /*kv_resource_manager*/ resource_manager,
                         const kv_database_config& limits)
          : session{ &the_session }, receiver{ receiver },
            resource_manager{ resource_manager }, limits{ limits } {}

      int64_t kv_erase(uint64_t contract, const char* key, uint32_t key_size) override {
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");

         current_key                = eosio::session::shared_bytes::invalid();
         current_value              = eosio::session::shared_bytes::invalid();
         auto old_value             = eosio::session::shared_bytes::invalid();
         auto actual_old_value_size = uint32_t{ 0 };
         try {
            try {
               auto composite_key = make_composite_key(contract, nullptr, 0, key, key_size);
               old_value          = session->read(composite_key);
               if (!old_value)
                  return 0;
               actual_old_value_size = actual_value_size(old_value.size());
               session->erase(composite_key);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         account_name payer = get_payer(old_value.data());
         return erase_table_usage(resource_manager, payer, key, key_size, actual_old_value_size);
      }

      int64_t kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size,
                     account_name payer) override {
         EOS_ASSERT(name{ contract } == receiver, table_operation_not_permitted, "Can not write to this key");
         EOS_ASSERT(key_size <= limits.max_key_size, kv_limit_exceeded, "Key too large");
         EOS_ASSERT(value_size <= limits.max_value_size, kv_limit_exceeded, "Value too large");

         current_key         = eosio::session::shared_bytes::invalid();
         current_value       = eosio::session::shared_bytes::invalid();
         auto old_value      = eosio::session::shared_bytes::invalid();
         auto old_value_size = int64_t{ 0 };
         try {
            try {
               auto composite_key = make_composite_key(contract, nullptr, 0, key, key_size);
               old_value          = session->read(composite_key);
               if (old_value != eosio::session::shared_bytes::invalid()) {
                  old_value_size = actual_value_size(old_value.size());
               }

               bytes final_value;
               build_value(value, value_size, payer,final_value);
               auto new_value = eosio::session::make_shared_bytes(final_value.data(), final_value.size());
               session->write(composite_key, new_value);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         auto resource_delta = int64_t{ 0 };
         if (old_value != eosio::session::shared_bytes::invalid()) {
            account_name old_payer = get_payer(old_value.data());
            resource_delta =
                  update_table_usage(resource_manager, old_payer, payer, key, key_size, old_value_size, value_size);
         } else {
            resource_delta = create_table_usage(resource_manager, payer, key, key_size, value_size);
         }

         return resource_delta;
      }

      bool kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) override {
         current_key   = eosio::session::shared_bytes::invalid();
         current_value = eosio::session::shared_bytes::invalid();
         try {
            try {
               current_key   = make_composite_key(contract, nullptr, 0, key, key_size);
               current_value = session->read(current_key);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()

         if (current_value != eosio::session::shared_bytes::invalid()) {
            value_size = actual_value_size(current_value.size());
            return true;
         } else {
            value_size = 0;
            return false;
         }
      }

      uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) override {
         const char* temp        = nullptr;
         uint32_t      temp_size = 0;
         if (current_value != eosio::session::shared_bytes::invalid()) {
            temp      = actual_value_start(current_value.data());
            temp_size = actual_value_size(current_value.size());
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
