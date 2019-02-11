bool is_nan( const float32_t f ) {
   return ((f.v & 0x7FFFFFFF) > 0x7F800000);
}

bool is_nan( const float64_t f ) {
   return ((f.v & 0x7FFFFFFFFFFFFFFF) > 0x7FF0000000000000);
}

bool is_nan( const float128_t& f ) {
   return (((~(f.v[1]) & uint64_t( 0x7FFF000000000000 )) == 0) && (f.v[0] || ((f.v[1]) & uint64_t( 0x0000FFFFFFFFFFFF ))));
}
extern "C" {
int32_t db_store_i64(uint64_t scope, uint64_t table, uint64_t payer, uint64_t id,  const char* data, uint32_t len) {
   return ctx().db_store_i64(scope, table, payer, id, data, len);
}

#if 0
int32_t db_store_i64_ex(uint64_t code, uint64_t scope, uint64_t table, uint64_t payer, uint64_t id,  const char* data, uint32_t len) {
   return ctx().db_store_i64(code, scope, table, payer, id, data, len);
}
#endif

void db_update_i64(int32_t iterator, uint64_t payer, const char* data, uint32_t len) {
   ctx().db_update_i64(iterator, payer, data, len);
}

void db_remove_i64(int32_t iterator) {
   ctx().db_remove_i64(iterator);
}

int32_t db_get_i64(int32_t iterator, void* data, uint32_t len) {
   return ctx().db_get_i64(iterator, (char*)data, len);
}
#if 0
int32_t db_get_i64_ex( int itr, uint64_t* primary, char* buffer, size_t buffer_size ) {
   return ctx().db_get_i64_ex( itr, *primary, buffer, buffer_size );
}

const char* db_get_i64_exex( int itr, size_t* buffer_size ) {
   return ctx().db_get_i64_exex( itr,  buffer_size);
}
#endif
int32_t db_next_i64(int32_t iterator, uint64_t* primary) {
   return ctx().db_next_i64(iterator, *primary);
}

int32_t db_previous_i64(int32_t iterator, uint64_t* primary) {
   return ctx().db_previous_i64(iterator, *primary);
}

int32_t db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
   return ctx().db_find_i64(code, scope, table, id);
}

int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
   return ctx().db_lowerbound_i64(code, scope, table, id);
}

int32_t db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
   return ctx().db_upperbound_i64(code, scope, table, id);
}

int32_t db_end_i64(uint64_t code, uint64_t scope, uint64_t table) {
   return ctx().db_end_i64(code, scope, table);
}

#define DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY_(IDX, TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE* secondary ) {\
         return ctx().IDX.store( scope, table, payer, id, *secondary );\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, const TYPE* secondary ) {\
         ctx().IDX.update( iterator, payer, *secondary );\
      }\
      void db_##IDX##_remove( int iterator ) {\
         ctx().IDX.remove( iterator );\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE* secondary, uint64_t* primary ) {\
         return ctx().IDX.find_secondary(code, scope, table, *secondary, *primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, TYPE* secondary, uint64_t primary ) {\
         return ctx().IDX.find_primary(code, scope, table, *secondary, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE* secondary, uint64_t* primary ) {\
         return ctx().IDX.lowerbound_secondary(code, scope, table, *secondary, *primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE* secondary, uint64_t* primary ) {\
         return ctx().IDX.upperbound_secondary(code, scope, table, *secondary, *primary);\
      }\
      int db_##IDX##_end( uint64_t code, uint64_t scope, uint64_t table ) {\
         return ctx().IDX.end_secondary(code, scope, table);\
      }\
      int db_##IDX##_next( int iterator, uint64_t* primary  ) {\
         return ctx().IDX.next_secondary(iterator, *primary);\
      }\
      int db_##IDX##_previous( int iterator, uint64_t* primary ) {\
         return ctx().IDX.previous_secondary(iterator, *primary);\
      }

#define DB_API_METHOD_WRAPPERS_ARRAY_SECONDARY_(IDX, ARR_SIZE, ARR_ELEMENT_TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const ARR_ELEMENT_TYPE* data, size_t data_len) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
              db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return ctx().IDX.store(scope, table, payer, id, (const ARR_ELEMENT_TYPE*)data);\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, const void* data, size_t data_len ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
              db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         ctx().IDX.update(iterator, payer, (const ARR_ELEMENT_TYPE*)data);\
      }\
      void db_##IDX##_remove( int iterator ) {\
         ctx().IDX.remove(iterator);\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const ARR_ELEMENT_TYPE* data, size_t data_len, uint64_t* primary ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
              db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return ctx().IDX.find_secondary(code, scope, table, (const ARR_ELEMENT_TYPE*)data, *primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, ARR_ELEMENT_TYPE* data, size_t data_len, uint64_t primary ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
              db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return ctx().IDX.find_primary(code, scope, table, (ARR_ELEMENT_TYPE*)data, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table, ARR_ELEMENT_TYPE* data, size_t data_len, uint64_t* primary ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
              db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return ctx().IDX.lowerbound_secondary(code, scope, table, (ARR_ELEMENT_TYPE*)data, *primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table, ARR_ELEMENT_TYPE* data, size_t data_len, uint64_t* primary ) {\
         EOS_ASSERT( data_len == ARR_SIZE,\
              db_api_exception,\
                    "invalid size of secondary key array for " #IDX ": given ${given} bytes but expected ${expected} bytes",\
                    ("given",data_len)("expected",ARR_SIZE) );\
         return ctx().IDX.upperbound_secondary(code, scope, table, (ARR_ELEMENT_TYPE*)data, *primary);\
      }\
      int db_##IDX##_end( uint64_t code, uint64_t scope, uint64_t table ) {\
         return ctx().IDX.end_secondary(code, scope, table);\
      }\
      int db_##IDX##_next( int iterator, uint64_t* primary  ) {\
         return ctx().IDX.next_secondary(iterator, *primary);\
      }\
      int db_##IDX##_previous( int iterator, uint64_t* primary ) {\
         return ctx().IDX.previous_secondary(iterator, *primary);\
      }

