#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/kv_context.hpp>

namespace eosio { namespace chain {

   struct kv_iterator_chainbase : kv_iterator {
      kv_iterator_chainbase() = default;
      ~kv_iterator_chainbase() override {}

      int32_t kv_it_status() override { throw std::runtime_error("not implemented"); }

      int32_t kv_it_compare(const kv_iterator& rhs) override { throw std::runtime_error("not implemented"); }

      int32_t kv_it_key_compare(const char* key, uint32_t size) override {
         throw std::runtime_error("not implemented");
      }

      int32_t kv_it_move_to_oob() override { throw std::runtime_error("not implemented"); }

      int32_t kv_it_increment() override { throw std::runtime_error("not implemented"); }

      int32_t kv_it_decrement() override { throw std::runtime_error("not implemented"); }

      int32_t kv_it_lower_bound(const char* key, uint32_t size) override {
         throw std::runtime_error("not implemented");
      }

      int32_t kv_it_key(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         throw std::runtime_error("not implemented");
      }

      int32_t kv_it_value(uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size) override {
         throw std::runtime_error("not implemented");
      }
   }; // kv_iterator_chainbase

   struct kv_context_chainbase : kv_context {
      chainbase::database& db;
      uint8_t              database_id;
      name                 receiver;

      kv_context_chainbase(chainbase::database& db, uint8_t database_id, name receiver)
          : db{ db }, database_id{ database_id }, receiver{ receiver } {}

      ~kv_context_chainbase() override {}

      void kv_erase(uint64_t contract, const char* key, uint32_t key_size) override {
         throw std::runtime_error("not implemented");
      }

      void kv_set(uint64_t contract, const char* key, uint32_t key_size, const char* value,
                  uint32_t value_size) override {
         throw std::runtime_error("not implemented");
      }

      bool kv_get(uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size) override {
         throw std::runtime_error("not implemented");
      }

      uint32_t kv_get_data(uint32_t offset, char* data, uint32_t data_size) override {
         throw std::runtime_error("not implemented");
      }

      std::unique_ptr<kv_iterator> kv_it_create(uint64_t contract, const char* prefix, uint32_t size) override {
         return std::make_unique<kv_iterator_chainbase>();
      }
   }; // kv_context_chainbase

   std::unique_ptr<kv_context> create_kv_chainbase_context(chainbase::database& db, uint8_t database_id,
                                                           name receiver) {
      return std::make_unique<kv_context_chainbase>(db, database_id, receiver);
   }

}} // namespace eosio::chain
