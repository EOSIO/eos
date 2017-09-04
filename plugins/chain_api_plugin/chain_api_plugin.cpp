#include <eos/chain_api_plugin/chain_api_plugin.hpp>
#include <eos/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eos {

using namespace eos;

class chain_api_plugin_impl {
public:
   chain_api_plugin_impl(chain_controller& db)
      : db(db) {}

   chain_controller& db;
};


chain_api_plugin::chain_api_plugin(){}
chain_api_plugin::~chain_api_plugin(){}

void chain_api_plugin::set_program_options(options_description&, options_description&) {}
void chain_api_plugin::plugin_initialize(const variables_map&) {}

#define CALL(api_name, api_handle, api_namespace, call_name) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(200, fc::json::to_string(result)); \
          } catch (fc::eof_exception) { \
             cb(400, "Invalid arguments"); \
             elog("Unable to parse arguments: ${args}", ("args", body)); \
          } catch (fc::exception& e) { \
             cb(500, e.to_detail_string()); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

#define CHAIN_RO_CALL(call_name) CALL(chain, ro_api, chain_apis::read_only, call_name)
#define CHAIN_RW_CALL(call_name) CALL(chain, rw_api, chain_apis::read_write, call_name)

void chain_api_plugin::plugin_startup() {
   ilog( "starting chain_api_plugin" );
   my.reset(new chain_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
   auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
   auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();

   app().get_plugin<http_plugin>().add_api({
      CHAIN_RO_CALL(get_info),
      CHAIN_RO_CALL(get_block),
      CHAIN_RO_CALL(get_account),
      CHAIN_RO_CALL(get_code),
      CHAIN_RO_CALL(get_table_rows),
      CHAIN_RO_CALL(abi_json_to_bin),
      CHAIN_RO_CALL(abi_bin_to_json),
      CHAIN_RO_CALL(get_required_keys),
      CHAIN_RW_CALL(push_block),
      CHAIN_RW_CALL(push_transaction),
      CHAIN_RW_CALL(push_transactions)
   });
}

void chain_api_plugin::plugin_shutdown() {}

}
