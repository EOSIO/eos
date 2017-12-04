/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/contracts/types.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   class scope_serial_object : public chainbase::object<scope_serial_object_type, scope_serial_object> {
      OBJECT_CTOR(scope_serial_object)

      id_type              id;
      scope_name           scope;
      account_name         receiver;
      uint64_t             serial       = 0;
   };
   using scope_serial_id_type = scope_serial_object::id_type;

   struct by_scope_receiver;
   using scope_serial_multi_index = chainbase::shared_multi_index_container<
      scope_serial_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<scope_serial_object, scope_serial_object::id_type, &scope_serial_object::id>>,
         ordered_unique<tag<by_scope_receiver>,
            composite_key< scope_serial_object,
               member<scope_serial_object, scope_name, &scope_serial_object::scope>,
               member<scope_serial_object, account_name, &scope_serial_object::receiver>
            >
         >
      >
   >;

   typedef chainbase::generic_index<scope_serial_multi_index> scope_serial_index;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::scope_serial_object, eosio::chain::scope_serial_multi_index)

FC_REFLECT(chainbase::oid<eosio::chain::scope_serial_object>, (_id))

FC_REFLECT(eosio::chain::scope_serial_object, (id)(scope)(receiver)(serial))
