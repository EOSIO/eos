#pragma once

#include <eosio/chain/block_state.hpp>
#include <eosio/state_history/types.hpp>

namespace eosio {
namespace state_history {

using chain::block_state_ptr;
using chain::transaction_id_type;

struct trace_converter {
   std::map<transaction_id_type, augmented_transaction_trace> cached_traces;
   fc::optional<augmented_transaction_trace>                  onblock_trace;

   void  add_transaction(const transaction_trace_ptr& trace, const signed_transaction& transaction);
   bytes pack(const chainbase::database& db, bool trace_debug_mode, const block_state_ptr& block_state);
};

} // namespace state_history
} // namespace eosio
