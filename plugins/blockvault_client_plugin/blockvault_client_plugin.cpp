#include <iostream>
#include <utility>
#include <vector>

#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp>

namespace eosio {
static appbase::abstract_plugin& _blockvault_client_plugin = app().register_plugin<blockvault_client_plugin>();

class blockvault_client_plugin_impl {
public:
   blockvault_client_plugin_impl() {
     FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin_impl::blockvault_client_plugin_impl()`"});
     FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin_impl::blockvault_client_plugin_impl()`"});
   }

   ~blockvault_client_plugin_impl() {
     FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin_impl::~blockvault_client_plugin_impl()`"});
     FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin_impl::~blockvault_client_plugin_impl()`"});
   }
};

blockvault_client_plugin::blockvault_client_plugin()
:_accepted_blockvault_block_channel(app().get_channel<compat::channels::accepted_blockvault_block>())
{
}

blockvault_client_plugin::~blockvault_client_plugin() {
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

void blockvault_client_plugin::plugin_startup() {
}

void blockvault_client_plugin::plugin_shutdown() {
}

void blockvault_client_plugin::propose_constructed_block(signed_block_ptr sbp) {
   watermark_t watermark;
   uint32_t lib;
   std::string_view block_content;
   if (get()->append_proposed_block(watermark, lib, block_content) == true) {
      publish(priority::high, sbp);
   }
}

void blockvault_client_plugin::append_external_block(signed_block_ptr sbp) {
   watermark_t watermark;
   std::string_view block_content;
   if (get()->append_proposed_block(watermark, block_content) == true) {
      publish(priority::high, sbp);
   }
}

// void blockvault_client_plugin::propose_snapshot() {
// }
// 
// void blockvault_client_plugin::sync_for_construction() {
//    // TODO: Integrate Chris's code here.
// }
// blockvault_interface* blockvault_client_plugin::get() {
// }
}

// We need to get rid of the signal that is in chain.commit of which net_plugin
