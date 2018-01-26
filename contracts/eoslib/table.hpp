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
            
            store_i64( scope, TableName, buf, sizeof(buf) );
         }
   };

}
