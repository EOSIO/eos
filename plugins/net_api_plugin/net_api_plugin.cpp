#include <eosio/net_api_plugin/net_api_plugin.hpp>
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

#define CALL_WITH_400(api_name, api_handle, call_name, INVOKE, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [&api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             INVOKE \
             cb(http_response_code, fc::variant(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define INVOKE_R_R(api_handle, call_name, in_param) \
     auto params = parse_params<in_param, http_params_types::params_required>(body);\
     fc::variant result( api_handle.call_name( std::move(params) ) );

#define INVOKE_R_V(api_handle, call_name) \
     body = parse_params<std::string, http_params_types::no_params_required>(body); \
     auto result = api_handle.call_name();

#define INVOKE_V_R(api_handle, call_name, in_param) \
     auto params = parse_params<in_param, http_params_types::params_required>(body);\
     api_handle.call_name( std::move(params) ); \
     eosio::detail::net_api_plugin_empty result;

#define INVOKE_V_V(api_handle, call_name) \
     body = parse_params<std::string, http_params_types::no_params_required>(body); \
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
       CALL_WITH_400(net, net_mgr, connect,
            INVOKE_R_R(net_mgr, connect, std::string), 201),
       CALL_WITH_400(net, net_mgr, disconnect,
            INVOKE_R_R(net_mgr, disconnect, std::string), 201),
       CALL_WITH_400(net, net_mgr, status,
            INVOKE_R_R(net_mgr, status, std::string), 201),
       CALL_WITH_400(net, net_mgr, connections,
            INVOKE_R_V(net_mgr, connections), 201),
    //   CALL(net, net_mgr, open,
    //        INVOKE_V_R(net_mgr, open, std::string), 200),
   }, appbase::priority::medium_high);
}

void net_api_plugin::plugin_initialize(const variables_map& options) {
   try {
      const auto& _http_plugin = app().get_plugin<http_plugin>();
      if( !_http_plugin.is_on_loopback()) {
         wlog( "\n"
               "**********SECURITY WARNING**********\n"
               "*                                  *\n"
               "* --         Net API            -- *\n"
               "* - EXPOSED to the LOCAL NETWORK - *\n"
               "* - USE ONLY ON SECURE NETWORKS! - *\n"
               "*                                  *\n"
               "************************************\n" );
      }
   } FC_LOG_AND_RETHROW()
}


#undef INVOKE_R_R
#undef INVOKE_R_V
#undef INVOKE_V_R
#undef INVOKE_V_V
#undef CALL

}
