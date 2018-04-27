/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/action.hpp>
#include <eosio/chain/action_receipt.hpp>
#include <eosio/chain/block.hpp>

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
      transaction_id_type          id;
      transaction_receipt_header   receipt;
      fc::microseconds             elapsed;
      uint64_t                     cpu_usage = 0;
      bool                         scheduled = false;
      vector<action_trace>         action_traces; ///< disposable
      fc::optional<fc::exception>  soft_except;
      fc::optional<fc::exception>  hard_except;

      std::exception_ptr           soft_except_ptr;
      std::exception_ptr           hard_except_ptr;

      uint32_t  kcpu_usage()const { return (cpu_usage + 1023)/1024; }
   };
   typedef std::shared_ptr<transaction_trace> transaction_trace_ptr;

   struct block_trace {
      fc::microseconds                elapsed;
      uint64_t                        cpu_usage;
      vector<transaction_trace_ptr>   trx_traces;
   };
   typedef std::shared_ptr<block_trace> block_trace_ptr;

} }  /// namespace eosio::chain

FC_REFLECT( eosio::chain::action_trace,
                    (receipt)(act)(elapsed)(cpu_usage)(console)(total_inline_cpu_usage)(inline_traces) )
FC_REFLECT( eosio::chain::transaction_trace, (id)(receipt)(elapsed)(cpu_usage)(scheduled)(action_traces)(soft_except)(hard_except) )
FC_REFLECT( eosio::chain::block_trace, (elapsed)(cpu_usage)(trx_traces) )
