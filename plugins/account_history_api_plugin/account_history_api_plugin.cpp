/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/account_history_api_plugin/account_history_api_plugin.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eosio {

using namespace eosio;

static appbase::abstract_plugin& _account_history_api_plugin = app().register_plugin<account_history_api_plugin>();

account_history_api_plugin::account_history_api_plugin(){}
account_history_api_plugin::~account_history_api_plugin(){}

void account_history_api_plugin::set_program_options(options_description&, options_description&) {}
void account_history_api_plugin::plugin_initialize(const variables_map&) {}

#define CALL(api_name, api_handle, api_namespace, call_name) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(200, fc::json::to_string(result)); \
          } catch (fc::eof_exception& e) { \
             error_results results{400, "Bad Request", e}; \
             cb(400, fc::json::to_string(results)); \
             elog("Unable to parse arguments: ${args}", ("args", body)); \
          } catch (fc::exception& e) { \
             error_results results{500, "Internal Service Error", e}; \
             cb(500, fc::json::to_string(results)); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

#define CHAIN_RO_CALL(call_name) CALL(account_history, ro_api, account_history_apis::read_only, call_name)
#define CHAIN_RW_CALL(call_name) CALL(account_history, rw_api, account_history_apis::read_write, call_name)

void account_history_api_plugin::plugin_startup() {
   ilog( "starting account_history_api_plugin" );
   auto ro_api = app().get_plugin<account_history_plugin>().get_read_only_api();
   auto rw_api = app().get_plugin<account_history_plugin>().get_read_write_api();

   app().get_plugin<http_plugin>().add_api({
      CHAIN_RO_CALL(get_transaction),
      CHAIN_RO_CALL(get_transactions),
      CHAIN_RO_CALL(get_key_accounts),
      CHAIN_RO_CALL(get_controlled_accounts)
   });
}

void account_history_api_plugin::plugin_shutdown() {}

}
