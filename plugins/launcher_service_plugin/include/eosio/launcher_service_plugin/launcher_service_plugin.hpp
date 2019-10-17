/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <string>
#include <vector>
#include <appbase/application.hpp>
#include <eosio/chain/contract_types.hpp>
#include <eosio/launcher_service_plugin/httpc.hpp>

#include <fc/static_variant.hpp>
#include <fc/crypto/private_key.hpp>
#include <fc/crypto/public_key.hpp>
#include <fc/log/logger_config.hpp>

using public_key_type = fc::crypto::public_key;
using private_key_type = fc::crypto::private_key;

namespace eosio {

using namespace appbase;

namespace launcher_service {

   // TODO
   struct remote_launcher_node {
      std::string                    launcher_service_id;
      std::string                    launcher_service_url;
   };

   struct launcher_config {
      std::string                    launcher_service_id;   // TODO: later

      uint16_t                       base_port;
      uint16_t                       cluster_span;
      uint16_t                       node_span;
      uint16_t                       max_nodes_per_cluster;
      uint16_t                       max_clusters;

      std::string                    data_dir;
      std::string                    host_name;
      std::string                    p2p_listen_addr;
      std::string                    nodeos_cmd;
      std::string                    genesis_file;
      fc::microseconds               abi_serializer_max_time;
      uint16_t                       log_file_rotate_max = 8;
      bool                           print_http_request = false;
      bool                           print_http_response = false;
      private_key_type               default_key;
   };

   struct node_def {

      int                            node_id = 0;
      bool                           dont_start = false;
      std::vector<string>            producers;
      std::vector<private_key_type>  producing_keys;
      std::vector<string>            extra_configs;

      uint16_t                       assigned_http_port = 0;
      uint16_t                       assigned_p2p_port = 0;

      uint16_t http_port(const launcher_config &config, int cluster_id) const {
         if (assigned_http_port) {
            return assigned_http_port;
         } else {
            return config.base_port + cluster_id * config.cluster_span + node_id * config.node_span;
         }
      }
      uint16_t p2p_port(const launcher_config &config, int cluster_id) const {
         if (assigned_p2p_port) {
            return assigned_p2p_port;
         } else {
            return http_port(config, cluster_id) + 1;
         }
      }
      bool is_bios() const {
         for (auto &p: producers) {
            if (p == "eosio") return true;
         }
         return false;
      }
   };

   struct cluster_def {
      std::string                    shape = "mesh"; // "mesh" / "star" / "bridge" / "line" / "ring" / "tree"
      int                            center_node_id = -1; // required if shape is "start" or "bridge"
      int                            cluster_id = 0;
      int                            node_count = 0;
      bool                           auto_port = true; // auto port assignment
      std::vector<node_def>          nodes;
      std::vector<string>            extra_configs;
      std::string                    extra_args;
      fc::log_level                  log_level = fc::log_level::info;
      std::map<std::string, fc::log_level> special_log_levels;

      bool                           _normalized = false;

      node_def &get_node_def(int id) {
         if (!_normalized) {
            for (int i = 0; i < node_count; ++i) { // normalize
               bool found = false;
               for (auto &n: nodes) {
                  if (n.node_id == i) {
                     found = true;
                     break;
                  }
               }
               if (found) continue;
               node_def n;
               n.node_id = i;
               nodes.push_back(n);
            }
            _normalized = true;
         }
         for (auto &n: nodes) {
            if (n.node_id == id) return n;
         }
         throw std::runtime_error("node config not found");
      }
   };

