#pragma once

#include <eosio/trace_api/trace.hpp>
#include <eosio/chain/block_state.hpp>

namespace eosio { namespace trace_api {

/// Used by to_transaction_trace  for creation of action_trace_v0 or action_trace_v1
template<typename ActionTrace>
inline ActionTrace to_action_trace( const chain::action_trace& at ) {
   ActionTrace r;
   r.receiver = at.receiver;
   r.account = at.act.account;
   r.action = at.act.name;
   r.data = at.act.data;
   if constexpr(std::is_same_v<ActionTrace, action_trace_v1>){
      r.return_value = at.return_value;
   }
   if( at.receipt ) {
      r.global_sequence = at.receipt->global_sequence;
   }
   r.authorization.reserve( at.act.authorization.size());
   for( const auto& auth : at.act.authorization ) {
      r.authorization.emplace_back( authorization_trace_v0{auth.actor, auth.permission} );
   }
   return r;
}

template<typename TransactionTrace>
inline TransactionTrace to_transaction_trace( const cache_trace& t ) {
   TransactionTrace r;
   if( !t.trace->failed_dtrx_trace ) {
      r.id = t.trace->id;
   } else {
      r.id = t.trace->failed_dtrx_trace->id; // report the failed trx id since that is the id known to user
   }
   if constexpr(std::is_same_v<TransactionTrace, transaction_trace_v1<action_trace_v0>>  || std::is_same_v<TransactionTrace, transaction_trace_v1<action_trace_v1>>){
      if (t.trace->receipt) {
         r.status = t.trace->receipt->status;
         r.cpu_usage_us = t.trace->receipt->cpu_usage_us;
         r.net_usage_words = t.trace->receipt->net_usage_words;
      }
      auto sigs = t.trx->get_signatures();
      if( sigs ) r.signatures = *sigs;
      r.trx_header = static_cast<const chain::transaction_header&>( t.trx->get_transaction() );
   }

   r.actions.reserve( t.trace->action_traces.size());
   for( const auto& at : t.trace->action_traces ) {
      if( !at.context_free ) { // not including CFA at this time
         using action_trace_tc = std::conditional_t<std::is_same_v<TransactionTrace, transaction_trace_v1<action_trace_v1>>, action_trace_v1, action_trace_v0>;
         r.actions.emplace_back( to_action_trace<action_trace_tc>(at) );

      }
   }
   return r;
}


template<typename BlockTrace>
inline BlockTrace create_block_trace( const chain::block_state_ptr& bsp ) {
   BlockTrace r;
   r.id = bsp->id;
   r.number = bsp->block_num;
   r.previous_id = bsp->block->previous;
   r.timestamp = bsp->block->timestamp;
   r.producer = bsp->block->producer;
   if constexpr(std::is_same_v<BlockTrace, block_trace_v1<action_trace_v0>>  || std::is_same_v<BlockTrace, block_trace_v1<action_trace_v1>>)
   {
      r.schedule_version = bsp->block->schedule_version;
      r.transaction_mroot = bsp->block->transaction_mroot;
      r.action_mroot = bsp->block->action_mroot;
   }
   return r;
}


} }
