#include <iostream>
#include <utility>
#include <vector>

#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp>

#include <fc/log/log_message.hpp>

namespace eosio {
static appbase::abstract_plugin& _blockvault_client_plugin = app().register_plugin<blockvault_client_plugin>();

blockvault_client_plugin::blockvault_client_plugin() {
   std::cout << "`blockvault_client_plugin::blockvault_client_plugin()`\n";
   
   bvs.blockvault_entity_status = blockvault_entity::leader;

   std::cout << "`\\blockvault_client_plugin::blockvault_client_plugin()`\n";
}

blockvault_client_plugin::~blockvault_client_plugin() {
   std::cout << "`blockvault_client_plugin::~blockvault_client_plugin()`\n";
   
   std::cout << "`\\blockvault_client_plugin::~blockvault_client_plugin()`\n";
}

void blockvault_client_plugin::set_program_options(options_description&, options_description& cfg) {
   std::cout << "`blockvault_client_plugin::set_program_options`\n";
   
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         // TODO: Start a new cluster(?).
         // TODO: Connect to an already existing cluster(?).
         // TODO: Offer snapshot(?).
         ;
   
   std::cout << "`\\blockvault_client_plugin::set_program_options`\n";
}

void blockvault_client_plugin::plugin_initialize(const variables_map& options) {
   std::cout << "`blockvault_client_plugin::plugin_initialize`\n";
   
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
      // TODO: Start a new cluster(?).
      // TODO: Connect to an already existing cluster(?).
      // TODO: Offer snapshot(?).
   } catch (...) {} // FC_LOG_AND_RETHROW()
   
   std::cout << "`\\blockvault_client_plugin::plugin_initialize`\n";
}

void blockvault_client_plugin::plugin_startup() {
   std::cout << "`blockvault_client_plugin::plugin_startup`\n";
   
   std::cout << "`\\blockvault_client_plugin::plugin_startup`\n";
}

void blockvault_client_plugin::plugin_shutdown() {
   std::cout << "`blockvault_client_plugin::plugin_shutdown`\n";
   
   std::cout << "`\\blockvault_client_plugin::plugin_shutdown`\n";
}

void blockvault_client_plugin::propose_constructed_block(signed_block_ptr sbp, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark) {
   std::cout << "`blockvault_client_plugin::propose_constructed_block`\n";

   if (bvs.blockvault_entity_status == blockvault_entity::leader) {
      if (std::get<block_num>(watermark) > std::get<block_num>(bvs.producer_watermark) && std::get<block_timestamp_type>(watermark) > std::get<block_timestamp_type>(bvs.producer_watermark)) {
         // bvi.append_proposed_block(watermark, lib, block_content);
         block_index.push_back(std::move(sbp));
         std::cout << "proposed block appended to the RAFT log\n";
         bvs.producer_lib_id = lib_id;
         bvs.producer_watermark = watermark;
         // get_next_raft_event();
         // send accept block signal
      } else {
        // send not leader signal
      }
   }
    
   std::cout << "`\\blockvault_client_plugin::propose_constructed_block`\n";
}

void blockvault_client_plugin::append_external_block(signed_block_ptr sbp, block_id_type lib_id) {
   std::cout << "`blockvault_client_plugin::append_external_block`\n";
   
   // if (lib_id == my->bvs.producer_lib_id &&
   //     sb.block_num() == std::get<block_num>(my->bvs.snapshot_watermark) &&
   //     (lib_id > std::get<block_num>(my->bvs.producer_watermark) || std::get<block_num>(my->bvs.producer_watermark) > lib_id)) {
   //    my->appended_blocks.push_back(sb);
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block` appended block accepted"});
   // } else {
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block` appended block rejected"});
   // }
   
   std::cout << "`\\blockvault_client_plugin::append_external_block`\n";
}

void blockvault_client_plugin::propose_snapshot(snapshot_reader_ptr snapshot, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark) {
   std::cout << "`blockvault_client_plugin::propose_snapshot`\n";
   
   // if (std::get<block_num>(watermark) > std::get<block_num>(my->bvs.snapshot_watermark) &&
   //     std::get<block_num>(watermark) < std::get<block_num>(my->bvs.producer_watermark)) {
   //    my->bvs.prune_state();
   //    my->bvs.snapshot_lib_id = lib_id;
   //    my->bvs.watermark = watermark;
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block` snapshot accepted"});
   // } else {
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block` snapshot rejected"});
   // }
   
   std::cout << "`\\blockvault_client_plugin::propose_snapshot`\n";
}

void blockvault_client_plugin::sync_for_construction(std::optional<block_num> block_height) {
   std::cout << "`blockvault_client_plugin::sync_for_construction`\n";
   
   // TODO: Integrate Chris's code here.
   
   std::cout << "`\\blockvault_client_plugin::sync_for_construction`\n";
}

}
