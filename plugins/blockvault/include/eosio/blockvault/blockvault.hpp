#pragma once
#include <appbase/application.hpp>
// #include <eosio/http_plugin/http_plugin.hpp>
// #include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;

struct block_vault_state {
   // time producer_watermark{};       // The  watermark for the latest accepted constructed block.
   // block_id_type lib_id{};          // The  implied LIB ID for the latest accepted constructed block.
   // block_id_type snapshot_lib_id{}; // The LIB ID for the latest snapshot.
   // tim snapshot_watermark{};        // The watermark for the latest snapshot.
};

class blockvault : public appbase::plugin<blockvault> {
public:
   blockvault();
   virtual ~blockvault();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   /// \brief If a proposed constructed block is accepted, the Block Vault
   /// cluster will guarantee that no future proposed blocks will be accepted
   /// which is in conflict with this block due to double production or finality
   /// violations. If the Block Vault cannot make that guarantee for any reason,
   /// it must reject the proposal.
   ///
   void propose_constructed_block();
    
   /// \brief If an external block is accepted, the Block Vault cluster will
   /// guarantee that all future nodeos nodes will know about this block OR
   /// about an accepted snapshot state that exceeds this block's block height
   /// before proposing a constructed block. If the implied LIB of this block
   /// conflicts with the Block Vault state, then it will be rejected. If the
   /// block is older than the currently available snapshot, it is still
   /// accepted but will not affect the Block Vault cluster state.
   ///
   void append_external_block();
    
   /// \brief This is the primary method for bringing a new nodeos node into
   /// sync with the Block Vault. Syncing is semi-session based, a syncing
   /// nodeos will establish a session with a single Block Vault node, stored on
   /// that node for the duration of the syncing process.  This session is used
   /// to guarantee that the Block Vault node does not prune any data that will
   /// be needed to complete this sync process until it is completed OR the
   /// session is timed-out.
   ///
   void sync_for_construction();

private:
   std::unique_ptr<class blockvault_impl> my;
   block_vault_state bvs;
};

}
