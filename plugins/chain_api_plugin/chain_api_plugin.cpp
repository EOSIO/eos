#include <eos/chain_api_plugin/chain_api_plugin.hpp>
#include <eos/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eos {

using namespace eos;

class chain_api_plugin_impl {
public:
   chain_api_plugin_impl(database& db)
      : db(db) {}

   database& db;
};


chain_api_plugin::chain_api_plugin()
   :my(new chain_api_plugin_impl(app().get_plugin<chain_plugin>().db())) {}
chain_api_plugin::~chain_api_plugin(){}

void chain_api_plugin::set_program_options(options_description&, options_description&) {}
void chain_api_plugin::plugin_initialize(const variables_map&) {}

#define CALL(api_name, api_handle, api_namespace, call_name) \
{std::string("/v1/" #api_name "/" #call_name), [this, api_handle](string, string body, url_response_callback cb) { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(200, fc::json::to_string(result)); \
          } catch (fc::exception& e) { \
             cb(500, e.what()); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

void chain_api_plugin::plugin_startup() {
   auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();

   app().get_plugin<http_plugin>().add_api({
      CALL(chain, ro_api, chain_apis::read_only, get_info)
   });
}

void chain_api_plugin::plugin_shutdown() {}

}
