#include <eosio/blockvault/blockvault.hpp>

namespace eosio {
   static appbase::abstract_plugin& _blockvault = app().register_plugin<blockvault>();

class blockvault_impl {
   public:
};

blockvault::blockvault():my(new blockvault_impl()){}
blockvault::~blockvault(){}

void blockvault::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void blockvault::plugin_initialize(const variables_map& options) {
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
   } catch (...) {} // FC_LOG_AND_RETHROW()
}

void blockvault::plugin_startup() {
   // Make the magic happen
}

void blockvault::plugin_shutdown() {
   // OK, that's enough magic
}

void blockvault::propose_constructed_block(){}
    
void blockvault::append_external_block(){}
    
void blockvault::sync_for_construction(){}

}
