#pragma once

#include <eosio/chain/trace.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/block.hpp>
#include <utility>

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

   struct action_trace_v1  :  public action_trace_v0  {
      chain::bytes                        return_value = {};
   };

  struct transaction_trace_v0 {
      using status_type = chain::transaction_receipt_header::status_enum;

      chain::transaction_id_type    id = {};
      std::vector<action_trace_v0>  actions = {};
   };

  struct transaction_trace_v1 : public transaction_trace_v0 {
     fc::enum_type<uint8_t,status_type>         status = {};
     uint32_t                                   cpu_usage_us = 0;
     fc::unsigned_int                           net_usage_words;
     std::vector<chain::signature_type>         signatures = {};
     chain::transaction_header                  trx_header = {};
  };

  struct transaction_trace_v2 {
     using status_type = chain::transaction_receipt_header::status_enum;

     chain::transaction_id_type                 id = {};
     std::variant<std::vector<action_trace_v1>> actions = {};
     fc::enum_type<uint8_t,status_type>         status = {};
     uint32_t                                   cpu_usage_us = 0;
     fc::unsigned_int                           net_usage_words;
     std::vector<chain::signature_type>         signatures = {};
     chain::transaction_header                  trx_header = {};
  };

  struct block_trace_v0 {
     chain::block_id_type               id = {};
     uint32_t                           number = {};
     chain::block_id_type               previous_id = {};
     chain::block_timestamp_type        timestamp = chain::block_timestamp_type(0);
     chain::name                        producer = {};
     std::vector<transaction_trace_v0>  transactions = {};
  };

  struct block_trace_v1 : public block_trace_v0 {
     chain::checksum256_type            transaction_mroot = {};
     chain::checksum256_type            action_mroot = {};
     uint32_t                           schedule_version = {};
     std::vector<transaction_trace_v1>  transactions_v1 = {};
  };

  struct block_trace_v2 {
     chain::block_id_type               id = {};
     uint32_t                           number = {};
     chain::block_id_type               previous_id = {};
     chain::block_timestamp_type        timestamp = chain::block_timestamp_type(0);
     chain::name                        producer = {};
     chain::checksum256_type            transaction_mroot = {};
     chain::checksum256_type            action_mroot = {};
     uint32_t                           schedule_version = {};
     std::variant<std::vector<transaction_trace_v2>>  transactions = {};
  };

  struct cache_trace {
      chain::transaction_trace_ptr        trace;
      chain::packed_transaction_ptr       trx;
  };

} }

FC_REFLECT(eosio::trace_api::authorization_trace_v0, (account)(permission))
FC_REFLECT(eosio::trace_api::action_trace_v0, (global_sequence)(receiver)(account)(action)(authorization)(data))
FC_REFLECT_DERIVED(eosio::trace_api::action_trace_v1, (eosio::trace_api::action_trace_v0),(return_value))
FC_REFLECT(eosio::trace_api::transaction_trace_v0, (id)(actions))
FC_REFLECT_DERIVED(eosio::trace_api::transaction_trace_v1, (eosio::trace_api::transaction_trace_v0), (status)(cpu_usage_us)(net_usage_words)(signatures)(trx_header))
FC_REFLECT(eosio::trace_api::transaction_trace_v2, (id)(actions)(status)(cpu_usage_us)(net_usage_words)(signatures)(trx_header))
FC_REFLECT(eosio::trace_api::block_trace_v0, (id)(number)(previous_id)(timestamp)(producer)(transactions))
FC_REFLECT_DERIVED(eosio::trace_api::block_trace_v1, (eosio::trace_api::block_trace_v0), (transaction_mroot)(action_mroot)(schedule_version)(transactions_v1))
FC_REFLECT(eosio::trace_api::block_trace_v2, (id)(number)(previous_id)(timestamp)(producer)(transaction_mroot)(action_mroot)(schedule_version)(transactions))
