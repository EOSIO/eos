#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_context.hpp>

namespace eosio { namespace chain { namespace backing_store {

   struct db_context_chainbase : db_context {
      apply_context&               context;
      const name                   receiver;

      db_context_chainbase(apply_context& context, name receiver)
         : context{ context }, receiver{ receiver } {}

      int32_t db_store_i64(uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char* buffer , size_t buffer_size) override {
         return context.db_store_i64(name(scope), name(table), payer, id, buffer, buffer_size);
      }

      void db_update_i64(int32_t itr, account_name payer, const char* buffer , size_t buffer_size) override {
         context.db_update_i64(itr, payer, buffer, buffer_size);
      }

      void db_remove_i64(int32_t itr) override {
         context.db_remove_i64(itr);
      }

      int32_t db_get_i64(int32_t itr, char* buffer , size_t buffer_size) override {
         return context.db_get_i64(itr, buffer, buffer_size);
      }

      int32_t db_next_i64(int32_t itr, uint64_t& primary) override {
         return context.db_next_i64(itr, primary);
      }

      int32_t db_previous_i64(int32_t itr, uint64_t& primary) override {
         return context.db_previous_i64(itr, primary);
      }

      int32_t db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override {
         return context.db_find_i64(name(code), name(scope), name(table), id);
      }

      int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override {
         return context.db_lowerbound_i64(name(code), name(scope), name(table), id);
      }

      int32_t db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override {
         return context.db_upperbound_i64(name(code), name(scope), name(table), id);
      }

      int32_t db_end_i64(uint64_t code, uint64_t scope, uint64_t table) override {
         return context.db_end_i64(name(code), name(scope), name(table));
      }

      /**
       * interface for uint64_t secondary
       */
      int32_t db_idx64_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                     const uint64_t& secondary) override {
         return context.idx64.store( scope, table, payer, id, secondary );
      }

      void db_idx64_update(int32_t iterator, account_name payer, const uint64_t& secondary) override {
         context.idx64.update( iterator, payer, secondary );
      }

      void db_idx64_remove(int32_t iterator) override {
         return context.idx64.remove(iterator);
      }

      int32_t db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint64_t& secondary,
                                      uint64_t& primary) override {
         return context.idx64.find_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                    uint64_t primary) override {
         return context.idx64.find_primary(code, scope, table, secondary, primary);
      }

      int32_t db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                  uint64_t& primary) override {
         return context.idx64.lowerbound_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                  uint64_t& primary) override {
         return context.idx64.upperbound_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx64_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return context.idx64.end_secondary(code, scope, table);
      }

      int32_t db_idx64_next(int32_t iterator, uint64_t& primary) override {
         return context.idx64.next_secondary(iterator, primary);
      }

      int32_t db_idx64_previous(int32_t iterator, uint64_t& primary) override {
         return context.idx64.previous_secondary(iterator, primary);
      }

