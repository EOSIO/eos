/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/multi_index_includes.hpp>

#include <chainbase/chainbase.hpp>

namespace eosio { namespace chain { namespace contracts {

   /**
    * @brief The table_id_object class tracks the mapping of (scope, code, table) to an opaque identifier
    */
   class table_id_object : public chainbase::object<table_id_object_type, table_id_object> {
      OBJECT_CTOR(table_id_object)

      id_type        id;
      scope_name     scope;
      account_name   code;
      table_name     table;
   };

   struct by_scope_code_table;

   using table_id_multi_index = chainbase::shared_multi_index_container<
      table_id_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            member<table_id_object, table_id_object::id_type, &table_id_object::id>
         >,
         ordered_unique<tag<by_scope_code_table>,
            composite_key< table_id_object,
               member<table_id_object, scope_name,   &table_id_object::scope>,
               member<table_id_object, account_name, &table_id_object::code>,
               member<table_id_object, table_name,   &table_id_object::table>
            >
         >
      >
   >;

   using table_id = table_id_object::id_type;

   struct by_scope_primary;
   struct by_scope_secondary;
   struct by_scope_tertiary;

   struct key_value_object : public chainbase::object<key_value_object_type, key_value_object> {
      OBJECT_CTOR(key_value_object, (value))

      typedef uint64_t key_type;
      static const int number_of_keys = 1;

      id_type               id;
      table_id              t_id;
      uint64_t              primary_key;
      shared_string         value;
   };

   using key_value_index = chainbase::shared_multi_index_container<
      key_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<key_value_object, key_value_object::id_type, &key_value_object::id>>,
         ordered_unique<tag<by_scope_primary>,
            composite_key< key_value_object,
               member<key_value_object, table_id, &key_value_object::t_id>,
               member<key_value_object, uint64_t, &key_value_object::primary_key>
            >,
            composite_key_compare< std::less<table_id>, std::less<uint64_t> >
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
      table_id              t_id;
      shared_string         primary_key;
      shared_string         value;
   };

   using keystr_value_index = chainbase::shared_multi_index_container<
      keystr_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<keystr_value_object, keystr_value_object::id_type, &keystr_value_object::id>>,
         ordered_unique<tag<by_scope_primary>,
            composite_key< keystr_value_object,
               member<keystr_value_object, table_id, &keystr_value_object::t_id>,
               const_mem_fun<keystr_value_object, const char*, &keystr_value_object::data>
            >,
            composite_key_compare< std::less<table_id>, shared_string_less>
         >
      >
   >;

   struct key128x128_value_object : public chainbase::object<key128x128_value_object_type, key128x128_value_object> {
      OBJECT_CTOR(key128x128_value_object, (value))

      typedef uint128_t key_type;
      static const int number_of_keys = 2;

      id_type               id;
      table_id              t_id;
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
               member<key128x128_value_object, table_id, &key128x128_value_object::t_id>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::primary_key>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::secondary_key>
            >,
            composite_key_compare< std::less<table_id>,std::less<uint128_t>,std::less<uint128_t> >
         >,
         ordered_unique<tag<by_scope_secondary>,
            composite_key< key128x128_value_object,
               member<key128x128_value_object, table_id, &key128x128_value_object::t_id>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::secondary_key>,
               member<key128x128_value_object, typename key128x128_value_object::id_type, &key128x128_value_object::id>
            >,
            composite_key_compare< std::less<table_id>,std::less<uint128_t>,std::less<typename key128x128_value_object::id_type> >
         >
      >
   >;

   struct key64x64x64_value_object : public chainbase::object<key64x64x64_value_object_type, key64x64x64_value_object> {
      OBJECT_CTOR(key64x64x64_value_object, (value))

      typedef uint64_t key_type;
      static const int number_of_keys = 3;

      id_type               id;
      table_id              t_id;
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
               member<key64x64x64_value_object, table_id, &key64x64x64_value_object::t_id>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::primary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::secondary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>
            >,
            composite_key_compare< std::less<table_id>,std::less<uint64_t>,std::less<uint64_t>,std::less<uint64_t> >
         >,
         ordered_unique<tag<by_scope_secondary>,
            composite_key< key64x64x64_value_object,
               member<key64x64x64_value_object, table_id, &key64x64x64_value_object::t_id>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::secondary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>,
               member<key64x64x64_value_object, typename key64x64x64_value_object::id_type, &key64x64x64_value_object::id>
            >
         >,
         ordered_unique<tag<by_scope_tertiary>,
            composite_key< key64x64x64_value_object,
               member<key64x64x64_value_object, table_id, &key64x64x64_value_object::t_id>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>,
               member<key64x64x64_value_object, typename key64x64x64_value_object::id_type, &key64x64x64_value_object::id>
            >,
            composite_key_compare< std::less<table_id>,std::less<uint64_t>,std::less<typename key64x64x64_value_object::id_type> >
         >
      >
   >;


} } }  // namespace eosio::chain::contracts

CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::table_id_object, eosio::chain::contracts::table_id_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::key_value_object, eosio::chain::contracts::key_value_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::keystr_value_object, eosio::chain::contracts::keystr_value_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::key128x128_value_object, eosio::chain::contracts::key128x128_value_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::key64x64x64_value_object, eosio::chain::contracts::key64x64x64_value_index)

FC_REFLECT(eosio::chain::contracts::table_id_object, (id)(scope)(code)(table) )
FC_REFLECT(eosio::chain::contracts::key_value_object, (id)(t_id)(primary_key)(value) )
FC_REFLECT(eosio::chain::contracts::keystr_value_object, (id)(t_id)(primary_key)(value) )
FC_REFLECT(eosio::chain::contracts::key128x128_value_object, (id)(t_id)(primary_key)(secondary_key)(value) )
FC_REFLECT(eosio::chain::contracts::key64x64x64_value_object, (id)(t_id)(primary_key)(secondary_key)(tertiary_key)(value) )