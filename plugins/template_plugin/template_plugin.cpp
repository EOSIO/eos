/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/template_plugin/template_plugin.hpp>

namespace eos {

class template_plugin_impl {
   public:
};

template_plugin::template_plugin():my(new template_plugin_impl()){}
template_plugin::~template_plugin(){}

void template_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void template_plugin::plugin_initialize(const variables_map& options) {
   if(options.count("option-name")) {
      // Handle the option
   }
}

void template_plugin::plugin_startup() {
   // Make the magic happen
}

void template_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

}