      /**
       * interface for uint128_t secondary
       */
      int32_t db_idx128_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                              const uint128_t& secondary) override {
         return context.idx128.store(scope, table, payer, id, secondary);
      }

      void db_idx128_update(int32_t iterator, account_name payer, const uint128_t& secondary) override {
         return context.idx128.update(iterator, payer, secondary);
      }

      void db_idx128_remove(int32_t iterator) override {
         context.idx128.remove(iterator);
      }

      int32_t db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t& secondary,
                                       uint64_t& primary) override {
         return context.idx128.find_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                             uint64_t primary) override {
         return context.idx128.find_primary(code, scope, table, secondary, primary);
      }

      int32_t db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                           uint64_t& primary) override {
         return context.idx128.lowerbound_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                           uint64_t& primary) override {
         return context.idx128.upperbound_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx128_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return context.idx128.end_secondary(code, scope, table);
      }

      int32_t db_idx128_next(int32_t iterator, uint64_t& primary) override {
         return context.idx128.next_secondary(iterator, primary);
      }

      int32_t db_idx128_previous(int32_t iterator, uint64_t& primary) override {
         return context.idx128.previous_secondary(iterator, primary);
      }

      /**
       * interface for 256-bit interger secondary
       */
      int32_t db_idx256_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                      const uint128_t* data) override {
         return context.idx256.store(scope, table, payer, id, data);
      }

      void db_idx256_update(int32_t iterator, account_name payer, const uint128_t* data) override {
         context.idx256.update(iterator, payer, data);
      }

      void db_idx256_remove(int32_t iterator) override {
         context.idx256.remove(iterator);
      }

      int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* data,
                                       uint64_t& primary) override {
         return context.idx256.find_secondary(code, scope, table, data, primary);
      }

      int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                     uint64_t primary) override {
         return context.idx256.find_primary(code, scope, table, data, primary);
      }

      int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                   uint64_t& primary) override {
         return context.idx256.lowerbound_secondary(code, scope, table, data, primary);
      }

      int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                   uint64_t& primary) override {
         return context.idx256.upperbound_secondary(code, scope, table, data, primary);
      }

      int32_t db_idx256_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return context.idx256.end_secondary(code, scope, table);
      }

      int32_t db_idx256_next(int32_t iterator, uint64_t& primary) override {
         return context.idx256.next_secondary(iterator, primary);
      }

      int32_t db_idx256_previous(int32_t iterator, uint64_t& primary) override {
         return context.idx256.previous_secondary(iterator, primary);
      }

      /**
       * interface for double secondary
       */
      int32_t db_idx_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                  const float64_t& secondary) override {
         return context.idx_double.store(scope, table, payer, id, secondary);
      }

      void db_idx_double_update(int32_t iterator, account_name payer, const float64_t& secondary) override {
         context.idx_double.update(iterator, payer, secondary);
      }

      void db_idx_double_remove(int32_t iterator) override {
         context.idx_double.remove(iterator);
      }

      int32_t db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                           const float64_t& secondary, uint64_t& primary) override {
         return context.idx_double.find_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                         uint64_t primary) override {
         return context.idx_double.find_primary(code, scope, table, secondary, primary);
      }

      int32_t db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                       uint64_t& primary) override {
         return context.idx_double.lowerbound_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                       uint64_t& primary) override {
         return context.idx_double.upperbound_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return context.idx_double.end_secondary(code, scope, table);
      }

      int32_t db_idx_double_next(int32_t iterator, uint64_t& primary) override {
         return context.idx_double.next_secondary(iterator, primary);
      }

      int32_t db_idx_double_previous(int32_t iterator, uint64_t& primary) override {
         return context.idx_double.previous_secondary(iterator, primary);
      }

      /**
       * interface for long double secondary
       */
      int32_t db_idx_long_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                       const float128_t& secondary) override {
         return context.idx_long_double.store(scope, table, payer, id, secondary);
      }

      void db_idx_long_double_update(int32_t iterator, account_name payer, const float128_t& secondary) override {
         context.idx_long_double.update(iterator, payer, secondary);
      }

      void db_idx_long_double_remove(int32_t iterator) override {
         context.idx_long_double.remove(iterator);
      }

      int32_t db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                                const float128_t& secondary, uint64_t& primary) override {
         return context.idx_long_double.find_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table,
                                              float128_t& secondary, uint64_t primary) override {
         return context.idx_long_double.find_primary(code, scope, table, secondary, primary);
      }

      int32_t db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                            uint64_t& primary) override {
         return context.idx_long_double.lowerbound_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                            uint64_t& primary) override {
         return context.idx_long_double.upperbound_secondary(code, scope, table, secondary, primary);
      }

      int32_t db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table) override {
         return context.idx_long_double.end_secondary(code, scope, table);
      }

      int32_t db_idx_long_double_next(int32_t iterator, uint64_t& primary) override {
         return context.idx_long_double.next_secondary(iterator, primary);
      }

      int32_t db_idx_long_double_previous(int32_t iterator, uint64_t& primary) override {
         return context.idx_long_double.previous_secondary(iterator, primary);
      }
   }; // db_context_chainbase

   std::unique_ptr<db_context> create_db_chainbase_context(apply_context& context, name receiver)
   {
      return std::make_unique<db_context_chainbase>(context, receiver);
   }

}}} // namespace eosio::chain::backing_store
