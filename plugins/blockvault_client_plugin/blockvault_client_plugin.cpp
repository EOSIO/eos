#include <vector> // std::vector
#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp> // eosio::blockvault_client_plugin
#include <fc/log/log_message.hpp> // FC_LOG_MESSAGE

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
   
   struct block_vault_state {
      block_id_type producer_lib_id{};                                 ///< The  implied LIB ID for the latest accepted constructed block.
      std::pair<block_num, block_timestamp_type> producer_watermark{}; ///< The  watermark for the latest accepted constructed block.
      block_id_type snapshot_lib_id{};                                 ///< The LIB ID for the latest snapshot.
      std::pair<block_num, block_timestamp_type> snapshot_watermark{}; ///< The watermark for the latest snapshot.
   };

   void prune_state() {
      
   }

   block_vault_state bvs;
   std::vector<signed_block> proposed_blocks;
   std::vector<signed_block> appended_blocks;
};

blockvault_client_plugin::blockvault_client_plugin()
:my{new blockvault_client_plugin_impl()} {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::blockvault_client_plugin()`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::blockvault_client_plugin()`"});
}
   
blockvault_client_plugin::~blockvault_client_plugin() {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::~blockvault_client_plugin()`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::~blockvault_client_plugin()`"});
}

void blockvault_client_plugin::set_program_options(options_description&, options_description& cfg) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::set_program_options`"});
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         // TODO: Start a new cluster(?).
         // TODO: Connect to an already existing cluster(?).
         // TODO: Offer snapshot(?).
         ;
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::set_program_options`"});
}

void blockvault_client_plugin::plugin_initialize(const variables_map& options) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::plugin_initialize`"});
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
      // TODO: Start a new cluster(?).
      // TODO: Connect to an already existing cluster(?).
      // TODO: Offer snapshot(?).
   } catch (...) {} // FC_LOG_AND_RETHROW()
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::plugin_initialize`"});
}

void blockvault_client_plugin::plugin_startup() {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::plugin_startup`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::plugin_startup`"});
}

void blockvault_client_plugin::plugin_shutdown() {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::plugin_shutdown`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::plugin_shutdown`"});
}

// TODO: `signed_block or `signed_block_ptr`?
// TODO: How can I ensure that it's serialized?
void blockvault_client_plugin::propose_constructed_block(signed_block sb, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::propose_constructed_block`"});   
   // if (std::get<block_num>(watermark) > std::get<block_num>(my->bvs.producer_watermark) &&
   //     std::get<block_timestamp_type>(watermark) > std::get<block_timestamp_type>(my->bvs.producer_watermark)) {
   //    my->proposed_blocks.push_back(sb);
   //    my->bvs.producer_lib_id = lib_id;
   //    my->bvs.producer_watermark = watermark;
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::propose_constructed_block` proposed block accepted"});
   // }
   // else {
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::propose_constructed_block` proposed block rejected"});
   // }
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::propose_constructed_block`"});
}
    
void blockvault_client_plugin::append_external_block(signed_block sb, block_id_type lib_id) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block`"});
   // if (lib_id == my->bvs.producer_lib_id &&
   //     sb.block_num() == std::get<block_num>(my->bvs.snapshot_watermark) &&
   //     (lib_id > std::get<block_num>(my->bvs.producer_watermark) || std::get<block_num>(my->bvs.producer_watermark) > lib_id)) {
   //    my->appended_blocks.push_back(sb);
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block` appended block accepted"});
   // } else {
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block` appended block rejected"});
   // }
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::append_external_block`"});
}

void blockvault_client_plugin::propose_snapshot(snapshot_reader_ptr snapshot, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::propose_snapshot`"});
   // if (std::get<block_num>(watermark) > std::get<block_num>(my->bvs.snapshot_watermark) &&
   //     std::get<block_num>(watermark) < std::get<block_num>(my->bvs.producer_watermark)) {
   //    my->bvs.prune_state();
   //    my->bvs.snapshot_lib_id = lib_id;
   //    my->bvs.watermark = watermark;
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block` snapshot accepted"});
   // } else {
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::append_external_block` snapshot rejected"});
   // }
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::propose_snapshot`"});
}

void blockvault_client_plugin::sync_for_construction(std::optional<block_num> block_height) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault_client_plugin::sync_for_construction`"});
   // TODO: Integrate Chris's code here.
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault_client_plugin::sync_for_construction`"});
}

}
