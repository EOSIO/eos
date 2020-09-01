#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/apply_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   /**
    * interface for primary index
    */
   int32_t interface::db_store_i64( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_span<const char> buffer ) {
      return context.db_get_context().db_store_i64( scope, table, account_name(payer), id, buffer.data(), buffer.size() );
   }
   void interface::db_update_i64( int32_t itr, uint64_t payer, legacy_span<const char> buffer ) {
      context.db_get_context().db_update_i64( itr, account_name(payer), buffer.data(), buffer.size() );
   }
   void interface::db_remove_i64( int32_t itr ) {
      context.db_get_context().db_remove_i64( itr );
   }
   int32_t interface::db_get_i64( int32_t itr, legacy_span<char> buffer ) {
      return context.db_get_context().db_get_i64( itr, buffer.data(), buffer.size() );
   }
   int32_t interface::db_next_i64( int32_t itr, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_next_i64(itr, *primary);
   }
   int32_t interface::db_previous_i64( int32_t itr, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_previous_i64(itr, *primary);
   }
   int32_t interface::db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
      return context.db_get_context().db_find_i64( code, scope, table, id );
   }
   int32_t interface::db_lowerbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
      return context.db_get_context().db_lowerbound_i64( code, scope, table, id );
   }
   int32_t interface::db_upperbound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id ) {
      return context.db_get_context().db_upperbound_i64( code, scope, table, id );
   }
   int32_t interface::db_end_i64( uint64_t code, uint64_t scope, uint64_t table ) {
      return context.db_get_context().db_end_i64( code, scope, table );
   }

   /**
    * interface for uint64_t secondary
    */
   int32_t interface::db_idx64_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_ptr<const uint64_t> secondary ) {
      return context.db_get_context().db_idx64_store( scope, table, account_name(payer), id, *secondary );
   }
   void interface::db_idx64_update( int32_t iterator, uint64_t payer, legacy_ptr<const uint64_t> secondary ) {
      return context.db_get_context().db_idx64_update( iterator, account_name(payer), *secondary );
   }
   void interface::db_idx64_remove( int32_t iterator ) {
      return context.db_get_context().db_idx64_remove( iterator );
   }
   int32_t interface::db_idx64_find_secondary( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const uint64_t> secondary, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx64_find_secondary(code, scope, table, *secondary, *primary);
   }
   int32_t interface::db_idx64_find_primary( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint64_t> secondary, uint64_t primary ) {
      return context.db_get_context().db_idx64_find_primary(code, scope, table, *secondary, primary);
   }
   int32_t interface::db_idx64_lowerbound( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint64_t> secondary, legacy_ptr<uint64_t> primary ) {
      int32_t result = context.db_get_context().db_idx64_lowerbound(code, scope, table, *secondary, *primary);
      (void)legacy_ptr<uint64_t>(std::move(secondary));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx64_upperbound( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint64_t> secondary, legacy_ptr<uint64_t> primary ) {
      int32_t result = context.db_get_context().db_idx64_upperbound(code, scope, table, *secondary, *primary);
      (void)legacy_ptr<uint64_t>(std::move(secondary));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx64_end( uint64_t code, uint64_t scope, uint64_t table ) {
      return context.db_get_context().db_idx64_end(code, scope, table);
   }
   int32_t interface::db_idx64_next( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx64_next(iterator, *primary);
   }
   int32_t interface::db_idx64_previous( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx64_previous(iterator, *primary);
   }


   /**
    * interface for uint128_t secondary
    */
   int32_t interface::db_idx128_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_ptr<const uint128_t> secondary ) {
      return context.db_get_context().db_idx128_store( scope, table, account_name(payer), id, *secondary );
   }
   void interface::db_idx128_update( int32_t iterator, uint64_t payer, legacy_ptr<const uint128_t> secondary ) {
      return context.db_get_context().db_idx128_update( iterator, account_name(payer), *secondary );
   }
   void interface::db_idx128_remove( int32_t iterator ) {
      return context.db_get_context().db_idx128_remove( iterator );
   }
   int32_t interface::db_idx128_find_secondary( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const uint128_t> secondary, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx128_find_secondary(code, scope, table, *secondary, *primary);
   }
   int32_t interface::db_idx128_find_primary( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint128_t> secondary, uint64_t primary ) {
      return context.db_get_context().db_idx128_find_primary(code, scope, table, *secondary, primary);
   }
   int32_t interface::db_idx128_lowerbound( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint128_t> secondary, legacy_ptr<uint64_t> primary ) {
      int32_t result = context.db_get_context().db_idx128_lowerbound(code, scope, table, *secondary, *primary);
      (void)legacy_ptr<uint128_t>(std::move(secondary));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx128_upperbound( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<uint128_t> secondary, legacy_ptr<uint64_t> primary ) {
      int32_t result = context.db_get_context().db_idx128_upperbound(code, scope, table, *secondary, *primary);
      (void)legacy_ptr<uint128_t>(std::move(secondary));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx128_end( uint64_t code, uint64_t scope, uint64_t table ) {
      return context.db_get_context().db_idx128_end(code, scope, table);
   }
   int32_t interface::db_idx128_next( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx128_next(iterator, *primary);
   }
   int32_t interface::db_idx128_previous( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx128_previous(iterator, *primary);
   }

   /**
    * interface for 256-bit interger secondary
    */
   inline static constexpr uint32_t idx256_array_size = 2;
   int32_t interface::db_idx256_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_span<const uint128_t> data ) {
      EOS_ASSERT( data.size() == idx256_array_size,
                    db_api_exception,
                    "invalid size of secondary key array for idx256: given ${given} bytes but expected ${expected} bytes",
                    ("given",data.size())("expected", idx256_array_size) );
      return context.db_get_context().db_idx256_store(scope, table, account_name(payer), id, data.data());
   }
   void interface::db_idx256_update( int32_t iterator, uint64_t payer, legacy_span<const uint128_t> data ) {
      EOS_ASSERT( data.size() == idx256_array_size,
                    db_api_exception,
                    "invalid size of secondary key array for idx256: given ${given} bytes but expected ${expected} bytes",
                    ("given",data.size())("expected", idx256_array_size) );
      return context.db_get_context().db_idx256_update(iterator, account_name(payer), data.data());
   }
   void interface::db_idx256_remove( int32_t iterator ) {
      return context.db_get_context().db_idx256_remove(iterator);
   }
   int32_t interface::db_idx256_find_secondary( uint64_t code, uint64_t scope, uint64_t table, legacy_span<const uint128_t> data, legacy_ptr<uint64_t> primary ) {
      EOS_ASSERT( data.size() == idx256_array_size,
                    db_api_exception,
                    "invalid size of secondary key array for idx256: given ${given} bytes but expected ${expected} bytes",
                    ("given",data.size())("expected", idx256_array_size) );
      return context.db_get_context().db_idx256_find_secondary(code, scope, table, data.data(), *primary);
   }
   int32_t interface::db_idx256_find_primary( uint64_t code, uint64_t scope, uint64_t table, legacy_span<uint128_t> data, uint64_t primary ) {
      EOS_ASSERT( data.size() == idx256_array_size,
                    db_api_exception,
                    "invalid size of secondary key array for idx256: given ${given} bytes but expected ${expected} bytes",
                    ("given",data.size())("expected", idx256_array_size) );
      return context.db_get_context().db_idx256_find_primary(code, scope, table, data.data(), primary);
   }
   int32_t interface::db_idx256_lowerbound( uint64_t code, uint64_t scope, uint64_t table, legacy_span<uint128_t> data, legacy_ptr<uint64_t> primary ) {
      EOS_ASSERT( data.size() == idx256_array_size,
                    db_api_exception,
                    "invalid size of secondary key array for idx256: given ${given} bytes but expected ${expected} bytes",
                    ("given",data.size())("expected", idx256_array_size) );
      int32_t result = context.db_get_context().db_idx256_lowerbound(code, scope, table, data.data(), *primary);
      (void)legacy_span<uint128_t>(std::move(data));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx256_upperbound( uint64_t code, uint64_t scope, uint64_t table, legacy_span<uint128_t> data, legacy_ptr<uint64_t> primary ) {
      EOS_ASSERT( data.size() == idx256_array_size,
                    db_api_exception,
                    "invalid size of secondary key array for idx256: given ${given} bytes but expected ${expected} bytes",
                    ("given",data.size())("expected", idx256_array_size) );
      int32_t result = context.db_get_context().db_idx256_upperbound(code, scope, table, data.data(), *primary);
      (void)legacy_span<uint128_t>(std::move(data));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx256_end( uint64_t code, uint64_t scope, uint64_t table ) {
      return context.db_get_context().db_idx256_end(code, scope, table);
   }
   int32_t interface::db_idx256_next( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx256_next(iterator, *primary);
   }
   int32_t interface::db_idx256_previous( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx256_previous(iterator, *primary);
   }

   /**
    * interface for double secondary
    */
   int32_t interface::db_idx_double_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_ptr<const float64_t> secondary ) {
      return context.db_get_context().db_idx_double_store( scope, table, account_name(payer), id, *secondary );
   }
   void interface::db_idx_double_update( int32_t iterator, uint64_t payer, legacy_ptr<const float64_t> secondary ) {
      return context.db_get_context().db_idx_double_update( iterator, account_name(payer), *secondary );
   }
   void interface::db_idx_double_remove( int32_t iterator ) {
      return context.db_get_context().db_idx_double_remove( iterator );
   }
   int32_t interface::db_idx_double_find_secondary( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const float64_t> secondary, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx_double_find_secondary(code, scope, table, *secondary, *primary);
   }
   int32_t interface::db_idx_double_find_primary( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float64_t> secondary, uint64_t primary ) {
      return context.db_get_context().db_idx_double_find_primary(code, scope, table, *secondary, primary);
   }
   int32_t interface::db_idx_double_lowerbound( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float64_t> secondary, legacy_ptr<uint64_t> primary ) {
      int32_t result = context.db_get_context().db_idx_double_lowerbound(code, scope, table, *secondary, *primary);
      (void)legacy_ptr<float64_t>(std::move(secondary));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx_double_upperbound( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float64_t> secondary, legacy_ptr<uint64_t> primary ) {
      int32_t result = context.db_get_context().db_idx_double_upperbound(code, scope, table, *secondary, *primary);
      (void)legacy_ptr<float64_t>(std::move(secondary));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx_double_end( uint64_t code, uint64_t scope, uint64_t table ) {
      return context.db_get_context().db_idx_double_end(code, scope, table);
   }
   int32_t interface::db_idx_double_next( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx_double_next(iterator, *primary);
   }
   int32_t interface::db_idx_double_previous( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx_double_previous(iterator, *primary);
   }

   /**
    * interface for long double secondary
    */
   int32_t interface::db_idx_long_double_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_ptr<const float128_t> secondary ) {
      return context.db_get_context().db_idx_long_double_store( scope, table, account_name(payer), id, *secondary );
   }
   void interface::db_idx_long_double_update( int32_t iterator, uint64_t payer, legacy_ptr<const float128_t> secondary ) {
      return context.db_get_context().db_idx_long_double_update( iterator, account_name(payer), *secondary );
   }
   void interface::db_idx_long_double_remove( int32_t iterator ) {
      return context.db_get_context().db_idx_long_double_remove( iterator );
   }
   int32_t interface::db_idx_long_double_find_secondary( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<const float128_t> secondary, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx_long_double_find_secondary(code, scope, table, *secondary, *primary);
   }
   int32_t interface::db_idx_long_double_find_primary( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float128_t> secondary, uint64_t primary ) {
      return context.db_get_context().db_idx_long_double_find_primary(code, scope, table, *secondary, primary);
   }
   int32_t interface::db_idx_long_double_lowerbound( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float128_t> secondary, legacy_ptr<uint64_t> primary ) {
      int32_t result = context.db_get_context().db_idx_long_double_lowerbound(code, scope, table, *secondary, *primary);
      (void)legacy_ptr<float128_t>(std::move(secondary));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx_long_double_upperbound( uint64_t code, uint64_t scope, uint64_t table, legacy_ptr<float128_t> secondary, legacy_ptr<uint64_t> primary ) {
      int32_t result = context.db_get_context().db_idx_long_double_upperbound(code, scope, table, *secondary, *primary);
      (void)legacy_ptr<float128_t>(std::move(secondary));
      (void)legacy_ptr<uint64_t>(std::move(primary));
      return result;
   }
   int32_t interface::db_idx_long_double_end( uint64_t code, uint64_t scope, uint64_t table ) {
      return context.db_get_context().db_idx_long_double_end(code, scope, table);
   }
   int32_t interface::db_idx_long_double_next( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx_long_double_next(iterator, *primary);
   }
   int32_t interface::db_idx_long_double_previous( int32_t iterator, legacy_ptr<uint64_t> primary ) {
      return context.db_get_context().db_idx_long_double_previous(iterator, *primary);
   }
}}} // ns eosio::chain::webassembly
