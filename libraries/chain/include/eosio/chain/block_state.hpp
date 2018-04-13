#pragma once
#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/transaction_metadata.hpp>

namespace eosio { namespace chain {

   /**
    *  For each action dispatched this receipt is generated
    */
   struct action_receipt {
      account_name    receiver;
      action          act;
      uint32_t        block_sequence;
      uint32_t        sender_sequence;
      fc::sha256      transaction_id_type;  /// the trx that started this action
      uint16_t        trx_action_dispatch_seq; ///< the relative order for implied trx
      uint16_t        trx_action_creator_seq; ///< the dispatch seq of the action that created this dispatch
      uint16_t        trx_action_dispatch_count; ///< the total number of actions dispatched by transaction when this action was dispatched
      /// TODO: add code/scope/rw sequence numbers
   };

   struct action_trace {
      fc::microseconds ellapsed;
      uint64_t         kops;
      string           console;
   };

   struct transaction_trace {
      fc::microseconds        ellapsed;
      uint64_t                cpu_usage;
      vector<action_trace>    action_traces; ///< disposable
      vector<action_receipt>  action_receipts; ///< part of consensus 
   };

   typedef std::shared_ptr<transaction_trace> transaction_trace_ptr;

   struct block_trace {
      fc::microseconds            ellapsed;
      uint64_t                    cpu_usage;
      vector<transaction_trace>   trx_traces;
   };
   typedef std::shared_ptr<block_trace> block_trace_ptr;


   struct block_state : public block_header_state {
      block_state( block_header_state h, signed_block_ptr b );

      /// weak_ptr prev_block_state....
      signed_block_ptr                                    block;
      map<transaction_id_type,transaction_metadata_ptr>   input_transactions;
      bool                                                validated = false;
      block_trace_ptr                                     trace;

      void reset_trace();
   };

   typedef std::shared_ptr<block_state> block_state_ptr;



} } /// namespace eosio::chain

FC_REFLECT_DERIVED( eosio::chain::block_state, (eosio::chain::block_header_state),  )
