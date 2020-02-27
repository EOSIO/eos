#pramga once

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
      chain::block_ud_type               previous_id = {};
      chain::block_timestamp_type        timestamp = {};
      chain::name                        producer = {};
      std::vector<transaction_trace_v0>  transactions = {};
   };

} }