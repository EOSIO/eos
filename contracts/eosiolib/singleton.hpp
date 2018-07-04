#pragma once
#include <eosiolib/multi_index.hpp>
#include <eosiolib/system.h>

namespace  eosio {

   /**
    *  This wrapper uses a single table to store named objects various types.
    *
    *  @tparam SingletonName - the name of this singlton variable
    *  @tparam T - the type of the singleton
    */
   template<uint64_t SingletonName, typename T>
   class singleton
   {
      constexpr static uint64_t pk_value = SingletonName;
      struct row {
         T value;

         uint64_t primary_key() const { return pk_value; }

         EOSLIB_SERIALIZE( row, (value) )
      };

      typedef eosio::multi_index<SingletonName, row> table;

      public:

         singleton( account_name code, scope_name scope ) : _t( code, scope ) {}

         bool exists() {
            return _t.find( pk_value ) != _t.end();
         }

         T get() {
            auto itr = _t.find( pk_value );
            eosio_assert( itr != _t.end(), "singleton does not exist" );
            return itr->value;
         }

         T get_or_default( const T& def = T() ) {
            auto itr = _t.find( pk_value );
            return itr != _t.end() ? itr->value : def;
         }

         T get_or_create( account_name bill_to_account, const T& def = T() ) {
            auto itr = _t.find( pk_value );
            return itr != _t.end() ? itr->value
               : _t.emplace(bill_to_account, [&](row& r) { r.value = def; })->value;
         }

         void set( const T& value, account_name bill_to_account ) {
            auto itr = _t.find( pk_value );
            if( itr != _t.end() ) {
               _t.modify(itr, bill_to_account, [&](row& r) { r.value = value; });
            } else {
               _t.emplace(bill_to_account, [&](row& r) { r.value = value; });
            }
         }

         void remove( ) {
            auto itr = _t.find( pk_value );
            if( itr != _t.end() ) {
               _t.erase(itr);
            }
         }

      private:
         table _t;
   };

} /// namespace eosio
