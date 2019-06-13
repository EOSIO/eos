/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <fc/io/raw.hpp>

#include <eosio/chain/transaction.hpp>
#include <fc/uint128.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <eosio/chain/multi_index_includes.hpp>

namespace eosio { namespace chain {
   /**
    * The purpose of this object is to enable the detection of duplicate transactions. When a transaction is included
    * in a block a transaction_object is added. At the end of block processing all transaction_objects that have
    * expired can be removed from the index.
    */
   class transaction_object : public cyberway::chaindb::object<transaction_object_type, transaction_object>
   {
         CHAINDB_OBJECT_ID_CTOR(transaction_object)

         id_type             id;
         time_point_sec      expiration;
         transaction_id_type trx_id;
   };

   struct by_expiration;
   struct by_trx_id;
   using transaction_table = cyberway::chaindb::table_container<
      transaction_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique< cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(transaction_object, transaction_object::id_type, id)>,
         cyberway::chaindb::ordered_unique< cyberway::chaindb::tag<by_trx_id>, BOOST_MULTI_INDEX_MEMBER(transaction_object, transaction_id_type, trx_id)>,
         cyberway::chaindb::ordered_unique< cyberway::chaindb::tag<by_expiration>,
            cyberway::chaindb::composite_key< transaction_object,
               BOOST_MULTI_INDEX_MEMBER( transaction_object, time_point_sec, expiration ),
               BOOST_MULTI_INDEX_MEMBER( transaction_object, transaction_object::id_type, id)
            >
         >
      >
   >;

} }

CHAINDB_SET_TABLE_TYPE(eosio::chain::transaction_object, eosio::chain::transaction_table)
CHAINDB_TAG(eosio::chain::transaction_object, transaction)
FC_REFLECT(eosio::chain::transaction_object, (id)(expiration)(trx_id))
