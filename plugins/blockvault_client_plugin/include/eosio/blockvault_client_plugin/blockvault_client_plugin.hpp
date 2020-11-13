#pragma once

#include <appbase/application.hpp>
#include "blockvault.hpp"

namespace eosio {

using namespace appbase;
using namespace eosio;
using namespace eosio::chain;

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

   // TODO remove all of these once branch merged; use `get()` instead
   void propose_constructed_block(blockvault::watermark_t watermark, uint32_t lib, signed_block_ptr block);
   void append_external_block(uint32_t lib, eosio::chain::signed_block_ptr block);
   void async_propose_constructed_block(blockvault::watermark_t watermark, uint32_t lib, signed_block_ptr block, std::function<void(bool)> handler);
   void async_append_external_block(uint32_t lib, eosio::chain::signed_block_ptr block, std::function<void(bool)> handler);
   void propose_snapshot(std::pair<uint32_t, uint32_t> watermark, const char* snapshot_filename);
   void sync_for_construction();

   static eosio::blockvault::block_vault_interface* get() { return nullptr; } // TODO: implement me

 private:
   std::unique_ptr<class blockvault_client_plugin_impl> my;
};

}
