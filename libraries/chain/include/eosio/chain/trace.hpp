#pragma once

namespace eosio { namespace chain {

   struct action_trace {
      action_trace( const action_receipt& r ):receipt(r){}
      action_trace(){}

      action_receipt       receipt;
      action               act;
      fc::microseconds     ellapsed;
      string               console;
      uint64_t             cpu_usage;
      uint64_t             total_inline_cpu_usage;
      vector<action_trace> inline_traces;
   };

   struct transaction_trace {
      fc::microseconds        ellapsed;
      uint64_t                cpu_usage;
      vector<action_trace>    action_traces; ///< disposable
   };

   typedef std::shared_ptr<transaction_trace> transaction_trace_ptr;

   struct block_trace {
      fc::microseconds                ellapsed;
      uint64_t                        cpu_usage;
      vector<transaction_trace_ptr>   trx_traces;
   };
   typedef std::shared_ptr<block_trace> block_trace_ptr;

} }  /// namespace eosio::chain
