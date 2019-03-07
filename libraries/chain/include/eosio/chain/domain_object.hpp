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

class domain_object : public cyberway::chaindb::object<domain_object_type, domain_object> {
   OBJECT_CTOR(domain_object)

   id_type              id;
   account_name         owner;
   account_name         linked_to;
   block_timestamp_type creation_date;
   domain_name          name;
};
using domain_id_type = domain_object::id_type;

struct by_name;
struct by_owner;
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

class username_object : public cyberway::chaindb::object<username_object_type, username_object> {
   OBJECT_CTOR(username_object)

   id_type              id;
   account_name         owner;
   account_name         scope;   // smart-contract (app), where name registered
   username             name;
};
using username_id_type = username_object::id_type;

struct by_scope_name;
struct by_owner;
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

} } // eosio::chain

CHAINDB_SET_TABLE_TYPE(eosio::chain::domain_object, eosio::chain::domain_table)
CHAINDB_TAG(eosio::chain::domain_object, domain)
CHAINDB_SET_TABLE_TYPE(eosio::chain::username_object, eosio::chain::username_table)
CHAINDB_TAG(eosio::chain::username_object, username)

FC_REFLECT(eosio::chain::domain_object, (id)(owner)(linked_to)(creation_date)(name))
FC_REFLECT(eosio::chain::username_object, (id)(owner)(scope)(name))
