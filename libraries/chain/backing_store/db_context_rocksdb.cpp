#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_context.hpp>

namespace eosio { namespace chain { namespace backing_store {

   struct db_context_rocksdb : db_context {
      apply_context&               context;
      const name                   receiver;

      db_context_rocksdb(apply_context& context, name receiver)
         : context{ context }, receiver{ receiver } {}

      ~db_context_rocksdb() override {}

      int32_t db_store_i64(uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char* buffer , size_t buffer_size) override {
         return -1;
      }

      void db_update_i64(int32_t itr, account_name payer, const char* buffer , size_t buffer_size) override {
      }

      void db_remove_i64(int32_t itr) override {
      }

      int32_t db_get_i64(int32_t itr, char* buffer , size_t buffer_size) override {
         return -1;
      }

      int32_t db_next_i64(int32_t itr, uint64_t& primary) override {
         return -1;
      }

      int32_t db_previous_i64(int32_t itr, uint64_t& primary) override {
         return -1;
      }

      int32_t db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override {
         return -1;
      }

      int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override {
         return -1;
      }

      int32_t db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override {
         return -1;
      }

      int32_t db_end_i64(uint64_t code, uint64_t scope, uint64_t table) override {
         return -1;
      }

      /**
       * interface for uint64_t secondary
       */
      int32_t db_idx64_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                     const uint64_t& secondary) override {
         return -1;
      }

      void db_idx64_update(int32_t iterator, account_name payer, const uint64_t& secondary) override {
      }

      void db_idx64_remove(int32_t iterator) override {
      }

      int32_t db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint64_t& secondary,
                                      uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                    uint64_t primary) override {
         return -1;
      }

      int32_t db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                  uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                  uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx64_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return -1;
      }

      int32_t db_idx64_next(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx64_previous(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      /**
       * interface for uint128_t secondary
       */
      int32_t db_idx128_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                              const uint128_t& secondary) override {
         return -1;
      }

      void db_idx128_update(int32_t iterator, account_name payer, const uint128_t& secondary) override {
      }

      void db_idx128_remove(int32_t iterator) override {
      }

      int32_t db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t& secondary,
                                       uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                     uint64_t primary) override {
         return -1;
      }

      int32_t db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                   uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                   uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx128_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return -1;
      }

      int32_t db_idx128_next(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx128_previous(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      /**
       * interface for 256-bit interger secondary
       */
      int32_t db_idx256_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                              const uint128_t* data) override {
         return -1;
      }

      void db_idx256_update(int32_t iterator, account_name payer, const uint128_t* data) override {
      }

      void db_idx256_remove(int32_t iterator) override {
      }

      int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* data,
                                       uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                     uint64_t primary) override {
         return -1;
      }

      int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                   uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                   uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx256_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return -1;
      }

      int32_t db_idx256_next(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx256_previous(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      /**
       * interface for double secondary
       */
      int32_t db_idx_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                  const float64_t& secondary) override {
         return -1;
      }

      void db_idx_double_update(int32_t iterator, account_name payer, const float64_t& secondary) override {
      }

      void db_idx_double_remove(int32_t iterator) override {
      }

      int32_t db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                           const float64_t& secondary, uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                         uint64_t primary) override {
         return -1;
      }

      int32_t db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                       uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                       uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return -1;
      }

      int32_t db_idx_double_next(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx_double_previous(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      /**
       * interface for long double secondary
       */
      int32_t db_idx_long_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                       const float128_t& secondary) override {
         return -1;
      }

      void db_idx_long_double_update(int32_t iterator, account_name payer, const float128_t& secondary) override {
      }

      void db_idx_long_double_remove(int32_t iterator) override {
      }

      int32_t db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                                const float128_t& secondary, uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table,
                                              float128_t& secondary, uint64_t primary) override {
         return -1;
      }

      int32_t db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                            uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                            uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return -1;
      }

      int32_t db_idx_long_double_next(int32_t iterator, uint64_t& primary) override {
         return -1;
      }

      int32_t db_idx_long_double_previous(int32_t iterator, uint64_t& primary) override {
         return -1;
      }
   }; // db_context_rocksdb

}}} // namespace eosio::chain::backing_store