#define DB_API_METHOD_WRAPPERS_FLOAT_SECONDARY_(IDX, TYPE)\
      int db_##IDX##_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE* secondary ) {\
         EOS_ASSERT( !is_nan( *secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return ctx().IDX.store( scope, table, payer, id, *secondary );\
      }\
      void db_##IDX##_update( int iterator, uint64_t payer, const TYPE* secondary ) {\
         EOS_ASSERT( !is_nan( *secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         ctx().IDX.update( iterator, payer, *secondary );\
      }\
      void db_##IDX##_remove( int iterator ) {\
         ctx().IDX.remove( iterator );\
      }\
      int db_##IDX##_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE* secondary, uint64_t* primary ) {\
         EOS_ASSERT( !is_nan( *secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return ctx().IDX.find_secondary(code, scope, table, *secondary, *primary);\
      }\
      int db_##IDX##_find_primary( uint64_t code, uint64_t scope, uint64_t table, TYPE* secondary, uint64_t primary ) {\
         return ctx().IDX.find_primary(code, scope, table, *secondary, primary);\
      }\
      int db_##IDX##_lowerbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE* secondary, uint64_t* primary ) {\
         EOS_ASSERT( !is_nan( *secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return ctx().IDX.lowerbound_secondary(code, scope, table, *secondary, *primary);\
      }\
      int db_##IDX##_upperbound( uint64_t code, uint64_t scope, uint64_t table,  TYPE* secondary, uint64_t* primary ) {\
         EOS_ASSERT( !is_nan( *secondary ), transaction_exception, "NaN is not an allowed value for a secondary key" );\
         return ctx().IDX.upperbound_secondary(code, scope, table, *secondary, *primary);\
      }\
      int db_##IDX##_end( uint64_t code, uint64_t scope, uint64_t table ) {\
         return ctx().IDX.end_secondary(code, scope, table);\
      }\
      int db_##IDX##_next( int iterator, uint64_t* primary  ) {\
         return ctx().IDX.next_secondary(iterator, *primary);\
      }\
      int db_##IDX##_previous( int iterator, uint64_t* primary ) {\
         return ctx().IDX.previous_secondary(iterator, *primary);\
      }

DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY_(idx64,  uint64_t)
DB_API_METHOD_WRAPPERS_SIMPLE_SECONDARY_(idx128, uint128_t)
DB_API_METHOD_WRAPPERS_ARRAY_SECONDARY_(idx256, 2, uint128_t)
DB_API_METHOD_WRAPPERS_FLOAT_SECONDARY_(idx_double, float64_t)
DB_API_METHOD_WRAPPERS_FLOAT_SECONDARY_(idx_long_double, float128_t)
#if 0
int get_table_item_count(uint64_t code, uint64_t scope, uint64_t table) {
   return ctx().get_table_item_count(code, scope, table);
}
#endif
}

#if 0
int db_store_i256( uint64_t scope, uint64_t table, uint64_t payer, void* id, int size, const char* buffer, size_t buffer_size ) {
   eosio_assert(size <= sizeof(key256_t), "size of id must be <=32 bytes long!");
   key256_t key = {{0, 0}};
   memcpy(key.data(),id, size);
   return ctx().db_store_i256( scope, table, payer, key, buffer, buffer_size);
}

void db_update_i256( int iterator, uint64_t payer, const char* buffer, size_t buffer_size ) {
   return ctx().db_update_i256(iterator, payer, buffer, buffer_size, false);
}

void db_remove_i256( int iterator ) {
   return ctx().db_remove_i256(iterator);
}

int db_get_i256( int iterator, char* buffer, size_t buffer_size ) {
   return ctx().db_get_i256(iterator, buffer, buffer_size);
}

int db_find_i256( uint64_t code, uint64_t scope, uint64_t table, void* id, int size ) {
   eosio_assert(size <= 32, "size of id must be <=32 bytes long!");
   key256_t key = {{0, 0}};
   memcpy(key.data(),id, size);
   return ctx().db_find_i256(code, scope, table, key);
}

int db_upperbound_i256( uint64_t code, uint64_t scope, uint64_t table, char* id, int size ) {
   eosio_assert(size <= 32, "size of id must be <=32 bytes long!");
   key256_t key = {{0, 0}};
   memcpy(key.data(),id, size);
   return ctx().db_upperbound_i256(code, scope, table, key);
}

int db_lowerbound_i256( uint64_t code, uint64_t scope, uint64_t table, char* id, int size ) {
   eosio_assert(size <= 32, "size of id must be <=32 bytes long!");
   key256_t key = {{0, 0}};
   memcpy(key.data(),id, size);
   return ctx().db_lowerbound_i256(code, scope, table, key);
}
#endif
