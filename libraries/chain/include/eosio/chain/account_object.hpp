/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/multi_index_includes.hpp>

namespace eosio { namespace chain {

   class account_object : public chainbase::object<account_object_type, account_object> {
      OBJECT_CTOR(account_object)

      id_type              id;
      account_name         name;
      uint8_t              vm_type      = 0;
      uint8_t              vm_version   = 0;
      bool                 privileged   = false;

      time_point           last_code_update;
      digest_type          code_version;
      block_timestamp_type creation_date;

      string  code;
      string  abi;

      void set_abi( const eosio::chain::abi_def& a ) {
         abi.resize( fc::raw::pack_size( a ) );
         fc::datastream<char*> ds( const_cast<char*>(abi.data()), abi.size() );
         fc::raw::pack( ds, a );
      }

      eosio::chain::abi_def get_abi()const {
         eosio::chain::abi_def a;
         EOS_ASSERT( abi.size() != 0, abi_not_found_exception, "No ABI set on account ${n}", ("n",name) );

         fc::datastream<const char*> ds( abi.data(), abi.size() );
         fc::raw::unpack( ds, a );
         return a;
      }
   };
   using account_id_type = account_object::id_type;

   struct by_name;
   using account_index = cyberway::chaindb::shared_multi_index_container<
      account_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(account_object, account_object::id_type, id)>,
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_name>, BOOST_MULTI_INDEX_MEMBER(account_object, account_name, name)>
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
   using account_sequence_index = cyberway::chaindb::shared_multi_index_container<
      account_sequence_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(account_sequence_object, account_sequence_object::id_type, id)>,
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_name>, BOOST_MULTI_INDEX_MEMBER(account_sequence_object, account_name, name)>
      >
   >;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::account_object, eosio::chain::account_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::account_sequence_object, eosio::chain::account_sequence_index)

//FC_REFLECT(chainbase::oid<eosio::chain::account_object>, (_id));
//FC_REFLECT(eosio::chain::account_object, (name)(vm_type)(vm_version)(code_version)(code)(creation_date))
FC_REFLECT(eosio::chain::account_object, (id)(name)(vm_type)(vm_version)(privileged)(last_code_update)(code_version)(creation_date)(code)(abi))

//FC_REFLECT(chainbase::oid<eosio::chain::account_sequence_object>, (_id));
FC_REFLECT(eosio::chain::account_sequence_object, (id)(name)(recv_sequence)(auth_sequence)(code_sequence)(abi_sequence))
