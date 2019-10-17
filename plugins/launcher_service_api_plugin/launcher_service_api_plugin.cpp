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

#define INVOKE_R(call_name) \
     auto result = app().get_plugin<launcher_service_plugin>().call_name();

#define INVOKE_R_R(call_name, in_param) \
     auto result = app().get_plugin<launcher_service_plugin>().call_name(fc::json::from_string(body).as<in_param>());

#define INVOKE_R_R_R(call_name, in_param0, in_param1) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     auto result = app().get_plugin<launcher_service_plugin>().call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>());

#define INVOKE_R_R_R_R(call_name, in_param0, in_param1, in_param2) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     auto result = app().get_plugin<launcher_service_plugin>().call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>(), vs.at(2).as<in_param2>());

#define INVOKE_V_R(call_name, in_param) \
     app().get_plugin<launcher_service_plugin>().call_name(fc::json::from_string(body).as<in_param>()); \
     eosio::detail::wallet_api_plugin_empty result;

void launcher_service_api_plugin::plugin_startup() {
   app().get_plugin<http_plugin>().add_api({
      CALL(launcher, launch_cluster, INVOKE_R_R(launch_cluster, launcher_service::cluster_def), 200),
      CALL(launcher, stop_cluster, INVOKE_R_R(stop_cluster, launcher_service::cluster_id_param), 200),
      CALL(launcher, clean_cluster, INVOKE_R_R(clean_cluster, launcher_service::cluster_id_param), 200),
      CALL(launcher, stop_all_clusters, INVOKE_R(stop_all_clusters), 200),
      CALL(launcher, stop_node, INVOKE_R_R(stop_node, launcher_service::stop_node_param), 200),
      CALL(launcher, start_node, INVOKE_R_R(start_node, launcher_service::start_node_param), 200),

      CALL(launcher, import_keys, INVOKE_R_R(import_keys, launcher_service::import_keys_param), 200),
      CALL(launcher, generate_key, INVOKE_R_R(generate_key, launcher_service::generate_key_param), 200),

      CALL(launcher, get_info, INVOKE_R_R(get_info, launcher_service::node_id_param), 200),
      CALL(launcher, get_block, INVOKE_R_R(get_block, launcher_service::get_block_param), 200),
      CALL(launcher, get_block_header_state, INVOKE_R_R(get_block_header_state, launcher_service::get_block_param), 200),
      CALL(launcher, get_account, INVOKE_R_R(get_account, launcher_service::get_account_param), 200),
      CALL(launcher, get_code_hash, INVOKE_R_R(get_code_hash, launcher_service::get_account_param), 200),
      CALL(launcher, get_cluster_info, INVOKE_R_R(get_cluster_info, launcher_service::cluster_id_param), 200),
      CALL(launcher, get_cluster_running_state, INVOKE_R_R(get_cluster_running_state, launcher_service::cluster_id_param), 200),
      CALL(launcher, get_protocol_features, INVOKE_R_R(get_protocol_features, launcher_service::node_id_param), 200),
      CALL(launcher, get_table_rows, INVOKE_R_R(get_table_rows, launcher_service::get_table_rows_param), 200),
      CALL(launcher, get_log_data, INVOKE_R_R(get_log_data, launcher_service::get_log_data_param), 200),

      CALL(launcher, create_bios_accounts, INVOKE_R_R(create_bios_accounts, launcher_service::create_bios_accounts_param), 200),
      CALL(launcher, create_account, INVOKE_R_R(create_account, launcher_service::new_account_param_ex), 200),
      CALL(launcher, link_auth, INVOKE_R_R(link_auth, launcher_service::link_auth_param), 200),
      CALL(launcher, unlink_auth, INVOKE_R_R(unlink_auth, launcher_service::unlink_auth_param), 200),
      CALL(launcher, set_contract, INVOKE_R_R(set_contract, launcher_service::set_contract_param), 200),
      CALL(launcher, push_actions, INVOKE_R_R(push_actions, launcher_service::push_actions_param), 200),
      CALL(launcher, verify_transaction, INVOKE_R_R(verify_transaction, launcher_service::verify_transaction_param), 200),
      CALL(launcher, schedule_protocol_feature_activations, INVOKE_R_R(schedule_protocol_feature_activations, launcher_service::schedule_protocol_feature_activations_param), 200),

      CALL(launcher, send_raw, INVOKE_R_R(send_raw, launcher_service::send_raw_param), 200)
   });
}

void launcher_service_api_plugin::plugin_shutdown() {
   ilog("launcher_service_api_plugin::plugin_shutdown()");
}

#undef CALL

}
