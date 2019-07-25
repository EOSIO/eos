/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/launcher_service_plugin/launcher_service_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <appbase/application.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/io/json.hpp>

namespace eosio {
   static appbase::abstract_plugin& _launcher_service_plugin = app().register_plugin<launcher_service_plugin>();

class launcher_service_plugin_impl {
   public:
};

launcher_service_plugin::launcher_service_plugin():my(new launcher_service_plugin_impl()){}
launcher_service_plugin::~launcher_service_plugin(){}

void launcher_service_plugin::set_program_options(options_description&, options_description& cfg) {
   std::cout << "launcher_service_plugin::set_program_options()" << std::endl;
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void launcher_service_plugin::plugin_initialize(const variables_map& options) {
   std::cout << "launcher_service_plugin::plugin_initialize()" << std::endl;
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
   }
   FC_LOG_AND_RETHROW()
}

void launcher_service_plugin::plugin_startup() {
   // Make the magic happen
   std::cout << "launcher_service_plugin::plugin_startup()" << std::endl;

   auto &httpplugin = app().get_plugin<http_plugin>();
}

void launcher_service_plugin::plugin_shutdown() {
   // OK, that's enough magic
   std::cout << "launcher_service_plugin::plugin_shutdown()" << std::endl;
}

fc::variant launcher_service_plugin::get_info(std::string url)
{
   bool print_request = true;
   bool print_response = true;
   try {
      client::http::http_context context = client::http::create_http_context();
      std::vector<std::string> headers;
      auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/get_info", false, headers);
      auto r = client::http::do_http_call(*sp, fc::variant(), print_request, print_response );
      return r;
   } catch (boost::system::system_error& e) {
      return fc::mutable_variant_object("exception", e.what())("url", url);
   }
}

}
