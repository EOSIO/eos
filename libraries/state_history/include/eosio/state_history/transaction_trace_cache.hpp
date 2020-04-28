#pragma once

#include <eosio/chain/block_state.hpp>
#include <eosio/state_history/types.hpp>

namespace eosio {
namespace state_history {

using chain::block_state_ptr;
using chain::packed_transaction_ptr;
using chain::transaction_id_type;

struct transaction_trace_cache {
   std::map<transaction_id_type, augmented_transaction_trace> cached_traces;
   fc::optional<augmented_transaction_trace>                  onblock_trace;

   void add_transaction(const transaction_trace_ptr& trace, const packed_transaction_ptr& transaction);

   std::vector<augmented_transaction_trace> prepare_traces(const block_state_ptr& block_state);
};

} // namespace state_history
} // namespace eosio
