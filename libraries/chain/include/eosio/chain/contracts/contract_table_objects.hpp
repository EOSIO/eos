/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/multi_index_includes.hpp>
#include <softfloat.hpp>

#include <chainbase/chainbase.hpp>

#include <array>
#include <type_traits>

namespace eosio { namespace chain { namespace contracts {

   /**
    * @brief The table_id_object class tracks the mapping of (scope, code, table) to an opaque identifier
    */
   class table_id_object : public chainbase::object<table_id_object_type, table_id_object> {
      OBJECT_CTOR(table_id_object)

      id_type        id;
      account_name   code;
      scope_name     scope;
      table_name     table;
      uint32_t       count = 0; /// the number of elements in the table
   };

   struct by_code_scope_table;

   using table_id_multi_index = chainbase::shared_multi_index_container<
      table_id_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            member<table_id_object, table_id_object::id_type, &table_id_object::id>
         >,
         ordered_unique<tag<by_code_scope_table>,
            composite_key< table_id_object,
               member<table_id_object, account_name, &table_id_object::code>,
               member<table_id_object, scope_name,   &table_id_object::scope>,
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
      account_name          payer = 0;
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

   struct by_primary;
   struct by_secondary;

   template<typename SecondaryKey, uint64_t ObjectTypeId, typename SecondaryKeyLess = std::less<SecondaryKey> >
   struct secondary_index
   {
      struct index_object : public chainbase::object<ObjectTypeId,index_object> {
         OBJECT_CTOR(index_object)
         typedef SecondaryKey secondary_key_type;

         typename chainbase::object<ObjectTypeId,index_object>::id_type       id;
         table_id      t_id;
         uint64_t      primary_key;
         SecondaryKey  secondary_key;
         account_name  payer;
      };


      typedef chainbase::shared_multi_index_container<
         index_object,
         indexed_by<
            ordered_unique<tag<by_id>, member<index_object, typename index_object::id_type, &index_object::id>>,
            ordered_unique<tag<by_primary>,
               composite_key< index_object,
                  member<index_object, table_id, &index_object::t_id>,
                  member<index_object, uint64_t, &index_object::primary_key>
               >,
               composite_key_compare< std::less<table_id>, std::less<uint64_t> >
            >,
            ordered_unique<tag<by_secondary>,
               composite_key< index_object,
                  member<index_object, table_id, &index_object::t_id>,
                  member<index_object, SecondaryKey, &index_object::secondary_key>,
                  member<index_object, uint64_t, &index_object::primary_key>
               >,
               composite_key_compare< std::less<table_id>, SecondaryKeyLess, std::less<uint64_t> >
            >
         >
      > index_index;
   };

   template<typename SecondaryKey>
   inline
   //typename std::enable_if<!_is_array<SecondaryKey>::value, size_t>::type
   size_t
   get_key_memory_usage() {
      return sizeof(SecondaryKey);
   }

   typedef secondary_index<uint64_t,index64_object_type>::index_object   index64_object;
   typedef secondary_index<uint64_t,index64_object_type>::index_index    index64_index;

   typedef secondary_index<uint128_t,index128_object_type>::index_object index128_object;
   typedef secondary_index<uint128_t,index128_object_type>::index_index  index128_index;

   typedef std::array<uint128_t, 2> key256_t;
   typedef secondary_index<key256_t,index256_object_type>::index_object index256_object;
   typedef secondary_index<key256_t,index256_object_type>::index_index  index256_index;

   struct soft_double_less {
      bool operator()( uint64_t a, uint64_t b )const {
         float64_t x; x.v = a;
         float64_t y; y.v = b;
         return f64_lt(x, y);
      }
   };

   /**
    *  This index supports a deterministic software implementation of double as the secondary key.
    *
    *  The software double implementation is using the Berkeley softfloat library (release 3).
    */
   typedef secondary_index<uint64_t,index_double_object_type,soft_double_less>::index_object  index_double_object;
   typedef secondary_index<uint64_t,index_double_object_type,soft_double_less>::index_index   index_double_index;

} } }  // namespace eosio::chain::contracts

CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::table_id_object, eosio::chain::contracts::table_id_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::key_value_object, eosio::chain::contracts::key_value_index)

CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::index64_object, eosio::chain::contracts::index64_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::index128_object, eosio::chain::contracts::index128_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::index256_object, eosio::chain::contracts::index256_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::index_double_object, eosio::chain::contracts::index_double_index)

FC_REFLECT(eosio::chain::contracts::table_id_object, (id)(code)(scope)(table) )
FC_REFLECT(eosio::chain::contracts::key_value_object, (id)(t_id)(primary_key)(value)(payer) )
