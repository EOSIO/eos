#include <vector> // std::vector
#include <eosio/blockvault/blockvault.hpp> // eosio::blockvault
#include <fc/log/log_message.hpp> // FC_LOG_MESSAGE

namespace eosio {
static appbase::abstract_plugin& _blockvault = app().register_plugin<blockvault>();

class blockvault_impl {
public:
   blockvault_impl() {
     FC_LOG_MESSAGE(debug, std::string{"`blockvault_impl::blockvault_impl()`"});
     FC_LOG_MESSAGE(debug, std::string{"`\blockvault_impl::blockvault_impl()`"});
   }

   ~blockvault_impl() {
     FC_LOG_MESSAGE(debug, std::string{"`blockvault_impl::~blockvault_impl()`"});
     FC_LOG_MESSAGE(debug, std::string{"`\blockvault_impl::~blockvault_impl()`"});
   }
   
   struct block_vault_state {
      block_id_type lib_id{};                                          ///< The  implied LIB ID for the latest accepted constructed block.
      std::pair<block_num, block_timestamp_type> producer_watermark{}; ///< The  watermark for the latest accepted constructed block.
      block_id_type snapshot_lib_id{};                                 ///< The LIB ID for the latest snapshot.
      std::pair<block_num, block_timestamp_type> snapshot_watermark{}; ///< The watermark for the latest snapshot.
   };

   block_vault_state bvs;
   std::vector<signed_block_ptr> proposed_blocks;
};

blockvault::blockvault()
:my{new blockvault_impl()} {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::blockvault()`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::blockvault()`"});
}
   
blockvault::~blockvault() {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::~blockvault()`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::~blockvault()`"});
}

void blockvault::set_program_options(options_description&, options_description& cfg) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::set_program_options`"});
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         // TODO: Start a new cluster(?).
         // TODO: Connect to an already existing cluster(?).
         // TODO: Offer snapshot(?).
         ;
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::set_program_options`"});
}

void blockvault::plugin_initialize(const variables_map& options) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::plugin_initialize`"});
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
      // TODO: Start a new cluster(?).
      // TODO: Connect to an already existing cluster(?).
      // TODO: Offer snapshot(?).
   } catch (...) {} // FC_LOG_AND_RETHROW()
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::plugin_initialize`"});
}

void blockvault::plugin_startup() {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::plugin_startup`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::plugin_startup`"});
}

void blockvault::plugin_shutdown() {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::plugin_shutdown`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::plugin_shutdown`"});
}

void blockvault::propose_constructed_block(signed_block_ptr sbp, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::propose_constructed_block`"});
   if (std::get<block_num>(watermark) > std::get<block_num>(my->bvs.producer_watermark) &&
       std::get<block_timestamp_type>(watermark) > std::get<block_timestamp_type>(my->bvs.producer_watermark)) {
      my->proposed_blocks.push_back(sbp);
      my->bvs.producer_watermark = watermark;
      FC_LOG_MESSAGE(debug, std::string{"`blockvault::propose_constructed_block` accepted"});
   }
   else {
      FC_LOG_MESSAGE(debug, std::string{"`blockvault::propose_constructed_block` rejected"});
   }
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::propose_constructed_block`"});
}
    
void blockvault::append_external_block(signed_block_ptr sbp, block_id_type lib_id) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::append_external_block`"});
   // if (lib_id == vault. && height == vault.height && (LIB ⧁ vault.LIB || vault.LIB ⧁ LIB)) {
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault::append_external_block` accepted"});
   // } else {
   //    FC_LOG_MESSAGE(debug, std::string{"`blockvault::append_external_block` rejected"});
   // }
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::append_external_block`"});
}

void blockvault::propose_snapshot(snapshot_reader_ptr snapshot, block_id_type lib_id, std::pair<block_num, block_timestamp_type> watermark) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::propose_snapshot`"});
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::propose_snapshot`"});
}

void blockvault::sync_for_construction(std::optional<block_num> block_height) {
   FC_LOG_MESSAGE(debug, std::string{"`blockvault::sync_for_construction`"});
   // TODO: Integrate Chris's code here.
   FC_LOG_MESSAGE(debug, std::string{"`\blockvault::sync_for_construction`"});
}

}
