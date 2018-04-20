/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/action.hpp>
#include <eosio/chain/action_receipt.hpp>

namespace eosio { namespace chain {

   struct action_trace {
      action_trace( const action_receipt& r ):receipt(r){}
      action_trace(){}

      action_receipt       receipt;
      action               act;
      fc::microseconds     elapsed;
      uint64_t             cpu_usage = 0;
      string               console;

      uint64_t             total_inline_cpu_usage = 0; /// total of inline_traces[x].cpu_usage + cpu_usage
      vector<action_trace> inline_traces;
   };

   struct transaction_trace {
      fc::microseconds        elapsed;
      uint64_t                cpu_usage = 0;
      vector<action_trace>    action_traces; ///< disposable
   };
   typedef std::shared_ptr<transaction_trace> transaction_trace_ptr;

   struct block_trace {
      fc::microseconds                elapsed;
      uint64_t                        cpu_usage;
      vector<transaction_trace_ptr>   trx_traces;
   };
   typedef std::shared_ptr<block_trace> block_trace_ptr;

} }  /// namespace eosio::chain

FC_REFLECT_DERIVED( eosio::chain::action_trace, (eosio::chain::action_receipt),
                    (elapsed)(cpu_usage)(console)(total_inline_cpu_usage)(inline_traces) )
