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
#include "blockvault.hpp"

namespace eosio {

using namespace appbase;
using namespace eosio;
using namespace eosio::chain;

using block_num = uint32_t;

class blockvault_client_plugin_impl;

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
   eosio::blockvault::block_vault_interface* get();

 private:
   std::unique_ptr<class blockvault_client_plugin_impl> my;
};

}
