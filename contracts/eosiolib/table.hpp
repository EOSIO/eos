#pragma once
#include <eosiolib/system.h>
#include <eosiolib/db.h>
#include <eosiolib/datastream.hpp>

namespace eosio {

   /**
    *  @tparam DefaultScope - the default 
    *  @tparam TableName - the name of the table with rows of type T
    *  @tparam T - a struct where the first 8 bytes are used as primary/unique key
    */
   template<uint64_t DefaultScope, uint64_t TableName, uint64_t BillToAccount, typename T>
   class table64
   {
      public:
         static bool exists( uint64_t key, scope_name scope = DefaultScope) {
            auto read = load_i64( DefaultScope, scope, TableName, (char*)&key, sizeof(key) );
            return read > 0;
         }

         static T get( uint64_t key, scope_name scope = DefaultScope ) {
            char temp[1024];
            *reinterpret_cast<uint64_t *>(temp) = key;
            auto read = load_i64( DefaultScope, scope , TableName, temp, sizeof(temp) );
            eosio_assert( read > 0, "key does not exist" );

            datastream<const char*> ds(temp, uint32_t(read) );
            T result;
            ds >> result;
            return result;
         }

         static T get_or_create( uint64_t key, scope_name scope = DefaultScope, const T& def = T() ) {
            char temp[1024];
            *reinterpret_cast<uint64_t *>(temp) = key;

            auto read = load_i64( DefaultScope, scope, TableName, temp, sizeof(temp) );
            if( read < 0 ) {
               set( def, scope );
               return def;
            }

            datastream<const char*> ds(temp, uint32_t(read) );
            T result;
            ds >> result;
            return result;
         }

         static T get_or_default( uint64_t key, scope_name scope = DefaultScope, const T& def = T() ) {
            char temp[1024];
            *reinterpret_cast<uint64_t *>(temp) = key;

            auto read = load_i64( scope, DefaultScope, TableName, temp, sizeof(temp) );
            if( read < 0 ) {
               return def;
            }

            datastream<const char*> ds(temp, uint32_t(read) );
            T result;
            ds >> result;
            return result;
         }

         static void set( const T& value = T(), scope_name scope = DefaultScope, uint64_t bta = BillToAccount ) {
            auto size = pack_size( value );
            char buf[size];
            eosio_assert( size <= 1024, "singleton too big to store" );

            datastream<char*> ds( buf, size );
            ds << value;
            
            store_i64( scope, TableName, bta, buf, ds.tellp() );
         }

         static void remove( uint64_t key, scope_name scope = DefaultScope ) {
            remove_i64(scope, TableName, &key);
         }
   };


   template<uint64_t Code, uint64_t TableName, uint64_t BillToAccount, typename T>
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
            
            auto read = lower_bound_primary_i64i64i64(    Code, _scope, TableName,
                                                (char*)temp, sizeof(temp) );
            if( read <= 0 ) {
               return false;
            }

            datastream<const char*> ds( (char*)temp, sizeof(temp) );
            ds >> result;
            return true;
         }

         bool primary_upper_bound( T& result,
                                   uint64_t primary = 0,
                                   uint64_t secondary = 0,
                                   uint64_t tertiary = 0 ) {

            uint64_t temp[1024/8];
            temp[0] = primary;
            temp[1] = secondary;
            temp[2] = tertiary;

            auto read = upper_bound_primary_i64i64i64(    Code, _scope, TableName,
                                                (char*)temp, sizeof(temp) );
            if( read <= 0 ) {
               return false;
            }

            result = unpack<T>( (char*)temp, sizeof(temp) );
            return true;
         }

         bool next_primary( T& result, const T& current ) {
            const uint64_t* keys = reinterpret_cast<const uint64_t*>(&current);
            return primary_upper_bound(result, keys[0], keys[1], keys[2]);
         }

         void store( const T& value, account_name bill_to = BillToAccount ) {
            char temp[1024];
            datastream<char*> ds(temp, sizeof(temp) );
            ds << value;

            store_i64i64i64( _scope, TableName, bill_to, temp, ds.tellp() );
         }

         void remove(uint64_t primary_key, uint64_t seconday_key, uint64_t tertiary_key) {
            uint64_t temp[3] = { primary_key, seconday_key, tertiary_key };
            remove_i64i64i64(_scope, TableName, temp);
         }

      private:
         uint64_t _scope;
   };

}
