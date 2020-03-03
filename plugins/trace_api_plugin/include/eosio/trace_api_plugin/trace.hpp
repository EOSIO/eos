#pragma once

#include <eosio/chain/trace.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace trace_api_plugin {

   struct authorization_trace_v0 {
      chain::name account;
      chain::name permission;
   };

   struct action_trace_v0 {
      uint64_t                            global_sequence = {};
      chain::name                         receiver = {};
      chain::name                         account = {};
      chain::name                         action = {};
      std::vector<authorization_trace_v0> authorization = {};
      chain::bytes                        data = {};
   };

   struct transaction_trace_v0 {
      using status_type = chain::transaction_receipt_header::status_enum;

      chain::transaction_id_type    id = {};
      status_type                   status = {};
      std::vector<action_trace_v0>  actions = {};
   };

   struct block_trace_v0 {
      chain::block_id_type               id = {};
      uint64_t                           number = {};
      chain::block_id_type               previous_id = {};
      chain::block_timestamp_type        timestamp = chain::block_timestamp_type(0);
      chain::name                        producer = {};
      std::vector<transaction_trace_v0>  transactions = {};
   };

   action_trace_v0 to_action_trace_v0( const chain::action_trace& at ) {
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

   transaction_trace_v0 to_transaction_trace_v0( const chain::transaction_trace& t ) {
      transaction_trace_v0 r;
      r.id = t.id;
      if( t.receipt ) {
         r.status = t.receipt->status;
      } else {
         r.status = eosio::chain::transaction_receipt_header::status_enum::hard_fail; // not really, but better than other values
      }
      r.actions.reserve( t.action_traces.size());
      for( const auto& at : t.action_traces ) {
         r.actions.emplace_back( to_action_trace_v0( at ));
      }
      return r;
   }

} }

FC_REFLECT(eosio::trace_api_plugin::authorization_trace_v0, (account)(permission))
FC_REFLECT(eosio::trace_api_plugin::action_trace_v0, (global_sequence)(receiver)(account)(action)(authorization)(data))
FC_REFLECT(eosio::trace_api_plugin::transaction_trace_v0, (id)(status)(actions))
FC_REFLECT(eosio::trace_api_plugin::block_trace_v0, (id)(number)(previous_id)(timestamp)(producer)(transactions))
