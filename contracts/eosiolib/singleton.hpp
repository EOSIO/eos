#pragma once
#include <eosiolib/db.hpp>
#include <eosiolib/raw.hpp>

namespace  eosio {

   /**
    *  This wrapper uses a single table to store named objects various types. 
    *
    *  @tparam Code - the name of the code which has write permission
    *  @tparam SingletonName - the name of this singlton variable
    *  @tparam T - the type of the singleton 
    */
   template<account_name Code, uint64_t SingletonName, typename T>
   class singleton
   {
      public:
         //static const uint64_t singleton_table_name = N(singleton);

         static bool exists( scope_name scope = Code ) {
            uint64_t key = SingletonName;
            auto read = load_i64( scope, Code, key, (char*)&key, sizeof(key) );
            return read > 0;
         }

         static T get( scope_name scope = Code ) {
            char temp[1024+8];
            *reinterpret_cast<uint64_t *>(temp) = SingletonName;
            auto read = load_i64( scope, Code, SingletonName, temp, sizeof(temp) );
            assert( read > 0, "singleton does not exist" );
            datastream<const char*> ds(temp + sizeof(SingletonName), read);

            T result;
            raw::unpack( ds, result );
            return result;
         }

         static T get_or_create( scope_name scope = Code, const T& def = T() ) {
            char temp[1024+8];
            *reinterpret_cast<uint64_t *>(temp) = SingletonName;


            auto read = load_i64( scope, Code, SingletonName, temp, sizeof(temp) );
            if( read < 0 ) {
               set( def, scope );
               return def;
            }

            datastream<const char*> ds(temp + sizeof(SingletonName), read);
            T result;
            ds >> result;
            return result;
         }

         static void set( const T& value = T(), scope_name scope = Code ) {
            auto size = raw::pack_size( value );
            char buf[size+ sizeof(SingletonName)];

            assert( sizeof(buf) <= 1024 + 8, "singleton too big to store" );

            datastream<char*> ds( buf, size + sizeof(SingletonName) );
            ds << SingletonName;
            ds << value;
            
            store_i64( scope, SingletonName, buf, sizeof(buf) );
         }
   };

} /// namespace eosio
