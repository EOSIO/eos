#pragma once
#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/transaction_metadata.hpp>

namespace eosio { namespace chain {

   /**
    *  For each action dispatched this receipt is generated
    */
   struct action_receipt {
      account_name                    receiver;
      action                          act;
      uint64_t                        global_sequence = 0; ///< total number of actions dispatched since genesis
      uint64_t                        recv_sequence = 0; ///< total number of actions with this receiver since genesis
      flat_map<account_name,uint32_t> auth_sequence; 

      digest_type digest()const { return digest_type::hash(*this); }
   };

   struct action_trace : public action_receipt {
      using action_receipt::action_receipt;

      fc::microseconds        ellapsed;
      uint64_t                cpu_usage = 0;
      string                  console;

      uint64_t                total_inline_cpu_usage = 0; /// total of inline_traces[x].cpu_usage + cpu_usage
      vector<action_trace>    inline_traces;
   };

   struct transaction_trace {
      fc::microseconds        ellapsed;
      uint64_t                cpu_usage = 0;
      vector<action_trace>    action_traces; ///< disposable
      vector<action_receipt>  action_receipts; ///< part of consensus 
   };

   typedef std::shared_ptr<transaction_trace> transaction_trace_ptr;

   struct block_trace {
      fc::microseconds                ellapsed;
      uint64_t                        cpu_usage = 0;
      vector<transaction_trace_ptr>   trx_traces;
   };
   typedef std::shared_ptr<block_trace> block_trace_ptr;


   struct block_state : public block_header_state {
      block_state( const block_header_state& cur ):block_header_state(cur){}
      block_state( const block_header_state& prev, signed_block_ptr b );
      block_state( const block_header_state& prev, block_timestamp_type when );
      block_state() = default;

      /// weak_ptr prev_block_state....
      signed_block_ptr                                    block;
      bool                                                validated = false;
   };

   typedef std::shared_ptr<block_state> block_state_ptr;



} } /// namespace eosio::chain

FC_REFLECT( eosio::chain::action_receipt, (receiver)(act)(global_sequence)(recv_sequence)(auth_sequence) )
FC_REFLECT_DERIVED( eosio::chain::action_trace, (eosio::chain::action_receipt), (ellapsed)(cpu_usage)(console)(total_inline_cpu_usage)(inline_traces) )
FC_REFLECT_DERIVED( eosio::chain::block_state, (eosio::chain::block_header_state), (block)(validated) )
