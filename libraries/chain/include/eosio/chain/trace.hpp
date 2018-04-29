/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/action.hpp>
#include <eosio/chain/action_receipt.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace chain {

   struct base_action_trace {
      base_action_trace( const action_receipt& r ):receipt(r){}
      base_action_trace(){}

      action_receipt       receipt;
      action               act;
      fc::microseconds     elapsed;
      uint64_t             cpu_usage = 0;
      string               console;

      uint64_t             total_cpu_usage = 0; /// total of inline_traces[x].cpu_usage + cpu_usage
      transaction_id_type  trx_id; ///< the transaction that generated this action
   };

   struct action_trace : public base_action_trace {
      using base_action_trace::base_action_trace;

      vector<action_trace> inline_traces;
   };

   struct transaction_trace;
   typedef std::shared_ptr<transaction_trace> transaction_trace_ptr;

   struct transaction_trace {
      transaction_id_type          id;
      transaction_receipt_header   receipt;
      fc::microseconds             elapsed;
      uint64_t                     net_usage = 0;
      uint64_t                     cpu_usage = 0;
      bool                         scheduled = false;
      vector<action_trace>         action_traces; ///< disposable

      transaction_trace_ptr         failed_dtrx_trace;
      fc::optional<fc::exception>   soft_except;
      fc::optional<fc::exception>   hard_except;
      std::exception_ptr            soft_except_ptr;
      std::exception_ptr            hard_except_ptr;

   };

   struct block_trace {
      fc::microseconds                elapsed;
      uint64_t                        cpu_usage;
      vector<transaction_trace_ptr>   trx_traces;
   };
   typedef std::shared_ptr<block_trace> block_trace_ptr;

} }  /// namespace eosio::chain

FC_REFLECT( eosio::chain::base_action_trace,
                    (receipt)(act)(elapsed)(cpu_usage)(console)(total_cpu_usage)(trx_id) )

FC_REFLECT_DERIVED( eosio::chain::action_trace,
                    (eosio::chain::base_action_trace), (inline_traces) )

FC_REFLECT( eosio::chain::transaction_trace, (id)(receipt)(elapsed)(net_usage)(cpu_usage)(scheduled)(action_traces)
                                             (failed_dtrx_trace)(soft_except)(hard_except) )
FC_REFLECT( eosio::chain::block_trace, (elapsed)(cpu_usage)(trx_traces) )
