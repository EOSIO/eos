#include "eosio/state_history/transaction_trace_cache.hpp"

namespace eosio {
namespace state_history {

using eosio::chain::packed_transaction;
using eosio::chain::state_history_exception;


bool is_onblock(const transaction_trace_ptr& p) {
   if (p->action_traces.size() != 1)
      return false;
   auto& act = p->action_traces[0].act;
   if (act.account != eosio::chain::config::system_account_name || act.name != N(onblock) ||
       act.authorization.size() != 1)
      return false;
   auto& auth = act.authorization[0];
   return auth.actor == eosio::chain::config::system_account_name &&
          auth.permission == eosio::chain::config::active_name;
}

void transaction_trace_cache::add_transaction(const transaction_trace_ptr& trace, const packed_transaction_ptr& transaction) {
   if (trace->receipt) {
      if (is_onblock(trace))
         onblock_trace.emplace(trace, transaction);
      else if (trace->failed_dtrx_trace)
         cached_traces[trace->failed_dtrx_trace->id] = augmented_transaction_trace{trace, transaction};
      else
         cached_traces[trace->id] = augmented_transaction_trace{trace, transaction};
   }
}

std::vector<augmented_transaction_trace> transaction_trace_cache::prepare_traces(const block_state_ptr& block_state) {

   std::vector<augmented_transaction_trace> traces;
   if (this->onblock_trace)
      traces.push_back(*this->onblock_trace);
   for (auto& r : block_state->block->transactions) {
      transaction_id_type id;
      if (r.trx.contains<transaction_id_type>())
         id = r.trx.get<transaction_id_type>();
      else
         id = r.trx.get<packed_transaction>().id();
      auto it = this->cached_traces.find(id);
      EOS_ASSERT(it != this->cached_traces.end() && it->second.trace->receipt, state_history_exception,
                 "missing trace for transaction ${id}", ("id", id));
      traces.push_back(it->second);
   }
   clear();
   return traces;
}

void transaction_trace_cache::clear() {
   this->cached_traces.clear();
   this->onblock_trace.reset();
}

}} // namespace eosio::state_history