#pragma once

#include <eos/chain/types.hpp>
#include <eos/chain/multi_index_includes.hpp>

#include <eos/types/types.hpp>

#include <chainbase/chainbase.hpp>

namespace eos {

/**
 * @brief The BalanceObject class tracks the EOS balance for accounts
 */
class BalanceObject : public chainbase::object<chain::balance_object_type, BalanceObject> {
   OBJECT_CTOR(BalanceObject)

   id_type id;
   types::AccountName ownerName;
   types::ShareType balance = 0;
};

struct byOwnerName;

using BalanceMultiIndex = chainbase::shared_multi_index_container<
   BalanceObject,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<BalanceObject, BalanceObject::id_type, &BalanceObject::id>
      >,
      ordered_unique<tag<byOwnerName>,
         member<BalanceObject, types::AccountName, &BalanceObject::ownerName>
      >
   >
>;

} // namespace eos

CHAINBASE_SET_INDEX_TYPE(eos::BalanceObject, eos::BalanceMultiIndex)