   struct new_account_param {
      chain::name                    name;
      public_key_type                owner;
      public_key_type                active;
   };
   struct create_bios_accounts_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      chain::name                    creator;
      std::vector<new_account_param> accounts;
   };
   struct link_auth_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      chain::name                    account;
      chain::name                    code;
      chain::name                    type;
      chain::name                    requirement;
   };
   struct unlink_auth_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      chain::name                    account;
      chain::name                    code;
      chain::name                    type;
   };
   struct new_account_param_ex { // newaccount in system contract
      int                            cluster_id = 0;
      int                            node_id = 0;
      chain::name                    creator;
      chain::name                    name;
      public_key_type                owner;
      public_key_type                active;
      std::string                    stake_cpu;
      std::string                    stake_net;
      uint32_t                       buy_ram_bytes = 8192;
      bool                           transfer = false;
   };

   struct get_block_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      std::string                    block_num_or_id;
   };

   struct get_account_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      chain::name                    name;
   };

   struct get_log_data_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      int64_t                        offset = 0; // negative -> offset from the end.
      int64_t                        len = 0;
      std::string                    filename;
   };

   struct set_contract_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      chain::name                    account;
      std::string                    contract_file; // file path
      std::string                    abi_file; // file path
   };

   struct import_keys_param {
      int                            cluster_id = 0;
      std::vector<private_key_type>  keys;
   };

   struct generate_key_param {
      int                            cluster_id = 0;
      std::string                    seed;
   };

   struct action_param {
      chain::name                     account;
      chain::name                     action;
      vector<chain::permission_level> permissions;
      fc::variant                     data;
   };

   struct push_actions_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      std::vector<action_param>      actions;
      std::vector<public_key_type>   sign_keys; // skip get_required_keys and use corresponding private keys to sign
   };

   struct verify_transaction_param {
      int                            cluster_id = 0;
      int                            node_id = 0;
      chain::transaction_id_type     transaction_id;
      int                            max_search_blocks = 7; // default expiration = 3s
      uint32_t                       block_num_hint = 0; // required if transaction was not pushed by launcher_service
   };

   struct schedule_protocol_feature_activations_param {
      int                             cluster_id = 0;
      int                             node_id = 0;
      std::vector<chain::digest_type> protocol_features_to_activate;
   };

   struct get_table_rows_param {
      int              cluster_id = 0;
      int              node_id = 0;
      bool             json = true;
      chain::name      code;
      std::string      scope;
      chain::name      table;
      std::string      lower_bound;
      std::string      upper_bound;
      uint32_t         limit = 10;
      std::string      key_type;  // type of key specified by index_position
      std::string      index_position = "1"; // 1 - primary (first), 2 - secondary index (in order defined by multi_index), 3 - third index, etc
      std::string      encode_type{"dec"}; //dec, hex , default=dec
      bool             reverse = false;
      bool             show_payer = false; // show RAM pyer
    };

   struct cluster_id_param {
      int              cluster_id = 0;
   };
   struct node_id_param {
      int              cluster_id = 0;
      int              node_id = 0;
   };

   struct start_node_param {
      int              cluster_id = 0;
      int              node_id = 0;
      std::string      extra_args;
   };
   struct stop_node_param {
      int              cluster_id = 0;
      int              node_id = 0;
      int              kill_sig = 15;
   };

   struct send_raw_param { // send abitrary string or json data to one node
      int              cluster_id = 0;
      int              node_id = 0;
      std::string      url; // suburl of endpoint
      std::string      string_data; // either string or json
      fc::variant      json_data;
   };
}

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class launcher_service_plugin : public appbase::plugin<launcher_service_plugin> {
public:
   launcher_service_plugin();
   virtual ~launcher_service_plugin();

   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   // cluster related calls
   fc::variant launch_cluster(launcher_service::cluster_def cluster_def);
   fc::variant stop_all_clusters();
   fc::variant stop_cluster(launcher_service::cluster_id_param);
   fc::variant clean_cluster(launcher_service::cluster_id_param);
   fc::variant start_node(launcher_service::start_node_param);
   fc::variant stop_node(launcher_service::stop_node_param);

   // wallet related calls
   fc::variant generate_key(launcher_service::generate_key_param);
   fc::variant import_keys(launcher_service::import_keys_param);

   // queries
   fc::variant get_info(launcher_service::node_id_param);
   fc::variant get_block(launcher_service::get_block_param);
   fc::variant get_block_header_state(launcher_service::get_block_param);
   fc::variant get_account(launcher_service::get_account_param);
   fc::variant get_code_hash(launcher_service::get_account_param);
   fc::variant get_cluster_info(launcher_service::cluster_id_param);
   fc::variant get_cluster_running_state(launcher_service::cluster_id_param);
   fc::variant get_protocol_features(launcher_service::node_id_param);
   fc::variant verify_transaction(launcher_service::verify_transaction_param);
   fc::variant get_table_rows(launcher_service::get_table_rows_param);
   fc::variant get_log_data(launcher_service::get_log_data_param);

   // transactions
   fc::variant create_bios_accounts(launcher_service::create_bios_accounts_param);
   fc::variant create_account(launcher_service::new_account_param_ex);
   fc::variant link_auth(launcher_service::link_auth_param);
   fc::variant unlink_auth(launcher_service::unlink_auth_param);
   fc::variant set_contract(launcher_service::set_contract_param);
   fc::variant push_actions(launcher_service::push_actions_param);
   fc::variant schedule_protocol_feature_activations(launcher_service::schedule_protocol_feature_activations_param);

   // miscellaneous
   fc::variant send_raw(launcher_service::send_raw_param);

   // for unit-test
   static std::string generate_node_config(const launcher_service::launcher_config &_config, launcher_service::cluster_def &def, int node_id);

private:
   std::unique_ptr<class launcher_service_plugin_impl>  _my;
};

}

