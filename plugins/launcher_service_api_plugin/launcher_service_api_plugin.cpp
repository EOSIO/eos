#include <eosio/launcher_service_api_plugin/launcher_service_api_plugin.hpp>

namespace eosio {
   static appbase::abstract_plugin& _launcher_service_api_plugin = app().register_plugin<launcher_service_api_plugin>();

class launcher_service_api_plugin_impl {
   public:
};

launcher_service_api_plugin::launcher_service_api_plugin():my(new launcher_service_api_plugin_impl()){}
launcher_service_api_plugin::~launcher_service_api_plugin(){}

void launcher_service_api_plugin::set_program_options(options_description&, options_description& cfg) {
}

void launcher_service_api_plugin::plugin_initialize(const variables_map& options) {
}

#define CALL(api_name, call_name, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [&](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = app().get_plugin<launcher_service_plugin>().call_name(fc::json::from_string(body).as<call_name ## _param>()); \
             cb(http_response_code, fc::variant(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

void launcher_service_api_plugin::plugin_startup() {

   using namespace launcher_service;
   using launch_cluster_param = cluster_def;
   using stop_cluster_param = cluster_id_param;
   using clean_cluster_param = cluster_id_param;
   using stop_all_clusters_param = empty_param;
   using get_info_param = node_id_param;
   using get_block_header_state_param = get_block_param;
   using get_code_hash_param = get_account_param;
   using get_cluster_info_param = cluster_id_param;
   using get_cluster_running_state_param = cluster_id_param;
   using get_protocol_features_param = node_id_param;

   app().get_plugin<http_plugin>().add_api({
      CALL(launcher, launch_cluster, 200),
      CALL(launcher, stop_cluster, 200),
      CALL(launcher, clean_cluster, 200),
      CALL(launcher, stop_all_clusters, 200),
      CALL(launcher, stop_node, 200),
      CALL(launcher, start_node, 200),

      CALL(launcher, import_keys, 200),
      CALL(launcher, generate_key, 200),

      CALL(launcher, get_info, 200),
      CALL(launcher, get_block, 200),
      CALL(launcher, get_account, 200),
      CALL(launcher, get_code_hash, 200),
      CALL(launcher, get_cluster_info, 200),
      CALL(launcher, get_cluster_running_state, 200),
      CALL(launcher, get_protocol_features, 200),
      CALL(launcher, get_table_rows, 200),
      CALL(launcher, get_log_data, 200),

      CALL(launcher, set_contract, 200),
      CALL(launcher, push_actions, 200),
      CALL(launcher, verify_transaction, 200),
      CALL(launcher, schedule_protocol_feature_activations, 200),

      CALL(launcher, send_raw, 200)
   });
}

void launcher_service_api_plugin::plugin_shutdown() {
   ilog("launcher_service_api_plugin::plugin_shutdown()");
}

#undef CALL

}
