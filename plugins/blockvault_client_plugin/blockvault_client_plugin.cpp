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



}
