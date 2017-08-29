#pragma once

#include <chainbase/chainbase.hpp>
#include <eos/chain/types.hpp>

namespace eos {
using chain::AccountName;
using chain::PermissionName;
using chain::shared_vector;
using chain::transaction_id_type;
using namespace boost::multi_index;

class account_control_history_object : public chainbase::object<chain::account_control_history_object_type, account_control_history_object> {
   OBJECT_CTOR(account_control_history_object)

   id_type                            id;
   AccountName                        controlled_account;
   PermissionName                     controlled_permission;
   AccountName                        controlling_account;
};

struct by_id;
struct by_controlling;
struct by_controlled_authority;
using account_control_history_multi_index = chainbase::shared_multi_index_container<
   account_control_history_object,
   indexed_by<
      ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(account_control_history_object, account_control_history_object::id_type, id)>,
      hashed_non_unique<tag<by_controlling>, BOOST_MULTI_INDEX_MEMBER(account_control_history_object, AccountName, controlling_account), std::hash<AccountName>>,
      hashed_non_unique<tag<by_controlled_authority>,
         composite_key< account_control_history_object,
            member<account_control_history_object, AccountName, &account_control_history_object::controlled_account>,
            member<account_control_history_object, PermissionName, &account_control_history_object::controlled_permission>
         >,
         composite_key_hash< std::hash<AccountName>, std::hash<PermissionName> >
      >
   >
>;

typedef chainbase::generic_index<account_control_history_multi_index> account_control_history_index;

}

CHAINBASE_SET_INDEX_TYPE( eos::account_control_history_object, eos::account_control_history_multi_index )

FC_REFLECT( eos::account_control_history_object, (controlled_account)(controlled_permission)(controlling_account) )

