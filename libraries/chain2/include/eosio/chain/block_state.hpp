#pragma once
#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/block_trace.hpp>

namespace eosio { namespace chain {

   struct block_state : public block_header_state {
      block_state( block_header_state h, signed_block_ptr b );

      /// weak_ptr prev_block_state....
      signed_block_ptr                                    block;
      map<transaction_id_type,transaction_metadata_ptr>   input_transactions;
      bool                                                validated = false;
      block_trace_ptr                                     trace;
   };

   struct block_trace {
      uint64_t                    cpu_usage;
      vector<action_receipt>      action_receipts;
      vector<transaction_receipt> transaction_receipts;
   };

   typedef std::shared_ptr<block_state> block_state_ptr;



} } /// namespace eosio::chain
