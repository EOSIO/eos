/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/database_utils.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/multi_index_includes.hpp>

namespace eosio { namespace chain {

class domain_object : public chainbase::object<domain_object_type, domain_object> {
   OBJECT_CTOR(domain_object)

   id_type              id;
   account_name         owner;
   block_timestamp_type creation_date;
   domain_name          name;
};
using domain_id_type = domain_object::id_type;

struct by_name;
struct by_owner;
using domain_index = cyberway::chaindb::shared_multi_index_container<
   domain_object,
   cyberway::chaindb::indexed_by<
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>,
         BOOST_MULTI_INDEX_MEMBER(domain_object, domain_object::id_type, id)>,
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_name>,
         BOOST_MULTI_INDEX_MEMBER(domain_object, string, name)>,
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_owner>,
         cyberway::chaindb::composite_key<domain_object,
            BOOST_MULTI_INDEX_MEMBER(domain_object, account_name, owner),
            BOOST_MULTI_INDEX_MEMBER(domain_object, domain_name, name)>
      >
   >
>;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::domain_object, eosio::chain::domain_index)

FC_REFLECT(eosio::chain::domain_object, (id)(owner)(name)(creation_date))
