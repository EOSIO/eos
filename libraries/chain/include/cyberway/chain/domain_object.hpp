/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/database_utils.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/multi_index_includes.hpp>

namespace cyberway { namespace chain {

using eosio::chain::account_name;
using eosio::chain::block_timestamp_type;

class domain_object : public cyberway::chaindb::object<eosio::chain::domain_object_type, domain_object> {
   CHAINDB_OBJECT_ID_CTOR(domain_object)

   id_type              id;
   account_name         owner;
   account_name         linked_to;
   block_timestamp_type creation_date;
   domain_name          name;
};
using domain_id_type = domain_object::id_type;

using eosio::chain::by_name;
using eosio::chain::by_owner;
using domain_table = cyberway::chaindb::table_container<
   domain_object,
   cyberway::chaindb::indexed_by<
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>,
         BOOST_MULTI_INDEX_MEMBER(domain_object, domain_object::id_type, id)>,
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_name>,
         BOOST_MULTI_INDEX_MEMBER(domain_object, domain_name, name)>,
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_owner>,
         cyberway::chaindb::composite_key<domain_object,
            BOOST_MULTI_INDEX_MEMBER(domain_object, account_name, owner),
            BOOST_MULTI_INDEX_MEMBER(domain_object, domain_name, name)>
      >
   >
>;

class username_object : public cyberway::chaindb::object<eosio::chain::username_object_type, username_object> {
   CHAINDB_OBJECT_ID_CTOR(username_object)

   id_type              id;
   account_name         owner;
   account_name         scope;   // smart-contract (app), where name registered
   username             name;
};
using username_id_type = username_object::id_type;

using eosio::chain::by_scope_name;
using eosio::chain::by_owner;
using username_table = cyberway::chaindb::table_container<
   username_object,
   cyberway::chaindb::indexed_by<
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>,
         BOOST_MULTI_INDEX_MEMBER(username_object, username_object::id_type, id)>,
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_scope_name>,
         cyberway::chaindb::composite_key<username_object,
            BOOST_MULTI_INDEX_MEMBER(username_object, account_name, scope),
            BOOST_MULTI_INDEX_MEMBER(username_object, username, name)>
      >,
      cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_owner>,
         cyberway::chaindb::composite_key<username_object,
            BOOST_MULTI_INDEX_MEMBER(username_object, account_name, owner),
            BOOST_MULTI_INDEX_MEMBER(username_object, account_name, scope),
            BOOST_MULTI_INDEX_MEMBER(username_object, username, name)>
      >
   >
>;

} } // cyberway::chain

CHAINDB_SET_TABLE_TYPE(cyberway::chain::domain_object, cyberway::chain::domain_table)
CHAINDB_TAG(cyberway::chain::domain_object, domain)
CHAINDB_SET_TABLE_TYPE(cyberway::chain::username_object, cyberway::chain::username_table)
CHAINDB_TAG(cyberway::chain::username_object, username)

FC_REFLECT(cyberway::chain::domain_object, (id)(owner)(linked_to)(creation_date)(name))
FC_REFLECT(cyberway::chain::username_object, (id)(owner)(scope)(name))
