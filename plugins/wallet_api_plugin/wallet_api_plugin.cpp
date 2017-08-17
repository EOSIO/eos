#include <eos/wallet_api_plugin/wallet_api_plugin.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/transaction.hpp>

#include <fc/io/json.hpp>

namespace eos {

using namespace eos;

#define CALL(api_name, api_handle, in_param, call_name) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<in_param>()); \
             cb(200, fc::json::to_string(result)); \
          } catch (fc::eof_exception) { \
             cb(400, "Invalid arguments"); \
             elog("Unable to parse arguments: ${args}", ("args", body)); \
          } catch (fc::exception& e) { \
             cb(500, e.to_detail_string()); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

#define RO_CALL(call_name, in_param) CALL(wallet, ro_api, in_param, call_name)
#define RW_CALL(call_name, in_param) CALL(wallet, rw_api, in_param, call_name)

void wallet_api_plugin::plugin_startup() {
   ilog("starting wallet_api_plugin");
   auto ro_api = app().get_plugin<wallet_plugin>().get_read_only_api();
   auto rw_api = app().get_plugin<wallet_plugin>().get_read_write_api();

   app().get_plugin<http_plugin>().add_api({
      RW_CALL(sign_transaction, chain::Transaction)
   });
}

#undef CALL
#undef RW_CALL
#undef RO_CALL

}
