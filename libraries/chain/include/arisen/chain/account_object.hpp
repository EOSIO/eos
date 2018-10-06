/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */
#pragma once
#include <arisen/chain/types.hpp>
#include <arisen/chain/authority.hpp>
#include <arisen/chain/block_timestamp.hpp>
#include <arisen/chain/abi_def.hpp>

#include "multi_index_includes.hpp"

namespace arisen { namespace chain {

   class account_object : public chainbase::object<account_object_type, account_object> {
      OBJECT_CTOR(account_object,(code)(abi))

      id_type              id;
      account_name         name;
      uint8_t              vm_type      = 0;
      uint8_t              vm_version   = 0;
      bool                 privileged   = false;

      time_point           last_code_update;
      digest_type          code_version;
      block_timestamp_type creation_date;

      shared_string  code;
      shared_string  abi;

      void set_abi( const arisen::chain::abi_def& a ) {
         abi.resize( fc::raw::pack_size( a ) );
         fc::datastream<char*> ds( abi.data(), abi.size() );
         fc::raw::pack( ds, a );
      }

      arisen::chain::abi_def get_abi()const {
         arisen::chain::abi_def a;
         RSN_ASSERT( abi.size() != 0, abi_not_found_exception, "No ABI set on account ${n}", ("n",name) );

         fc::datastream<const char*> ds( abi.data(), abi.size() );
         fc::raw::unpack( ds, a );
         return a;
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

   class account_sequence_object : public chainbase::object<account_sequence_object_type, account_sequence_object>
   {
      OBJECT_CTOR(account_sequence_object);

      id_type      id;
      account_name name;
      uint64_t     recv_sequence = 0;
      uint64_t     auth_sequence = 0;
      uint64_t     code_sequence = 0;
      uint64_t     abi_sequence  = 0;
   };

   struct by_name;
   using account_sequence_index = chainbase::shared_multi_index_container<
      account_sequence_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<account_sequence_object, account_sequence_object::id_type, &account_sequence_object::id>>,
         ordered_unique<tag<by_name>, member<account_sequence_object, account_name, &account_sequence_object::name>>
      >
   >;

} } // arisen::chain

CHAINBASE_SET_INDEX_TYPE(arisen::chain::account_object, arisen::chain::account_index)
CHAINBASE_SET_INDEX_TYPE(arisen::chain::account_sequence_object, arisen::chain::account_sequence_index)


FC_REFLECT(arisen::chain::account_object, (name)(vm_type)(vm_version)(code_version)(code)(creation_date))
