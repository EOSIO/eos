/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>

#include <numeric>

namespace eosio { namespace chain {

   struct data_access_info {
      enum access_type {
         read = 0, ///< scope was read by this action
         write  = 1, ///< scope was (potentially) written to by this action
      };

      access_type                type;
      account_name               code;
      scope_name                 scope;

      uint64_t                   sequence;
   };

   struct action_trace {
      account_name               receiver;
      bool                       context_free;
      uint64_t                   cpu_usage;
      action                     act;
      string                     console;
      vector<data_access_info>   data_access;
      uint32_t                   auths_used;

      fc::microseconds           _profiling_us = fc::microseconds(0);
   };

   struct transaction_trace : transaction_receipt {
      using transaction_receipt::transaction_receipt;

      vector<action_trace>          action_traces;
      vector<fc::static_variant<deferred_transaction, deferred_reference>> deferred_transaction_requests;

      flat_set<shard_lock>          read_locks;
      flat_set<shard_lock>          write_locks;

      uint64_t                      cpu_usage;
      uint64_t                      net_usage;

      optional<digest_type>         packed_trx_digest;
      uint64_t                      region_id;
      uint64_t                      cycle_index;
      uint64_t                      shard_index;

      fc::microseconds              _profiling_us = fc::microseconds(0);
      fc::microseconds              _setup_profiling_us = fc::microseconds(0);
   };
} } // eosio::chain

FC_REFLECT_ENUM( eosio::chain::data_access_info::access_type, (read)(write))
FC_REFLECT( eosio::chain::data_access_info, (type)(code)(scope)(sequence))
FC_REFLECT( eosio::chain::action_trace, (receiver)(context_free)(cpu_usage)(act)(console)(data_access)(_profiling_us) )
FC_REFLECT_ENUM( eosio::chain::transaction_receipt::status_enum, (executed)(soft_fail)(hard_fail)(delayed) )
FC_REFLECT_DERIVED( eosio::chain::transaction_trace, (eosio::chain::transaction_receipt), (action_traces)(deferred_transaction_requests)(read_locks)(write_locks)(cpu_usage)(net_usage)(packed_trx_digest)(region_id)(cycle_index)(shard_index)(_profiling_us)(_setup_profiling_us) )
