#pragma once
#include <eosiolib/multi_index.hpp>
#include <eosiolib/system.h>

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
      constexpr static uint64_t pk_value = SingletonName;
      struct row {
         T value;

         uint64_t primary_key() const { return pk_value; }

         EOSLIB_SERIALIZE( row, (value) );
      };

      typedef eosio::multi_index<SingletonName, row> table;

      public:
         //static const uint64_t singleton_table_name = N(singleton);

         static bool exists( scope_name scope = Code ) {
            table t( Code, scope );
            return t.find( pk_value );
         }

         static T get( scope_name scope = Code ) {
            table t( Code, scope );
            auto ptr = t.find( pk_value );
            eosio_assert( bool(ptr), "singleton does not exist" );
            return ptr->value;
         }

         static T get_or_default( scope_name scope = Code, const T& def = T() ) {
            table t( Code, scope );
            auto ptr = t.find( pk_value );
            return ptr ? ptr->value : def;
         }

         static T get_or_create( scope_name scope = Code, const T& def = T() ) {
            table t( Code, scope );
            auto ptr = t.find( pk_value );
            return ptr ? ptr->value
               : t.emplace(BillToAccount, [&](row& r) { r.value = def; });
         }

         static void set( const T& value = T(), scope_name scope = Code, account_name b = BillToAccount ) {
            table t( Code, scope );
            auto ptr = t.find( pk_value );
            if (ptr) {
               t.update(*ptr, b, [&](row& r) { r.value = value; });
            } else {
               t.emplace(b, [&](row& r) { r.value = value; });
            }
         }

         static void remove( scope_name scope = Code ) {
            table t( Code, scope );
            auto ptr = t.find( pk_value );
            if (ptr) {
               t.remove(*ptr);
            }
         }
   };

} /// namespace eosio
