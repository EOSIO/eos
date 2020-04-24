#pragma once

#include <eosio/trace_api/trace.hpp>
#include <eosio/chain/block_state.hpp>

namespace eosio { namespace trace_api {

/// Used by to_transaction_trace_v0 for creation of action_trace_v0
inline action_trace_v0 to_action_trace_v0( const chain::action_trace& at ) {
   action_trace_v0 r;
   r.receiver = at.receiver;
   r.account = at.act.account;
   r.action = at.act.name;
   r.data = at.act.data;
   if( at.receipt ) {
      r.global_sequence = at.receipt->global_sequence;
   }
   r.authorization.reserve( at.act.authorization.size());
   for( const auto& auth : at.act.authorization ) {
      r.authorization.emplace_back( authorization_trace_v0{auth.actor, auth.permission} );
   }
   return r;
}

/// @return transaction_trace_v0 with populated action_trace_v0
inline transaction_trace_v0 to_transaction_trace_v0( const chain::transaction_trace_ptr& t ) {
   transaction_trace_v0 r;
   if( !t->failed_dtrx_trace ) {
      r.id = t->id;
   } else {
      r.id = t->failed_dtrx_trace->id; // report the failed trx id since that is the id known to user
   }
   r.actions.reserve( t->action_traces.size());
   for( const auto& at : t->action_traces ) {
      if( !at.context_free ) { // not including CFA at this time
         r.actions.emplace_back( to_action_trace_v0( at ));
      }
   }
   return r;
}

inline transaction_trace_v1 to_transaction_trace_v1( const chain::transaction_trace_ptr& t, const chain::signed_transaction& trx = {} ) {
    transaction_trace_v1 r;
    if( !t->failed_dtrx_trace ) {
        r.id = t->id;
        r.status = t->receipt->status;
        r.cpu_usage_us = t->receipt->cpu_usage_us;
        r.net_usage_words = t->receipt->net_usage_words;
    } else {
        r.id = t->failed_dtrx_trace->id; // report the failed trx id since that is the id known to user
        r.status = t->failed_dtrx_trace->receipt->status;
        r.cpu_usage_us = t->failed_dtrx_trace->receipt->cpu_usage_us;
        r.net_usage_words = t->failed_dtrx_trace->receipt->net_usage_words;
    }
    r.signatures = trx.signatures;
    r.trx_header = chain::transaction_header{
            trx.expiration,
            trx.ref_block_num,
            trx.ref_block_prefix,
            trx.max_net_usage_words,
            trx.max_cpu_usage_ms,
            trx.delay_sec
    };
    r.actions.reserve( t->action_traces.size());
    for( const auto& at : t->action_traces ) {
        if( !at.context_free ) { // not including CFA at this time
            r.actions.emplace_back( to_action_trace_v0( at ));
        }
    }
    return r;
}

/// @return block_trace_v0 without any transaction_trace_v0
inline block_trace_v0 create_block_trace_v0( const chain::block_state_ptr& bsp ) {
   block_trace_v0 r;
   r.id = bsp->id;
   r.number = bsp->block_num;
   r.previous_id = bsp->block->previous;
   r.timestamp = bsp->block->timestamp;
   r.producer = bsp->block->producer;
   return r;
}

/// @return block_trace_v1 without any transaction_trace_v1
inline block_trace_v1 create_block_trace_v1( const chain::block_state_ptr& bsp ) {
    block_trace_v1 r;
    r.id = bsp->id;
    r.number = bsp->block_num;
    r.previous_id = bsp->block->previous;
    r.timestamp = bsp->block->timestamp;
    r.producer = bsp->block->producer;
    r.schedule_version = bsp->block->schedule_version;
    r.transaction_mroot = bsp->block->transaction_mroot;
    r.action_mroot = bsp->block->action_mroot;
    return r;
}

} }
