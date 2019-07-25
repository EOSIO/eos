/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/launcher_service_api_plugin/launcher_service_api_plugin.hpp>

namespace eosio {
   static appbase::abstract_plugin& _launcher_service_api_plugin = app().register_plugin<launcher_service_api_plugin>();

class launcher_service_api_plugin_impl {
   public:
};

launcher_service_api_plugin::launcher_service_api_plugin():my(new launcher_service_api_plugin_impl()){}
launcher_service_api_plugin::~launcher_service_api_plugin(){}

void launcher_service_api_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void launcher_service_api_plugin::plugin_initialize(const variables_map& options) {
   ilog("launcher_service_api_plugin::plugin_initialize()");
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
   }
   FC_LOG_AND_RETHROW()
}

int test_func(std::string s) {
   ilog("test_func");
   return 1234;
}

std::string test_func2(std::string s) {
   ilog("test_func2");
   return "OK";
}

fc::variant get_info(std::string url) {
   return app().get_plugin<launcher_service_plugin>().get_info(url);
}

int run_command(std::string cmd) {
   ilog("ready to execute \"${s}\"", ("s", cmd));
   int r = ::system(cmd.c_str());
   return r;
}

#define CALL(api_name, call_name, INVOKE, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [&](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             INVOKE \
             cb(http_response_code, fc::variant(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define INVOKE_R_R(call_name, in_param) \
     auto result = call_name(fc::json::from_string(body).as<in_param>());

#define INVOKE_V_R(api_handle, call_name, in_param) \
     call_name(fc::json::from_string(body).as<in_param>()); \
     eosio::detail::wallet_api_plugin_empty result;

void launcher_service_api_plugin::plugin_startup() {
   // Make the magic happen
   ilog("launcher_service_api_plugin::plugin_startup()");

   app().get_plugin<http_plugin>().add_api({
      CALL(launcher, test, INVOKE_R_R(test_func, std::string), 200),
      CALL(launcher, test2, INVOKE_R_R(test_func2, std::string), 200),
      CALL(launcher, get_info, INVOKE_R_R(get_info, std::string), 200),
      CALL(launcher, run_command, INVOKE_R_R(run_command, std::string), 200)
   });
}

void launcher_service_api_plugin::plugin_shutdown() {
   // OK, that's enough magic
   ilog("launcher_service_api_plugin::plugin_shutdown()");
}

#undef CALL

}
