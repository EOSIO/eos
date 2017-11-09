/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/types.hpp>
#include <eos/chain/multi_index_includes.hpp>

#include <eos/types/types.hpp>

#include <chainbase/chainbase.hpp>

namespace native {
namespace eosio {
namespace chain = ::eosio::chain;
namespace types = ::eosio::types;
namespace config = ::eosio::config;

/**
 * @brief The BalanceObject class tracks the EOS balance for accounts
 */
class BalanceObject : public chainbase::object<chain::balance_object_type, BalanceObject> {
   OBJECT_CTOR(BalanceObject)

   id_type id;
   types::account_name ownerName;
   types::share_type balance = 0;
};

struct byOwnerName;

using BalanceMultiIndex = chainbase::shared_multi_index_container<
   BalanceObject,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<BalanceObject, BalanceObject::id_type, &BalanceObject::id>
      >,
      ordered_unique<tag<byOwnerName>,
         member<BalanceObject, types::account_name, &BalanceObject::ownerName>
      >
   >
>;

} } // namespace native::eos

CHAINBASE_SET_INDEX_TYPE(native::eosio::BalanceObject, native::eosio::BalanceMultiIndex)
