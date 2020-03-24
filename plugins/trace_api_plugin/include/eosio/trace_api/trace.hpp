#pragma once

#include <eosio/chain/trace.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace trace_api {

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
      std::vector<action_trace_v0>  actions = {};
   };

   struct block_trace_v0 {
      chain::block_id_type               id = {};
      uint32_t                           number = {};
      chain::block_id_type               previous_id = {};
      chain::block_timestamp_type        timestamp = chain::block_timestamp_type(0);
      chain::name                        producer = {};
      std::vector<transaction_trace_v0>  transactions = {};
   };

} }

FC_REFLECT(eosio::trace_api::authorization_trace_v0, (account)(permission))
FC_REFLECT(eosio::trace_api::action_trace_v0, (global_sequence)(receiver)(account)(action)(authorization)(data))
FC_REFLECT(eosio::trace_api::transaction_trace_v0, (id)(actions))
FC_REFLECT(eosio::trace_api::block_trace_v0, (id)(number)(previous_id)(timestamp)(producer)(transactions))
