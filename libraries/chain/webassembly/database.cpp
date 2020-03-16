#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   int32_t interface::db_store_i64( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, legacy_array_ptr<const char> data, uint32_t data_len ) {
   }
   void    interface::db_update_i64( int32_t itr, uint64_t payer, legacy_array_ptr<const char> data, uint32_t data_len ) {
   }
   void    interface::db_remove_i64( int32_t itr ) {
   }
   int32_t interface::db_get_i64( int32_t, legacy_array_ptr<char>, uint32_t ) {
   }
   int32_t interface::db_next_i64( int32_t, uint64_t& ) {
   }
   int32_t interface::db_previous_i64( int32_t, uint64_t& ) {
   }
   int32_t interface::db_find_i64( uint64_t, uint64_t, uint64_t, uint64_t ) {
   }
   int32_t interface::db_lowerbound_i64( uint64_t, uint64_t, uint64_t, uint64_t ) {
   }
   int32_t interface::db_upperbound_i64( uint64_t, uint64_t, uint64_t, uint64_t ) {
   }
   int32_t interface::db_end_i64( uint64_t, uint64_t, uint64_t ) {
   }

   int32_t interface::db_idx64_store( uint64_t, uint64_t, uint64_t, uint64_t, const uint64_t& ) {
   }
   void    interface::db_idx64_update( int32_t, uint64_t, const uint64_t& ) {
   }
   void    interface::db_idx64_remove( int32_t ) {
   }
   int32_t interface::db_idx64_find_secondary( uint64_t, uint64_t, uint64_t, const uint64_t&, uint64_t& ) {
   }
   int32_t interface::db_idx64_find_primary( uint64_t, uint64_t, uint64_t, uint64_t&, uint64_t& ) {
   }
   int32_t interface::db_idx64_lowerbound( uint64_t, uint64_t, uint64_t, uint64_t&, uint64_t& ) {
   }
   int32_t interface::db_idx64_upperbound( uint64_t, uint64_t, uint64_t, uint64_t&, uint64_t& ) {
   }
   int32_t interface::db_idx64_end( uint64_t, uint64_t, uint64_t ) {
   }
   int32_t interface::db_idx64_next( int32_t, uint64_t& ) {
   }
   int32_t interface::db_idx64_previous( int32_t, uint64_t& ) {
   }

   int32_t interface::db_idx128_store( uint64_t, uint64_t, uint64_t, uint64_t, const uint128_t& ) {
   }
   void    interface::db_idx128_update( int32_t, uint64_t, const uint128_t& ) {
   }
   void    interface::db_idx128_remove( int32_t ) {
   }
   int32_t interface::db_idx128_find_secondary( uint64_t, uint64_t, uint64_t, const uint128_t&, uint64_t& ) {
   }
   int32_t interface::db_idx128_find_primary( uint64_t, uint64_t, uint64_t, uint128_t&, uint64_t& ) {
   }
   int32_t interface::db_idx128_lowerbound( uint64_t, uint64_t, uint64_t, uint128_t&, uint64_t& ) {
   }
   int32_t interface::db_idx128_upperbound( uint64_t, uint64_t, uint64_t, uint128_t&, uint64_t& ) {
   }
   int32_t interface::db_idx128_end( uint64_t, uint64_t, uint64_t ) {
   }
   int32_t interface::db_idx128_next( int32_t, uint64_t& ) {
   }
   int32_t interface::db_idx128_previous( int32_t, uint64_t& ) {
   }

   int32_t interface::db_idx256_store( uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t ) {
   }
   void    interface::db_idx256_update( int32_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t ) {
   }
   void    interface::db_idx256_remove( int32_t ) {
   }
   int32_t interface::db_idx256_find_secondary( uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t, uint64_t& ) {
   }
   int32_t interface::db_idx256_find_primary( uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, uint64_t& ) {
   }
   int32_t interface::db_idx256_lowerbound( uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, uint64_t& ) {
   }
   int32_t interface::db_idx256_upperbound( uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, uint64_t& ) {
   }
   int32_t interface::db_idx256_end( uint64_t, uint64_t, uint64_t ) {
   }
   int32_t interface::db_idx256_next( int32_t, uint64_t& ) {
   }
   int32_t interface::db_idx256_previous( int32_t, uint64_t& ) {
   }

   int32_t interface::db_idx_double_store( uint64_t, uint64_t, uint64_t, uint64_t, const float64_t& ) {
   }
   void    interface::db_idx_double_update( int32_t, uint64_t, const float64_t& ) {
   }
   void    interface::db_idx_double_remove( int32_t ) {
   }
   int32_t interface::db_idx_double_find_secondary( uint64_t, uint64_t, uint64_t, const float64_t&, uint64_t& ) {
   }
   int32_t interface::db_idx_double_find_primary( uint64_t, uint64_t, uint64_t, float64_t&, uint64_t& ) {
   }
   int32_t interface::db_idx_double_lowerbound( uint64_t, uint64_t, uint64_t, float64_t&, uint64_t& ) {
   }
   int32_t interface::db_idx_double_upperbound( uint64_t, uint64_t, uint64_t, float64_t&, uint64_t& ) {
   }
   int32_t interface::db_idx_double_end( uint64_t, uint64_t, uint64_t ) {
   }
   int32_t interface::db_idx_double_next( int32_t, uint64_t& ) {
   }
   int32_t interface::db_idx_double_previous( int32_t, uint64_t& ) {
   }

   int32_t interface::db_idx_long_double_store( uint64_t, uint64_t, uint64_t, uint64_t, const float128_t& ) {
   }
   void    interface::db_idx_long_double_update( int32_t, uint64_t, const float128_t& ) {
   }
   void    interface::db_idx_long_double_remove( int32_t ) {
   }
   int32_t interface::db_idx_long_double_find_secondary( uint64_t, uint64_t, uint64_t, const float128_t&, uint64_t& ) {
   }
   int32_t interface::db_idx_long_double_find_primary( uint64_t, uint64_t, uint64_t, float128_t&, uint64_t& ) {
   }
   int32_t interface::db_idx_long_double_lowerbound( uint64_t, uint64_t, uint64_t, float128_t&, uint64_t& ) {
   }
   int32_t interface::db_idx_long_double_upperbound( uint64_t, uint64_t, uint64_t, float128_t&, uint64_t& ) {
   }
   int32_t interface::db_idx_long_double_end( uint64_t, uint64_t, uint64_t ) {
   }
   int32_t interface::db_idx_long_double_next( int32_t, uint64_t& ) {
   }
   int32_t interface::db_idx_long_double_previous( int32_t, uint64_t& ) {
   }
}}} // ns eosio::chain::webassembly
