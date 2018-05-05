/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <appbase/channel.hpp>
#include <appbase/method.hpp>

#include <eosio/chain/block.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain { namespace plugin_interface {
   using namespace eosio::chain;
   using namespace appbase;

   struct chain_plugin_interface;
   
   namespace channels {
      using accepted_block_header  = channel_decl<struct accepted_block_header_tag, block_state_ptr>;
      using accepted_block         = channel_decl<struct accepted_block_tag,        block_state_ptr>;
      using irreversible_block     = channel_decl<struct irreversible_block_tag,    block_state_ptr>;
      using accepted_transaction   = channel_decl<struct accepted_transaction_tag,  transaction_metadata_ptr>;
      using applied_transaction    = channel_decl<struct applied_transaction_tag,   transaction_trace_ptr>;
      using accepted_confirmation  = channel_decl<struct accepted_confirmation_tag, header_confirmation>;

      using incoming_block         = channel_decl<struct incoming_blocks_tag,       signed_block_ptr>;
      using incoming_transaction   = channel_decl<struct incoming_transactions_tag, packed_transaction_ptr>;
   }

   namespace methods {
      using get_block_by_number    = method_decl<chain_plugin_interface, signed_block_ptr(uint32_t block_num)>;
      using get_block_by_id        = method_decl<chain_plugin_interface, signed_block_ptr(const block_id_type& block_id)>;
      using get_head_block_id      = method_decl<chain_plugin_interface, block_id_type ()>;

      using get_last_irreversible_block_number = method_decl<chain_plugin_interface, uint32_t ()>;

      // synchronously push a block/trx to a single provider
      using incoming_block_sync       = method_decl<chain_plugin_interface, void(const signed_block_ptr&), first_provider_policy>;
      using incoming_transaction_sync = method_decl<chain_plugin_interface, transaction_trace_ptr(const packed_transaction_ptr&), first_provider_policy>;

      // start the "best" coordinator
      using start_coordinator      = method_decl<chain_plugin_interface, void(), first_provider_policy>;
   }

} } }