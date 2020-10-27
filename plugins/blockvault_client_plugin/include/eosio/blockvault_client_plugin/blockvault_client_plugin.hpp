// CORE TODO
// [ ] `net_plugin`: block broadcast decoupled from controller
// [ ] `producer_plugin`: control block broadcast
// [ ] `producer_plugin`: send committed blocks to BlockVault.
// [ ] `producer_plugin`: send accepted blocks to BlockVault.
// [ ] `producer_plugin`: store snapshots in BlockVault driven by RPC.
//
// [X] BlockVault "propose_constructed_block" interface.
// [ ] BlockVault "propose_constructed_block" implementation.
// [ ] BlockVault "propose_constructed_block" unit tests with mocked connection.
//
// [X] BlockVault "append_external_block" interface.
// [ ] BlockVault "append_external_block" implementation.
// [ ] BlockVault "append_external_block" unit tests with mocked connection.
//
// [X] BlockVault "propose_snapshot" interface.
// [ ] BlockVault "propose_snapshot" implementation.
// [ ] BlockVault "propose_snapshot" unit tests with mocked connection.
//
// [ ] `net_plugin`: disable `net_plugin` syncing when BlockVault is configured.
// [ ] `chain_plugin`: `chain_plugin` integration.
// [ ] BlockVault "sync_for_construction" interface.
// [ ] BlockVault "sync_for_construction" implementation.
// [ ] BlockVault "sync_for_construction" unit tests with mocked connection and consumer.
// [ ] BlockVault "sync_for_construction" sync controller with BlockVault when configured implementation.
// [ ] BlockVault "sync_for_construction" unit tests with mocked provider.

// LONG-TERM TOOD
// Rid all `using namespace` for verbosity-sake (or perhaps not).
// After interface is implemented double-check documentation comments.
// Make sure to delete all trailing whitespace in all touched files.
// Merge blockvault changes into my branch.
// Merge develop into blockvault.
// Submit PR to blockvault branch.

// SHORT-TERM TODO
// Finish plugin init test.
// Finish rest of impl.

#pragma once

#include <optional> // std::optional
#include <utility> // std::pair
#include <appbase/application.hpp> // appbase::plugin
// #include <eosio/chain_plugin/chain_plugin.hpp>
// #include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain/types.hpp> // eosio::block_id_type
#include <eosio/chain/block.hpp> // eosio::signed_block
#include <eosio/chain/snapshot.hpp> // eosio::snapshot_reader_ptr
#include <eosio/chain/block_timestamp.hpp> // eosio::block_timestamp

namespace eosio {

using namespace appbase;
using namespace eosio;
using namespace eosio::chain;

using block_num = uint32_t;

/// Documentation for blockvault_client_plugin.
///
/// blockvault_client_plugin is a proposed clustered component in an EOSIO
/// network architecture which provides a replicated durable storage with strong
/// consistency guarantees for all the input required by a redundant cluster of
/// nodeos nodes to achieve the guarantees:
///
/// Guarantee against double-production of blocks
/// Guarantee against finality violation
/// Guarantee of liveness (ability to make progress as a blockchain)
class blockvault_client_plugin : public appbase::plugin<blockvault_client_plugin> {
public:
   blockvault_client_plugin();
   virtual ~blockvault_client_plugin();

   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) final;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   /// \fn propose_constructed_block(signed_block sb, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark)
   ///
   /// \brief The primary method for adding constructed blocks to the Block
   /// Vault. If a proposed constructed block is accepted, the Block Vault
   /// cluster will guarantee that no future proposed blocks will be accepted
   /// which is in conflict with this block due to double production or finality
   /// violations. If the Block Vault cannot make that guarantee for any reason,
   /// it must reject the proposal.
   ///
   /// \param sb A serialized and signed constructed block.
   ///
   /// \param lib_id The LIB ID implied by accepting this block.
   ///
   /// \param watermark The producer watermark implied by accepting this block.
   ///
   void propose_constructed_block(signed_block sb, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark);

   /// \fn append_external_block(signed_block sb, block_id_type lib_id)
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
   /// \param sb A serialized and signed externally discovered block.
   ///
   /// \param lib_id The LIB ID implied by accepting this block.
   ///
   void append_external_block(signed_block sb, block_id_type lib_id);

   /// \fn propose_snapshot(snapshot_reader_ptr snapshot, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark)
   ///
   /// \brief The primary method for a nodeos node to offer snapshot data to
   /// Block Vault that facilitates log pruning. If a snapshot's height is
   /// greater than the current snapshot AND less than or equal to the current
   /// implied LIB height, it will be accepted, and Block Vault will be able to
   /// prune its state internally. Otherwise, it should reject the proposed
   /// snapshot.
   ///
   /// \param snapshot Serialized snapshot.
   ///
   /// \param lib_id The LIB ID (also HEAD block) implied at the block height of this snapshot.
   ///
   /// \param watermark The producer watermark at the block height of this snapshot.
   ///
   void propose_snapshot(snapshot_reader_ptr snapshot, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark);

   /// \fn sync_for_construction(std::optional<block_num> block_height)
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
   /// \param block_height The last synced block height (optional).
   ///
   void sync_for_construction(std::optional<block_num> block_height);

private:
   std::unique_ptr<class blockvault_client_plugin_impl> my;
};

}
