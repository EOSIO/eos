#pragma once
#include <eosiolib/db.hpp>
#include <eosiolib/datastream.hpp>

namespace  eosio {

   /**
    *  This wrapper uses a single table to store named objects various types. 
    *
    *  @tparam Code - the name of the code which has write permission
    *  @tparam SingletonName - the name of this singlton variable
    *  @tparam T - the type of the singleton 
    */
   template<account_name Code, uint64_t SingletonName, account_name BillToAccount, typename T>
   class singleton
   {
      public:
         //static const uint64_t singleton_table_name = N(singleton);

         static bool exists( scope_name scope = Code ) {
            uint64_t key = SingletonName;
            auto read = load_i64( Code, scope, key, (char*)&key, sizeof(key) );
            return read > 0;
         }

         static T get( scope_name scope = Code ) {
            char temp[1024+8];
            *reinterpret_cast<uint64_t *>(temp) = SingletonName;
            auto read = load_i64( Code, scope, SingletonName, temp, sizeof(temp) );
            eosio_assert( read > 0, "singleton does not exist" );
            return unpack<T>( temp + sizeof(SingletonName), read );
         }

         static T get_or_default( scope_name scope = Code, const T& def = T() ) {
            char temp[1024+8];
            *reinterpret_cast<uint64_t *>(temp) = SingletonName;
            auto read = load_i64( Code, scope, SingletonName, temp, sizeof(temp) );
            if ( read < 0 ) {
               return def;
            }
            return unpack<T>( temp + sizeof(SingletonName), size_t(read) );
         }

         static T get_or_create( scope_name scope = Code, const T& def = T() ) {
            char temp[1024+8];
            *reinterpret_cast<uint64_t *>(temp) = SingletonName;


            auto read = load_i64( Code, scope, SingletonName, temp, sizeof(temp) );
            if( read < 0 ) {
               set( def, scope );
               return def;
            }
            return unpack<T>( temp + sizeof(SingletonName), read );
         }

         static void set( const T& value = T(), scope_name scope = Code, account_name b = BillToAccount ) {
            auto size = pack_size( value );
            char buf[size+ sizeof(SingletonName)];

            eosio_assert( sizeof(buf) <= 1024 + 8, "singleton too big to store" );

            datastream<char*> ds( buf, size + sizeof(SingletonName) );
            ds << SingletonName;
            ds << value;
            
            store_i64( scope, SingletonName, b, buf, sizeof(buf) );
         }

         static void remove( scope_name scope = Code ) {
            uint64_t key = SingletonName;
            remove_i64( scope, SingletonName, &key );
         }
   };

} /// namespace eosio
