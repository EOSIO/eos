#pragma once
#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace chain {

   struct block_state : public block_header_state {
      block_state( block_header_state h, signed_block_ptr b );

      signed_block_ptr                                    block;
      map<transaction_id_type,transaction_metadata_ptr>   input_transactions;
      bool                                                validated = false;
      block_trace_ptr                                     trace;
   };

   typedef std::shared_ptr<block_state> block_state_ptr;



} } /// namespace eosio::chain
