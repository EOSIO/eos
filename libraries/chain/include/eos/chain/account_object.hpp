/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/types.hpp>
#include <eos/chain/authority.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   class account_object : public chainbase::object<account_object_type, account_object> {
      OBJECT_CTOR(account_object,(code)(abi))

      id_type             id;
      account_name        name;
      uint8_t             vm_type      = 0;
      uint8_t             vm_version   = 0;
      fc::sha256          code_version;
      time                creation_date;
      shared_vector<char> code;
      shared_vector<char> abi;

      void set_abi( const eosio::types::abi& _abi ) {
         // Added resize(0) here to avoid bug in boost vector container
         abi.resize( 0 );
         abi.resize( fc::raw::pack_size( _abi ) );
         fc::datastream<char*> ds( abi.data(), abi.size() );
         fc::raw::pack( ds, _abi );
      }
   };
   using account_id_type = account_object::id_type;

   struct by_name;
   using account_index = chainbase::shared_multi_index_container<
      account_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<account_object, account_object::id_type, &account_object::id>>,
         ordered_unique<tag<by_name>, member<account_object, account_name, &account_object::name>>
      >
   >;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::account_object, eosio::chain::account_index)

FC_REFLECT(chainbase::oid<eosio::chain::account_object>, (_id))

FC_REFLECT(eosio::chain::account_object, (id)(name)(vm_type)(vm_version)(code_version)(code)(creation_date))