FC_REFLECT(eosio::launcher_service::node_def, (node_id)(producers)(producing_keys)(extra_configs)(dont_start) )
FC_REFLECT(eosio::launcher_service::cluster_def, (shape)(center_node_id)(cluster_id)(node_count)(auto_port)(nodes)(extra_configs)(extra_args)(log_level)(special_log_levels) )
FC_REFLECT(eosio::launcher_service::new_account_param, (name)(owner)(active))
FC_REFLECT(eosio::launcher_service::create_bios_accounts_param, (cluster_id)(node_id)(creator)(accounts))
FC_REFLECT(eosio::launcher_service::link_auth_param, (cluster_id)(node_id)(account)(code)(type)(requirement))
FC_REFLECT(eosio::launcher_service::unlink_auth_param, (cluster_id)(node_id)(account)(code)(type))
FC_REFLECT(eosio::launcher_service::new_account_param_ex, (cluster_id)(node_id)(creator)(name)(owner)(active)(stake_cpu)(stake_net)(buy_ram_bytes)(transfer))
FC_REFLECT(eosio::launcher_service::get_block_param, (cluster_id)(node_id)(block_num_or_id))
FC_REFLECT(eosio::launcher_service::get_account_param, (cluster_id)(node_id)(name))
FC_REFLECT(eosio::launcher_service::set_contract_param, (cluster_id)(node_id)(account)(contract_file)(abi_file))
FC_REFLECT(eosio::launcher_service::import_keys_param, (cluster_id)(keys))
FC_REFLECT(eosio::launcher_service::generate_key_param, (cluster_id)(seed))
FC_REFLECT(eosio::launcher_service::action_param, (account)(action)(permissions)(data))
FC_REFLECT(eosio::launcher_service::push_actions_param, (cluster_id)(node_id)(actions)(sign_keys))
FC_REFLECT(eosio::launcher_service::verify_transaction_param, (cluster_id)(node_id)(transaction_id)(max_search_blocks)(block_num_hint))
FC_REFLECT(eosio::launcher_service::schedule_protocol_feature_activations_param, (cluster_id)(node_id)(protocol_features_to_activate))
FC_REFLECT(eosio::launcher_service::get_table_rows_param, (cluster_id)(node_id)(json)(code)(scope)(table)(lower_bound)(upper_bound)(limit)(key_type)(index_position)(encode_type)(reverse)(show_payer))
FC_REFLECT(eosio::launcher_service::get_log_data_param, (cluster_id)(node_id)(offset)(len)(filename))
FC_REFLECT(eosio::launcher_service::cluster_id_param, (cluster_id))
FC_REFLECT(eosio::launcher_service::node_id_param, (cluster_id)(node_id))
FC_REFLECT(eosio::launcher_service::start_node_param, (cluster_id)(node_id)(extra_args))
FC_REFLECT(eosio::launcher_service::stop_node_param, (cluster_id)(node_id)(kill_sig))
FC_REFLECT(eosio::launcher_service::send_raw_param, (cluster_id)(node_id)(url)(string_data)(json_data))
