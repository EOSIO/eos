#pragma once

#include <boost/signals2/signal.hpp>
#include <appbase/application.hpp> // appbase::plugin
// #include <blockvault.hpp>
// #include <eosio/chain_plugin/chain_plugin.hpp>
// #include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain/types.hpp> // eosio::block_id_type
#include <eosio/chain/block.hpp> // eosio::signed_block
#include <eosio/chain/snapshot.hpp> // eosio::snapshot_reader_ptr
#include <eosio/chain/block_timestamp.hpp> // eosio::block_timestamp
#include <eosio/chain/plugin_interface.hpp>

namespace eosio {
    
using namespace appbase;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::plugin_interface;

using block_num = uint32_t;

class blockvault_client_plugin : public appbase::plugin<blockvault_client_plugin> {
public:
   blockvault_client_plugin();
   virtual ~blockvault_client_plugin();

   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) final;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   void propose_constructed_block(signed_block_ptr sbp);
   void append_external_block(signed_block_ptr sbp);
   // void propose_snapshot();
   // void sync_for_construction();
   // blockvault_interface* get();

   enum class blockvault_entity : size_t {
      leader,
      follower
   };

   compat::channels::accepted_blockvault_block::channel_type& _accepted_blockvault_block_channel;

   blockvault_entity entity;
   std::vector<signed_block_ptr> block_index;
};

}
