/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <chainbase/chainbase.hpp>
#include <eosio/chain/types.hpp>

namespace eosio {
using chain::account_name;
using chain::permission_name;
using chain::shared_vector;
using chain::transaction_id_type;
using namespace boost::multi_index;

class account_control_history_object : public chainbase::object<chain::account_control_history_object_type, account_control_history_object> {
   OBJECT_CTOR(account_control_history_object)

   id_type                            id;
   account_name                       controlled_account;
   permission_name                    controlled_permission;
   account_name                       controlling_account;
};

struct by_id;
struct by_controlling;
struct by_controlled_authority;
using account_control_history_multi_index = chainbase::shared_multi_index_container<
   account_control_history_object,
   indexed_by<
      ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(account_control_history_object, account_control_history_object::id_type, id)>,
      ordered_unique<tag<by_controlling>,
         composite_key< account_control_history_object,
            member<account_control_history_object, account_name,                            &account_control_history_object::controlling_account>,
            member<account_control_history_object, account_control_history_object::id_type, &account_control_history_object::id>
         >
      >,
      ordered_unique<tag<by_controlled_authority>,
         composite_key< account_control_history_object,
            member<account_control_history_object, account_name, &account_control_history_object::controlled_account>,
            member<account_control_history_object, permission_name, &account_control_history_object::controlled_permission>,
            member<account_control_history_object, account_name, &account_control_history_object::controlling_account>
         >
      >
   >
>;

typedef chainbase::generic_index<account_control_history_multi_index> account_control_history_index;

}

CHAINBASE_SET_INDEX_TYPE( eosio::account_control_history_object, eosio::account_control_history_multi_index )

FC_REFLECT( eosio::account_control_history_object, (controlled_account)(controlled_permission)(controlling_account) )

