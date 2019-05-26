/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/database_utils.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/multi_index_includes.hpp>

#include <cyberway/chaindb/abi_info.hpp>

namespace eosio { namespace chain {

   class account_object : public cyberway::chaindb::object<account_object_type, account_object> {
      CHAINDB_OBJECT_CTOR(account_object, name.value)

      account_name         name;
      uint8_t              vm_type      = 0;
      uint8_t              vm_version   = 0;
      bool                 privileged   = false;

      time_point           last_code_update;
      digest_type          code_version;
      digest_type          abi_version;
      block_timestamp_type creation_date;

      string               code;
      bytes                abi;

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

      cyberway::chaindb::abi_info& get_abi_info();

   private:
      std::unique_ptr<cyberway::chaindb::abi_info> abi_info_;
   };

   struct by_name;
   using account_table = cyberway::chaindb::table_container<
      account_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(account_object, account_name, name)>
      >
   >;

   class account_sequence_object : public cyberway::chaindb::object<account_sequence_object_type, account_sequence_object> {
      CHAINDB_OBJECT_CTOR(account_sequence_object, name.value)

      account_name name;
      uint64_t     recv_sequence = 0;
      uint64_t     auth_sequence = 0;
      uint64_t     code_sequence = 0;
      uint64_t     abi_sequence  = 0;
   };

   struct by_name;
   using account_sequence_table = cyberway::chaindb::table_container<
      account_sequence_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(account_sequence_object, account_name, name)>
      >
   >;

} } // eosio::chain

CHAINDB_SET_TABLE_TYPE(eosio::chain::account_object, eosio::chain::account_table)
CHAINDB_TAG(eosio::chain::account_object, account)
CHAINDB_SET_TABLE_TYPE(eosio::chain::account_sequence_object, eosio::chain::account_sequence_table)
CHAINDB_TAG(eosio::chain::account_sequence_object, accountseq)

FC_REFLECT(eosio::chain::account_object, (name)(vm_type)(vm_version)(privileged)(last_code_update)(code_version)(abi_version)(creation_date)(code)(abi))
FC_REFLECT(eosio::chain::account_sequence_object, (name)(recv_sequence)(auth_sequence)(code_sequence)(abi_sequence))
