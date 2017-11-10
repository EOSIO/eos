/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   struct by_scope_primary;
   struct by_scope_secondary;
   struct by_scope_tertiary;

   struct key_value_object : public chainbase::object<key_value_object_type, key_value_object> {
      OBJECT_CTOR(key_value_object, (value))
      
      typedef uint64_t key_type;
      static const int number_of_keys = 1;

      id_type               id;
      account_name           scope; 
      account_name           code;
      account_name           table;
      uint64_t              primary_key;
      shared_string         value;
   };

   using key_value_index = chainbase::shared_multi_index_container<
      key_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<key_value_object, key_value_object::id_type, &key_value_object::id>>,
         ordered_unique<tag<by_scope_primary>, 
            composite_key< key_value_object,
               member<key_value_object, account_name, &key_value_object::scope>,
               member<key_value_object, account_name, &key_value_object::code>,
               member<key_value_object, account_name, &key_value_object::table>,
               member<key_value_object, uint64_t, &key_value_object::primary_key>
            >,
            composite_key_compare< std::less<account_name>,std::less<account_name>,std::less<account_name>,std::less<uint64_t> >
         >
      >
   >;

   struct shared_string_less {
      bool operator()( const char* a, const char* b )const {
         return less(a, b);
      }

      bool operator()( const std::string& a, const char* b )const {
         return less(a.c_str(), b);
      }

      bool operator()( const char* a, const std::string& b )const {
         return less(a, b.c_str());
      }

      inline bool less( const char* a, const char* b )const{
         return std::strcmp( a, b ) < 0;
      }
   };

   struct keystr_value_object : public chainbase::object<keystr_value_object_type, keystr_value_object> {
      OBJECT_CTOR(keystr_value_object, (primary_key)(value))
      
      typedef std::string key_type;
      static const int number_of_keys = 1;

      const char* data() const { return primary_key.data(); }

      id_type               id;
      account_name           scope; 
      account_name           code;
      account_name           table;
      shared_string         primary_key;
      shared_string         value;
   };

   using keystr_value_index = chainbase::shared_multi_index_container<
      keystr_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<keystr_value_object, keystr_value_object::id_type, &keystr_value_object::id>>,
         ordered_unique<tag<by_scope_primary>, 
            composite_key< keystr_value_object,
               member<keystr_value_object, account_name, &keystr_value_object::scope>,
               member<keystr_value_object, account_name, &keystr_value_object::code>,
               member<keystr_value_object, account_name, &keystr_value_object::table>,
               const_mem_fun<keystr_value_object, const char*, &keystr_value_object::data>
            >,
            composite_key_compare< std::less<account_name>,std::less<account_name>,std::less<account_name>,shared_string_less>
         >
      >
   >;

   struct key128x128_value_object : public chainbase::object<key128x128_value_object_type, key128x128_value_object> {
      OBJECT_CTOR(key128x128_value_object, (value))

      typedef uint128_t key_type;
      static const int number_of_keys = 2;

      id_type               id;
      account_name           scope; 
      account_name           code;
      account_name           table;
      uint128_t             primary_key;
      uint128_t             secondary_key;
      shared_string         value;
   };

   using key128x128_value_index = chainbase::shared_multi_index_container<
      key128x128_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<key128x128_value_object, key128x128_value_object::id_type, &key128x128_value_object::id>>,
         ordered_unique<tag<by_scope_primary>, 
            composite_key< key128x128_value_object,
               member<key128x128_value_object, account_name, &key128x128_value_object::scope>,
               member<key128x128_value_object, account_name, &key128x128_value_object::code>,
               member<key128x128_value_object, account_name, &key128x128_value_object::table>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::primary_key>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::secondary_key>
            >,
            composite_key_compare< std::less<account_name>,std::less<account_name>,std::less<account_name>,std::less<uint128_t>,std::less<uint128_t> >
         >,
         ordered_unique<tag<by_scope_secondary>, 
            composite_key< key128x128_value_object,
               member<key128x128_value_object, account_name, &key128x128_value_object::scope>,
               member<key128x128_value_object, account_name, &key128x128_value_object::code>,
               member<key128x128_value_object, account_name, &key128x128_value_object::table>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::secondary_key>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::primary_key>
            >,
            composite_key_compare< std::less<account_name>,std::less<account_name>,std::less<account_name>,std::less<uint128_t>,std::less<uint128_t> >
         >
      >
   >;

   struct key64x64x64_value_object : public chainbase::object<key64x64x64_value_object_type, key64x64x64_value_object> {
      OBJECT_CTOR(key64x64x64_value_object, (value))

      typedef uint64_t key_type;
      static const int number_of_keys = 3;

      id_type               id;
      account_name           scope;
      account_name           code;
      account_name           table;
      uint64_t              primary_key;
      uint64_t              secondary_key;
      uint64_t              tertiary_key;
      shared_string         value;
   };

   using key64x64x64_value_index = chainbase::shared_multi_index_container<
      key64x64x64_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<key64x64x64_value_object, key64x64x64_value_object::id_type, &key64x64x64_value_object::id>>,
         ordered_unique<tag<by_scope_primary>,
            composite_key< key64x64x64_value_object,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::scope>,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::code>,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::table>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::primary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::secondary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>
            >,
            composite_key_compare< std::less<account_name>,std::less<account_name>,std::less<account_name>,std::less<uint64_t>,std::less<uint64_t>,std::less<uint64_t> >
         >,
         ordered_unique<tag<by_scope_secondary>,
            composite_key< key64x64x64_value_object,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::scope>,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::code>,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::table>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::secondary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>,
               member<key64x64x64_value_object, typename key64x64x64_value_object::id_type, &key64x64x64_value_object::id>
            >
         >,
         ordered_unique<tag<by_scope_tertiary>,
            composite_key< key64x64x64_value_object,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::scope>,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::code>,
               member<key64x64x64_value_object, account_name, &key64x64x64_value_object::table>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>,
               member<key64x64x64_value_object, typename key64x64x64_value_object::id_type, &key64x64x64_value_object::id>
            >,
            composite_key_compare< std::less<account_name>,std::less<account_name>,std::less<account_name>,std::less<uint64_t>,std::less<typename key64x64x64_value_object::id_type> >
         >         
      >
   >;




} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::key_value_object, eosio::chain::key_value_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::keystr_value_object, eosio::chain::keystr_value_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::key128x128_value_object, eosio::chain::key128x128_value_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::key64x64x64_value_object, eosio::chain::key64x64x64_value_index)

FC_REFLECT(eosio::chain::key_value_object, (id)(scope)(code)(table)(primary_key)(value) )
