/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/net_api_plugin/net_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include <chrono>

namespace eosio { namespace detail {
  struct net_api_plugin_empty {};
}}

FC_REFLECT(eosio::detail::net_api_plugin_empty, );

namespace eosio {

static appbase::abstract_plugin& _net_api_plugin = app().register_plugin<net_api_plugin>();

using namespace eosio;

#define CALL(api_name, api_handle, call_name, INVOKE, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [&api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             INVOKE \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (fc::eof_exception& e) { \
             error_results results{400, "Bad Request", e.to_string()}; \
             cb(400, fc::json::to_string(results)); \
             elog("Unable to parse arguments: ${args}", ("args", body)); \
          } catch (fc::exception& e) { \
             error_results results{500, "Internal Service Error", e.to_detail_string()}; \
             cb(500, fc::json::to_string(results)); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

#define INVOKE_R_R(api_handle, call_name, in_param) \
     auto result = api_handle.call_name(fc::json::from_string(body).as<in_param>());

#define INVOKE_R_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     auto result = api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>(), vs.at(2).as<in_param2>());

#define INVOKE_R_V(api_handle, call_name) \
     auto result = api_handle.call_name();

#define INVOKE_V_R(api_handle, call_name, in_param) \
     api_handle.call_name(fc::json::from_string(body).as<in_param>()); \
     eosio::detail::net_api_plugin_empty result;

#define INVOKE_V_R_R(api_handle, call_name, in_param0, in_param1) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>()); \
     eosio::detail::net_api_plugin_empty result;

#define INVOKE_V_V(api_handle, call_name) \
     api_handle.call_name(); \
     eosio::detail::net_api_plugin_empty result;


void net_api_plugin::plugin_startup() {
   ilog("starting net_api_plugin");
   // lifetime of plugin is lifetime of application
   auto& net_mgr = app().get_plugin<net_plugin>();

   app().get_plugin<http_plugin>().add_api({
    //   CALL(net, net_mgr, set_timeout,
    //        INVOKE_V_R(net_mgr, set_timeout, int64_t), 200),
    //   CALL(net, net_mgr, sign_transaction,
    //        INVOKE_R_R_R_R(net_mgr, sign_transaction, chain::signed_transaction, flat_set<public_key_type>, chain::chain_id_type), 201),
       CALL(net, net_mgr, connect,
            INVOKE_R_R(net_mgr, connect, std::string), 201),
       CALL(net, net_mgr, disconnect,
            INVOKE_R_R(net_mgr, disconnect, std::string), 201),
       CALL(net, net_mgr, status,
            INVOKE_R_R(net_mgr, status, std::string), 201),
       CALL(net, net_mgr, connections,
            INVOKE_R_V(net_mgr, connections), 201),
    //   CALL(net, net_mgr, open,
    //        INVOKE_V_R(net_mgr, open, std::string), 200),
   });
}

void net_api_plugin::plugin_initialize(const variables_map& options) {
   if (options.count("http-server-address")) {
      const auto& lipstr = options.at("http-server-address").as<string>();
      const auto& host = lipstr.substr(0, lipstr.find(':'));
      if (host != "localhost" && host != "127.0.0.1") {
         wlog("\n"
              "*************************************\n"
              "*                                   *\n"
              "*  --  Net API NOT on localhost  -- *\n"
              "*                                   *\n"
              "*   this may be abused if exposed   *\n"
              "*                                   *\n"
              "*************************************\n");
      }
   }
}


#undef INVOKE_R_R
#undef INVOKE_R_R_R_R
#undef INVOKE_R_V
#undef INVOKE_V_R
#undef INVOKE_V_R_R
#undef INVOKE_V_V
#undef CALL

}
