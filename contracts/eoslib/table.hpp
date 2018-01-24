#pragma once

namespace eosio {

   /**
    *  @tparam DefaultScope - the default 
    *  @tparam TableName - the name of the table with rows of type T
    *  @tparam T - a struct where the first 8 bytes are used as primary/unique key
    */
   template<uint64_t DefaultScope, uint64_t TableName, typename T>
   class table64
   {
      public:
         static bool exists( uint64_t key, scope_name scope = DefaultScope) {
            auto read = load_i64( scope, DefaultScope, TableName, (char*)&key, sizeof(key) );
            return read > 0;
         }

         static T get( uint64_t key, scope_name scope = DefaultScope ) {
            char temp[1024];
            *reinterpret_cast<uint64_t *>(temp) = key;
            auto read = load_i64( scope, DefaultScope , TableName, temp, sizeof(temp) );
            assert( read > 0, "key does not exist" );

            datastream<const char*> ds(temp, read);
            T result;
            ds >> result;
            return result;
         }

         static T get_or_create( uint64_t key, scope_name scope = DefaultScope, const T& def = T() ) {
            char temp[1024];
            *reinterpret_cast<uint64_t *>(temp) = key;

            auto read = load_i64( scope, DefaultScope, TableName, temp, sizeof(temp) );
            if( read < 0 ) {
               set( def, scope );
               return def;
            }

            datastream<const char*> ds(temp, read);
            T result;
            ds >> result;
            return result;
         }

         static void set( const T& value = T(), scope_name scope = DefaultScope ) {
            auto size = raw::pack_size( value );
            char buf[size];
            assert( size <= 1024, "singleton too big to store" );

            datastream<char*> ds( buf, size );
            ds << value;
            
            store_i64( scope, TableName, buf, ds.tellp() );
         }
   };


   template<uint64_t Code, uint64_t TableName, typename T>
   class table_i64i64i64 {
      public:
         table_i64i64i64( uint64_t scope = Code  )
         :_scope(scope){}

         bool primary_lower_bound( T& result,
                                   uint64_t primary = 0, 
                                   uint64_t secondary = 0, 
                                   uint64_t tertiary = 0 ) {

            uint64_t temp[1024/8];
            temp[0] = primary;
            temp[1] = secondary;
            temp[2] = tertiary;
            
            auto read = lower_bound_primary_i64i64i64( Code, _scope, TableName, 
                                                (char*)temp, sizeof(temp) );
            if( read <= 0 ) {
               return false;
            }

            datastream<const char*> ds( (char*)temp, sizeof(temp) );
            ds >> result;
         }

         bool next_primary( T& result, const T& current ) {
            uint64_t temp[1024/8];
            memcpy( temp, (const char*)&current, 3*sizeof(uint64_t) );
            
            auto read = next_primary_i64i64i64( Code, _scope, TableName, 
                                                (char*)temp, sizeof(temp) );
            if( read <= 0 ) {
               return false;
            }

            datastream<const char*> ds( (char*)temp, sizeof(temp) );
            ds >> result;
         }

         void store( const T& value, account_name bill_to ) {
            char temp[1024];
            datastream<char*> ds(temp, sizeof(temp) );
            ds << value;

            store_i64i64i64( _scope, TableName, temp, ds.tellp() );
         }
      private:
         uint64_t _scope;
   };

}
