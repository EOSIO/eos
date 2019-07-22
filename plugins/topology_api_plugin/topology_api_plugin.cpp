/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/topology_api_plugin/topology_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include <chrono>

namespace eosio { namespace detail {
  struct topology_api_plugin_empty {};
}}

FC_REFLECT(eosio::detail::topology_api_plugin_empty, );

namespace eosio {

static appbase::abstract_plugin& _topology_api_plugin = app().register_plugin<topology_api_plugin>();

using namespace eosio;

#define CALL(api_name, api_handle, call_name, INVOKE, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [&api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             INVOKE \
             cb(http_response_code, fc::variant(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
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
     eosio::detail::topology_api_plugin_empty result;

#define INVOKE_V_R_R(api_handle, call_name, in_param0, in_param1) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>()); \
     eosio::detail::topology_api_plugin_empty result;

#define INVOKE_V_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2)    \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>(), vs.at(2).as<in_param2>()); \
     eosio::detail::topology_api_plugin_empty result;

#define INVOKE_V_R_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2, in_param3) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>(), vs.at(2).as<in_param2>(), vs.at(3).as<in_param3>() ); \
     eosio::detail::topology_api_plugin_empty result;

#define INVOKE_V_V(api_handle, call_name) \
     api_handle.call_name(); \
     eosio::detail::topology_api_plugin_empty result;


void topology_api_plugin::plugin_startup() {
   ilog("starting topology_api_plugin");
   // lifetime of plugin is lifetime of application
   auto& topo_mgr = app().get_plugin<topology_plugin>();

   app().get_plugin<http_plugin>().add_api({
       CALL(topology, topo_mgr, nodes,
            INVOKE_R_R(topo_mgr, nodes, std::string), 201),
       CALL(topology, topo_mgr, links,
            INVOKE_R_R(topo_mgr, links, std::string), 201),
       CALL(topology, topo_mgr, gen_grid,
            INVOKE_R_V(topo_mgr, gen_grid), 201),
       CALL(topology, topo_mgr, get_sample,
            INVOKE_R_V(topo_mgr, get_sample), 201)
   });
}

void topology_api_plugin::plugin_initialize(const variables_map& options) {
   try {
      const auto& _http_plugin = app().get_plugin<http_plugin>();
      if( !_http_plugin.is_on_loopback()) {
         ilog( "Topology Monitor API started\n" );
      }
   } FC_LOG_AND_RETHROW()
}


#undef INVOKE_R_R
#undef INVOKE_R_R_R_R
#undef INVOKE_R_V
#undef INVOKE_V_R
#undef INVOKE_V_R_R
#undef INVOKE_V_R_R_R
#undef INVOKE_V_R_R_R_R
#undef INVOKE_V_V
#undef CALL

}
