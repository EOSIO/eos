#pragma once

#include <eos/chain/types.hpp>
#include <eos/chain/multi_index_includes.hpp>

#include <eos/types/types.hpp>

#include <chainbase/chainbase.hpp>

namespace eos {

/**
 * @brief The StakedBalanceObject class tracks the staked balance (voting balance) for accounts
 */
class StakedBalanceObject : public chainbase::object<chain::staked_balance_object_type, StakedBalanceObject> {
   OBJECT_CTOR(StakedBalanceObject, (approvedProducers))

   id_type id;
   types::AccountName ownerName;

   types::ShareType stakedBalance = 0;
   types::ShareType unstakingBalance = 0;
   types::Time lastUnstakingTime = types::Time::maximum();

   chain::shared_set<types::AccountName> approvedProducers;
};

struct byOwnerName;

using StakedBalanceMultiIndex = chainbase::shared_multi_index_container<
   StakedBalanceObject,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<StakedBalanceObject, StakedBalanceObject::id_type, &StakedBalanceObject::id>
      >,
      ordered_unique<tag<byOwnerName>,
         member<StakedBalanceObject, types::AccountName, &StakedBalanceObject::ownerName>
      >
   >
>;

} // namespace eos

CHAINBASE_SET_INDEX_TYPE(eos::StakedBalanceObject, eos::StakedBalanceMultiIndex)
