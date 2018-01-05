#pragma once
#include <eoslib/db.hpp>
#include <eoslib/serialize.hpp>

namespace  eosio {

   /**
    *  This wrapper uses a single table to store named singleton objects of various sizes.
    */
   template<typename T, uint64_t Code, uint64_t SingletonName>
   class singleton
   {
      public:
         static const uint64_t singleton_table_name = N(singleton);

         static bool exists( scope_name scope ) {
            uint64_t key = SingletonName;
            auto read = load_i64( scope, Code, singleton_table_name, (char*)&key, sizeof(key) );
            return read > 0;
         }

         static T get( scope_name scope ) {
            char temp[1024+8];
            auto read = load_i64( scope, Code, singleton_table_name, temp, sizeof(temp) );
            assert( read > 0, "singleton does not exist" );
            datastream<const char*> ds(temp + sizeof(SingletonName), read);

            T result;
            unpack( ds, result );
            return result;
         }

         static T get_or_create( scope_name scope, const T& def = T() ) {
            char temp[1024+8];

            auto read = load_i64( scope, Code, singleton_table_name, temp, sizeof(temp) );
            if( read < 0 ) {
               set( scope, def );
               return def;
            }

            datastream<const char*> ds(temp + sizeof(SingletonName), read);

            T result;
            unpack( ds, result );
            return result;
         }

         static void set( scope_name scope, const T& value ) {
            auto size = pack_size( value );
            char buf[size+ sizeof(SingletonName)];

            assert( sizeof(buf) <= 1024 + 8, "singleton too big to store" );

            datastream<char*> ds( buf, size + sizeof(SingletonName) );
            pack( ds, SingletonName );
            pack( ds, value );
            
            store_i64( scope, singleton_table_name, buf, sizeof(buf) );
         }
   };

} /// namespace eosio
