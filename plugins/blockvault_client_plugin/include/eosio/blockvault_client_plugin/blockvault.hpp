#pragma once
#include <eosio/chain/block.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <stdint.h>
#include <string_view>

namespace eosio {
namespace blockvault {
using watermark_t = std::pair<uint32_t, eosio::chain::block_timestamp_type>;

class sync_callback {
 public:
   virtual void on_snapshot(const char* snapshot_filename)     = 0;
   virtual void on_block(eosio::chain::signed_block_ptr block) = 0;
};

class block_vault_interface {
 public:
   virtual ~block_vault_interface() {}

   ///
   /// \brief The primary method for adding constructed blocks to the Block
   /// Vault. If a proposed constructed block is accepted, the Block Vault
   /// cluster will guarantee that no future proposed blocks will be accepted
   /// which is in conflict with this block due to double production or finality
   /// violations. If the Block Vault cannot make that guarantee for any reason,
   /// it must reject the proposal.
   ///
   /// Notice that handler would be called from a thread different from the invoker
   /// of this member function.
   ///
   ///
   /// \param watermark The producer watermark implied by accepting this block.
   ///
   /// \param lib The LIB implied by accepting this block.
   ///
   /// \param block A signed constructed block.
   ///
   /// \param handler The callback function to inform caller whether the block has accepted the Block Vault or not.

   virtual void async_propose_constructed_block(watermark_t watermark, uint32_t lib,
                                                eosio::chain::signed_block_ptr block,
                                                std::function<void(bool)>      handler) = 0;

   ///
   /// \brief The primary method for adding externally discovered blocks to the
   /// Block Vault. If an external block is accepted, the Block Vault cluster
   /// will guarantee that all future nodeos nodes will know about this block OR
   /// about an accepted snapshot state that exceeds this block's block height
   /// before proposing a constructed block. If the implied LIB of this block
   /// conflicts with the Block Vault state, then it will be rejected. If the
   /// block is older than the currently available snapshot, it is still
   /// accepted but will not affect the Block Vault cluster state.
   ///
   /// Notice that handler would be called from a thread different from the invoker
   /// of this member function.
   ///
   /// \param lib The LIB implied by accepting this block.
   ///
   /// \param block A signed externally discovered block.
   ///
   /// \param handler The callback function to inform caller whether the block has accepted the Block Vault or not.
   ///
   virtual void async_append_external_block(uint32_t lib, eosio::chain::signed_block_ptr block,
                                            std::function<void(bool)> handler) = 0;

   ///
   /// \brief The primary method for a nodeos node to offer snapshot data to
   /// Block Vault that facilitates log pruning. If a snapshot's height is
   /// greater than the current snapshot AND less than or equal to the current
   /// implied LIB height, it will be accepted, and Block Vault will be able to
   /// prune its state internally. Otherwise, it should reject the proposed
   /// snapshot.
   ///
   /// \param snapshot_filename  the filename of snapshot.
   ///
   /// \param watermark The producer watermark at the block height of this snapshot.
   ///
   virtual bool propose_snapshot(watermark_t watermark, const char* snapshot_filename) = 0;

   ///
   /// \brief The primary method for bringing a new nodeos node into sync with
   /// the Block Vault. This is the primary method for bringing a new nodeos
   /// node into sync with the Block Vault. Syncing is semi-session based, a
   /// syncing nodeos will establish a session with a single Block Vault node,
   /// stored on that node for the duration of the syncing process.  This
   /// session is used to guarantee that the Block Vault node does not prune any
   /// data that will be needed to complete this sync process until it is
   /// completed OR the session is timed-out. This session is not replicated
   /// amongst other Block Vault nodes.  As a result, if the syncing process is
   /// interrupted due to inability to communicate with the chosen Block Vault
   /// node, it must restart from the beginning with a new node.
   ///
   /// \param block_id The BlockID of the best known final block on the client
   ///
   /// \param callback The callback object to receive the snapshot and blocks
   ///
   virtual void sync(const eosio::chain::block_id_type* block_id, sync_callback& callback) = 0;
};

} // namespace blockvault
} // namespace eosio
