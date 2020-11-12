#include <vector>
#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp>

#include "blockvault_impl.hpp" // Needed for `producer_plugin` changes; this won't get merged in.

namespace eosio {
    
static appbase::abstract_plugin& _blockvault_client_plugin = app().register_plugin<blockvault_client_plugin>();

class blockvault_client_plugin_impl {
public:
   blockvault_client_plugin_impl()
   {
   }

   ~blockvault_client_plugin_impl()
   {
   }

   std::vector<signed_block_ptr> block_index;
};

blockvault_client_plugin::blockvault_client_plugin()
{
}

blockvault_client_plugin::~blockvault_client_plugin()
{
}

void blockvault_client_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         // TODO: Start a new cluster(?)
         // TODO: Connect to an already existing cluster(?)
         // TODO: Connect to libpqxx(?)
         // TODO: Master node to start things off(?)
         // TODO: Offer snapshot(?)
         ;
}

void blockvault_client_plugin::plugin_initialize(const variables_map& options) {
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
      // TODO: Start a new cluster(?)
      // TODO: Connect to an already existing cluster(?)
      // TODO: Connect to libpqxx(?)
      // TODO: Master node to start things off(?)
      // TODO: Offer snapshot(?)
   } catch (...) {} // FC_LOG_AND_RETHROW()
}

void blockvault_client_plugin::plugin_startup()
{
}

void blockvault_client_plugin::plugin_shutdown()
{
}

void blockvault_client_plugin::propose_constructed_block(blockvault::watermark_t watermark, uint32_t lib, signed_block_ptr block)
{
}

void blockvault_client_plugin::append_external_block(uint32_t lib, eosio::chain::signed_block_ptr block)
{
}

void blockvault_client_plugin::async_propose_constructed_block(blockvault::watermark_t watermark, uint32_t lib, signed_block_ptr block, std::function<void(bool)> handler) {
   get()->async_propose_constructed_block(watermark, lib, block, handler);
}

void blockvault_client_plugin::async_append_external_block(uint32_t lib, eosio::chain::signed_block_ptr block, std::function<void(bool)> handler) {
   get()->async_append_external_block(lib, block, handler);
}

void blockvault_client_plugin::propose_snapshot()
{
}

void blockvault_client_plugin::sync_for_construction()
{
}
    
} // namespace eosio
