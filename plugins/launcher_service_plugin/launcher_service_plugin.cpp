/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <string.h>
#include <signal.h> // kill()
#include <map>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <boost/process/system.hpp>
#include <boost/process/io.hpp>
#include <boost/process/child.hpp>
#include <fc/io/json.hpp>
#include <fc/io/fstream.hpp>
#include <fc/variant.hpp>

#include <eosio/launcher_service_plugin/launcher_service_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <appbase/application.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio {
   static appbase::abstract_plugin& _launcher_service_plugin = app().register_plugin<launcher_service_plugin>();

   namespace bfs = boost::filesystem;
   namespace bp = boost::process;
   namespace bpo = boost::program_options;
   using namespace chain;

   namespace launcher_service {
      action create_newaccount(const name& creator, const name& accname, public_key_type owner, public_key_type active) {
         return action {
            vector<chain::permission_level>{{creator, N(active)}},
            newaccount {
               .creator      = creator,
               .name         = accname,
               .owner        = authority(owner),
               .active       = authority(active)
            }
         };
      }
      action create_setabi(const name& account, const bytes& abi) {
         return action {
            vector<chain::permission_level>{{account, N(active)}},
            setabi{
               .account   = account,
               .abi       = abi
            }
         };
      }
      action create_setcode(const name& account, const bytes& code) {
         return action {
            vector<chain::permission_level>{{account, N(active)}},
            setcode{
               .account   = account,
               .vmtype    = 0,
               .vmversion = 0,
               .code      = code
            }
         };
      }
   }

   using namespace launcher_service;
   class launcher_service_plugin_impl {

public:
      struct node_state {
         int                        id = 0;
         int                        pid = 0;
         uint16_t                   http_port = 0;
         uint16_t                   p2p_port = 0;
         bool                       is_bios = false;
         std::string                stdout_path;
         std::string                stderr_path;
         std::shared_ptr<bp::child> child;
      };
      struct cluster_state {
         cluster_def                                 def;
         std::map<int, node_state>                   nodes; // node id => node_state
         std::map<public_key_type, private_key_type> imported_keys;

         // possible transaction block-number for query purpose
         std::map<transaction_id_type, uint32_t>     transaction_blocknum;
      };

      launcher_config                _config;
      std::map<int, cluster_state>   _running_clusters;

   public:
      static std::string cluster_to_string(int id) {
         char str[32];
         sprintf(str, "cluster%05d", (int)id);
         return str;
      }
      static std::string node_to_string(int id) {
         char str[32];
         sprintf(str, "node_%02d", (int)id);
         return str;
      }
      void create_path(bfs::path path, bool clean = false) {
         boost::system::error_code ec;
         if (!bfs::exists(path)) {
            if (!bfs::create_directories (path, ec)) {
               throw ec;
            }
         } else if (clean) {
            bfs::remove_all(path);
            if (!bfs::create_directories (path, ec)) {
               throw ec;
            }
         }
      }
      static std::string itoa(int v) {
         char str[32];
         sprintf(str, "%d", v);
         return str;
      }
      void setup_cluster(const cluster_def &def) {
         create_path(bfs::path(_config.data_dir));
         create_path(bfs::path(_config.data_dir) / cluster_to_string(def.cluster_id), true);
         for (int i = 0; i < def.node_count; ++i) {
            node_def node_config = def.get_node_def(i);
            node_state state;
            state.id = i;
            state.http_port = node_config.http_port(def.cluster_id);
            state.p2p_port = node_config.p2p_port(def.cluster_id);
            state.is_bios = node_config.is_bios();

            bfs::path node_path = bfs::path(_config.data_dir) / cluster_to_string(def.cluster_id) / node_to_string(i);
            create_path(bfs::path(node_path));

            bfs::path block_path = node_path / "blocks";
            create_path(bfs::path(block_path));

            bfs::ofstream cfg(node_path / "config.ini");
            cfg << "http-server-address = " << _config.host_name << ":" << state.http_port << "\n";
            cfg << "http-validate-host = false\n";
            cfg << "p2p-listen-endpoint = " << _config.p2p_listen_addr << ":" << state.p2p_port << "\n";
            cfg << "agent-name = " << cluster_to_string(def.cluster_id) << "_" << node_to_string(i) << "\n";

            if (def.shape == "mesh") {
               for (int peer = 0; peer < i; ++peer) {
                  cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(def.cluster_id) << "\n";
               }
            }

            if (node_config.is_bios()) {
               cfg << "enable-stale-production = true\n";
            }
            cfg << "allowed-connection = any\n";

            for (const auto &priKey: node_config.producing_keys) {
               cfg << "private-key = [\"" << std::string(priKey.get_public_key()) << "\",\""
                  << std::string(priKey) << "\"]\n";
            }

            if (node_config.producers.size()) {
               cfg << "private-key = [\"" << std::string(_config.default_key.get_public_key()) << "\",\""
                  << std::string(_config.default_key) << "\"]\n";
            }
            for (auto &p : node_config.producers) {
               cfg << "producer-name = " << p << "\n";
            }
            cfg << "plugin = eosio::net_plugin\n";
            cfg << "plugin = eosio::chain_api_plugin\n";
            cfg << "plugin = eosio::producer_api_plugin\n";

            for (const std::string &s : def.extra_configs) {
               cfg << s << "\n";
            }
            for (const std::string &s : node_config.extra_configs) {
               cfg << s << "\n";
            }
            cfg.close();
            _running_clusters[def.cluster_id].nodes[i] = state;
         }
         _running_clusters[def.cluster_id].def = def;
         _running_clusters[def.cluster_id].imported_keys[_config.default_key.get_public_key()] = _config.default_key;
      }
      void launch_nodes(const cluster_def &def, int node_id, bool restart = false, std::string extra_args = "") {
         for (int i = 0; i < def.node_count; ++i) {
            if (i != node_id && node_id != -1) {
               continue;
            }

            node_state &state = _running_clusters[def.cluster_id].nodes[i];
            node_def node_config = def.get_node_def(i);

            bfs::path node_path = bfs::path(_config.data_dir) / cluster_to_string(def.cluster_id) / node_to_string(i);

            // log file rotations.. (latest file is always stdout_0.txt & stderr_0.txt)
            bfs::path stdout_path, stderr_path;
            for (int std_count = _config.log_file_rotate_max - 1; std_count >= 0; --std_count) {
               stdout_path = node_path / (std::string("stdout_") + itoa(std_count) + ".txt");
               stderr_path = node_path / (std::string("stderr_") + itoa(std_count) + ".txt");
               bfs::path stdout_path2 = node_path / (std::string("stdout_") + itoa(std_count + 1) + ".txt");
               bfs::path stderr_path2 = node_path / (std::string("stderr_") + itoa(std_count + 1) + ".txt");
               if (boost::filesystem::exists(stdout_path)) {
                  if (std_count == _config.log_file_rotate_max - 1) {
                     boost::filesystem::remove(stdout_path);
                  } else {
                     boost::filesystem::rename(stdout_path, stdout_path2);
                  }
               }
               if (boost::filesystem::exists(stderr_path)) {
                  if (std_count == _config.log_file_rotate_max - 1) {
                     boost::filesystem::remove(stderr_path);
                  } else {
                     boost::filesystem::rename(stderr_path, stderr_path2);
                  }
               }
            }
            state.stdout_path = stdout_path.string();
            state.stderr_path = stderr_path.string();

            bfs::path pid_file_path = node_path / "pid.txt";
            bfs::ofstream pidout(pid_file_path);

            std::string cmd = _config.nodeos_cmd;
            cmd += " --config-dir=" + node_path.string();
            cmd += " --data-dir=" + node_path.string() + "/data";
            if (!restart) {
               cmd += " --genesis-json=" + _config.genesis_file;
               cmd += " --delete-all-blocks";
            }
            if (def.extra_args.length()) {
               cmd += " ";
               cmd += def.extra_args;
            }
            if (extra_args.length()) {
               cmd += " ";
               cmd += extra_args;
            }

            state.pid = 0;
            state.child.reset();

            if (node_id == -1 && node_config.dont_start) {
               continue;
            }

            ilog("start to execute:${c}...", ("c", cmd));
            state.child.reset(new bp::child(cmd, bp::std_out > stdout_path, bp::std_err > stderr_path));

            state.pid = state.child->id();
            pidout << state.child->id();
            pidout.flush();
            pidout.close();
            state.child->detach();
         }
      }
      void launch_cluster(const cluster_def &def) {
         stop_cluster(def.cluster_id);
         setup_cluster(def);
         launch_nodes(def, -1, true);
      }
      void start_node(int cluster_id, int node_id, std::string extra_args) {
         if (_running_clusters.find(cluster_id) == _running_clusters.end()) {
            throw std::string("cluster is not running");
         }
         if (_running_clusters[cluster_id].nodes.find(node_id) != _running_clusters[cluster_id].nodes.end()) {
            node_state &state = _running_clusters[cluster_id].nodes[node_id];
            if (state.child && state.child->running()) {
               throw std::string("node already running");
            }
         }
         launch_nodes(_running_clusters[cluster_id].def, node_id, false, extra_args);
      }
      void stop_node(int cluster_id, int node_id, int killsig) {
         if (_running_clusters.find(cluster_id) != _running_clusters.end()) {
            if (_running_clusters[cluster_id].nodes.find(node_id) != _running_clusters[cluster_id].nodes.end()) {
               node_state &state = _running_clusters[cluster_id].nodes[node_id];
               if (state.child) {
                  if (state.child->running()) {
                     ilog("killing pid ${p}", ("p", state.child->id()));
                     ::kill(state.child->id(), killsig);
                  }
                  if (killsig == SIGKILL || !state.child->running()) {
                     state.pid = 0;
                     state.child.reset();
                  }
               }
            }
         }
      }
      void stop_cluster(int cluster_id) {
         if (_running_clusters.find(cluster_id) != _running_clusters.end()) {
            for (auto &itr : _running_clusters[cluster_id].nodes) {
               stop_node(cluster_id, itr.first, SIGKILL);
            }
            _running_clusters.erase(cluster_id);
         }
      }
      void stop_all_clusters() {
         while (_running_clusters.size()) { // don't use ranged for loop as _running_clusters is modified
            stop_cluster(_running_clusters.begin()->first);
         }
      }

      fc::variant call(int cluster_id, int node_id, std::string func, fc::variant args = fc::variant()) {
         if (_running_clusters.find(cluster_id) == _running_clusters.end()) {
            throw std::string("cluster is not running");
         }
         if (_running_clusters[cluster_id].nodes.find(node_id) == _running_clusters[cluster_id].nodes.end()) {
            throw std::string("nodeos is not running");
         }
         int port = _running_clusters[cluster_id].nodes[node_id].http_port;
         if (port) {
            std::string url;
            url = "http://" + _config.host_name + ":" + itoa(port);
            client::http::http_context context = client::http::create_http_context();
            std::vector<std::string> headers;
            auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + func, false, headers);
            return client::http::do_http_call(*sp, args, _config.print_http_request, _config.print_http_response );
         }
         std::string err = "failed to " + func + ", nodeos is not running";
         throw err;
      }

      fc::variant get_info(int cluster_id, int node_id) {
         return call(cluster_id, node_id, "/v1/chain/get_info");
      }

      fc::variant get_account(launcher_service::get_account_param param) {
         return call(param.cluster_id, param.node_id, "/v1/chain/get_account", fc::mutable_variant_object("account_name", param.name));
      }

      fc::variant get_code_hash(launcher_service::get_account_param param) {
         return call(param.cluster_id, param.node_id, "/v1/chain/get_code_hash", fc::mutable_variant_object("account_name", param.name));
      }

      fc::variant get_block(int cluster_id, int node_id, std::string block_num_or_id) {
         return call(cluster_id, node_id, "/v1/chain/get_block", fc::mutable_variant_object("block_num_or_id", block_num_or_id));
      }

      fc::variant get_block_header_state(int cluster_id, int node_id, std::string block_num_or_id) {
         return call(cluster_id, node_id, "/v1/chain/get_block_header_state", fc::mutable_variant_object("block_num_or_id", block_num_or_id));
      }

      fc::variant get_protocol_features(int cluster_id, int node_id) {
         return call(cluster_id, node_id, "/v1/producer/get_supported_protocol_features");
      }

      fc::variant determine_required_keys(int cluster_id, int node_id, const signed_transaction& trx) {
         if (_running_clusters.find(cluster_id) == _running_clusters.end()) {
            throw std::string("cluster is not running");
         }
         std::vector<public_key_type> pub_keys;
         for (auto &key : _running_clusters[cluster_id].imported_keys) {
            pub_keys.push_back(key.first);
         }
         auto get_arg = fc::mutable_variant_object
               ("transaction", (chain::transaction)trx)
               ("available_keys", pub_keys);
         return call(cluster_id, node_id, "/v1/chain/get_required_keys", get_arg);
      }

      fc::optional<abi_serializer> abi_serializer_resolver(int cluster_id, int node_id, const name& account,
                                                           std::map<name, fc::optional<abi_serializer> > &abi_cache) {
         auto it = abi_cache.find( account );
         if ( it == abi_cache.end() ) {
            auto result = call(cluster_id, node_id, "/v1/chain/get_abi", fc::mutable_variant_object("account_name", account));
            auto abi_results = result.as<eosio::chain_apis::read_only::get_abi_results>();
            fc::optional<abi_serializer> abis;
            if( abi_results.abi.valid() ) {
               abis.emplace( *abi_results.abi, _config.abi_serializer_max_time );
            }
            abi_cache.emplace( account, abis );
            return abis;
         }
         return it->second;
      };

      bytes action_variant_to_bin(int cluster_id, int node_id, const name& account, const name& action,
                                  const fc::variant& action_args_var,
                                  std::map<name, fc::optional<abi_serializer> > &abi_cache) {
         auto abis = abi_serializer_resolver(cluster_id, node_id, account, abi_cache);
         FC_ASSERT( abis.valid(), "No ABI found for ${contract}", ("contract", account));

         auto action_type = abis->get_action_type( action );
         FC_ASSERT( !action_type.empty(), "Unknown action ${action} in contract ${contract}", ("action", action)( "contract", account ));
         return abis->variant_to_binary( action_type, action_args_var, _config.abi_serializer_max_time );
      }

      fc::variant push_transaction(int cluster_id, int node_id, signed_transaction& trx,
                                   packed_transaction::compression_type compression = packed_transaction::compression_type::none ) {
         int port = _running_clusters[cluster_id].nodes[node_id].http_port;
         std::string url = "http://" + _config.host_name + ":" + itoa(port);
         fc::variant info_ = get_info(cluster_id, node_id);
         eosio::chain_apis::read_only::get_info_results info = info_.as<eosio::chain_apis::read_only::get_info_results>();

         if (trx.signatures.size() == 0) {
            trx.expiration = info.head_block_time + fc::seconds(3);

            // Set tapos, default to last irreversible block if it's not specified by the user
            block_id_type ref_block_id = info.last_irreversible_block_id;
            trx.set_reference_block(ref_block_id);

           // if (tx_force_unique) {
           //    trx.context_free_actions.emplace_back( generate_nonce_action() );
           // }

            trx.max_cpu_usage_ms = 30;
            trx.max_net_usage_words = 50000;
            trx.delay_sec = 0;
         }

         bool has_key = false;
         fc::variant required_keys = determine_required_keys(cluster_id, node_id, trx);
         const fc::variant &keys = required_keys["required_keys"];
         for (const fc::variant &k : keys.get_array()) {
            public_key_type pub_key = public_key_type(k.as_string());
            private_key_type pri_key = _running_clusters[cluster_id].imported_keys[pub_key];
            trx.sign(pri_key, info.chain_id);
            has_key = true;
         }
         if (!has_key) {
            throw std::string("private keys are not imported");
         }
         //trx.sign(_config.default_key, info.chain_id);

         _running_clusters[cluster_id].transaction_blocknum[trx.id()] = info.head_block_num + 1;

         client::http::http_context context = client::http::create_http_context();
         std::vector<std::string> headers;
         auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/push_transaction", false, headers);
         return client::http::do_http_call(*sp, fc::variant(packed_transaction(trx, compression)),  _config.print_http_request, _config.print_http_response );
      }

      fc::variant push_actions(int cluster_id, int node_id, std::vector<chain::action>&& actions,
                               packed_transaction::compression_type compression = packed_transaction::compression_type::none ) {
         signed_transaction trx;
         trx.actions = std::forward<decltype(actions)>(actions);

         return push_transaction(cluster_id, node_id, trx, compression);
      }

      fc::variant push_actions(launcher_service::push_actions_param param) {
         std::map<name, fc::optional<abi_serializer> > cache;
         std::vector<chain::action> actlist;
         actlist.reserve(param.actions.size());
         for (const action_param &p : param.actions) {
            bytes data = action_variant_to_bin(param.cluster_id, param.node_id, p.account, p.action, p.data, cache);
            actlist.emplace_back(p.permissions, p.account, p.action, data);
         }
         return push_actions(param.cluster_id, param.node_id, std::move(actlist));
      }

      fc::variant create_bios_accounts(create_bios_accounts_param param) {
         std::vector<eosio::chain::action> actlist;
         public_key_type def_key = _config.default_key.get_public_key();
         for (auto &a : param.accounts) {
            if (a.owner == public_key_type()) {
               a.owner = def_key;
            }
            if (a.active == public_key_type()) {
               a.active = def_key;
            }
            actlist.push_back(create_newaccount(param.creator, a.name, a.owner, a.active));
         }
         return push_actions(param.cluster_id, param.node_id, std::move(actlist));
      }

      fc::variant create_account(new_account_param_ex param) {
         std::map<name, fc::optional<abi_serializer> > cache;

         // new account
         std::vector<eosio::chain::action> actlist;
         public_key_type def_key = _config.default_key.get_public_key();
         if (param.owner == public_key_type()) {
            param.owner = def_key;
         }
         if (param.active == public_key_type()) {
            param.active = def_key;
         }
         actlist.push_back(create_newaccount(param.creator, param.name, param.owner, param.active));

         // delegatebw
         fc::variant delegate_act = fc::mutable_variant_object()
                                    ("from", param.creator.to_string())
                                    ("receiver", param.name.to_string())
                                    ("stake_net_quantity", param.stake_net)
                                    ("stake_cpu_quantity", param.stake_cpu)
                                    ("transfer", param.transfer);
         actlist.emplace_back(vector<chain::permission_level>{{param.creator, N(active)}}, N(eosio), N(delegatebw),
            action_variant_to_bin(param.cluster_id, param.node_id, N(eosio), N(delegatebw), delegate_act, cache));

         // buyram
         fc::variant buyram_act = fc::mutable_variant_object()
               ("payer", param.creator.to_string())
               ("receiver", param.name.to_string())
               ("bytes", param.buy_ram_bytes);
         actlist.emplace_back(vector<chain::permission_level>{{param.creator, N(active)}}, N(eosio), N(buyrambytes),
            action_variant_to_bin(param.cluster_id, param.node_id, N(eosio), N(buyrambytes), buyram_act, cache));

         return push_actions(param.cluster_id, param.node_id, std::move(actlist));
      }

      fc::variant set_contract(set_contract_param param) {
         std::vector<eosio::chain::action> actlist;
         if (param.contract_file.length()) {
            std::string wasm;
            fc::read_file_contents(param.contract_file, wasm);
            actlist.push_back(create_setcode(param.account, bytes(wasm.begin(), wasm.end())));
         }
         if (param.abi_file.length()) {
            bytes abi_bytes = fc::raw::pack(fc::json::from_file(param.abi_file).as<abi_def>());
            actlist.push_back(create_setabi(param.account, abi_bytes));
         }
         if (actlist.size() == 0) {
            throw std::string("contract_file and abi_file are both empty");
         }
         return push_actions(param.cluster_id, param.node_id, std::move(actlist), packed_transaction::compression_type::zlib);
      }

      fc::variant import_keys(import_keys_param param) {
         if (_running_clusters.find(param.cluster_id) == _running_clusters.end()) {
            throw std::string("cluster is not running");
         }
         auto &cluster_state = _running_clusters[param.cluster_id];
         std::vector<std::string> pub_keys;
         for (auto &key : param.keys) {
            auto pkey = key.get_public_key();
            pub_keys.push_back(std::string(pkey));
            cluster_state.imported_keys[pkey] = key;
         }
         return fc::mutable_variant_object("imported_keys", pub_keys);
      }

      fc::variant generate_key(generate_key_param param) {
         if (_running_clusters.find(param.cluster_id) == _running_clusters.end()) {
            throw std::string("cluster is not running");
         }
         fc::sha256 digest(param.seed);
         private_key_type pri_key = private_key_type::regenerate(fc::ecc::private_key::generate_from_seed(digest).get_secret());
         public_key_type pub_key = pri_key.get_public_key();
         _running_clusters[param.cluster_id].imported_keys[pub_key] = pri_key;
         return fc::mutable_variant_object("generated_key", std::string(pub_key));
      }

      fc::variant verify_transaction(launcher_service::verify_transaction_param param) {
         uint32_t txn_block_num = param.block_num_hint;
         if (!txn_block_num) {
            auto itr = _running_clusters[param.cluster_id].transaction_blocknum.find(param.transaction_id);
            if (itr == _running_clusters[param.cluster_id].transaction_blocknum.end()) {
               return fc::mutable_variant_object("result", "transaction not found but block_num_hint was not given");
            }
            txn_block_num = itr->second;
         }
         fc::variant info_ = get_info(param.cluster_id, param.node_id);
         eosio::chain_apis::read_only::get_info_results info = info_.as<eosio::chain_apis::read_only::get_info_results>();
         uint32_t head_block_num = info.head_block_num;
         uint32_t lib_num = info.last_irreversible_block_num;
         uint32_t contained_blocknum = 0;
         std::string txn_id = std::string(param.transaction_id);
         for (uint32_t x = txn_block_num;
            contained_blocknum == 0 && x <= head_block_num && x < txn_block_num + param.max_search_blocks; ++x) {
            fc::variant block_ = get_block(param.cluster_id, param.node_id, itoa(x));
            if (block_.is_object()) {
               const fc::variant_object& block = block_.get_object();
               if (block.find("transactions") != block.end()) {
                  const fc::variant &txns_ = block["transactions"];
                  if (txns_.is_array()) {
                     const variants &txns = txns_.get_array();
                     for (const variant &txn : txns) {
                        const variant &trx = txn["trx"];
                        if (trx["id"] == txn_id) {
                           contained_blocknum = x;
                           break;
                        }
                     }
                  }
               }
            }
         }
         return fc::mutable_variant_object("contained", contained_blocknum ? true : false)
                                          ("irreversible", (contained_blocknum && contained_blocknum <= lib_num) ? true : false)
                                          ("contained_block_num", contained_blocknum)
                                          ("block_num_hint", txn_block_num)
                                          ("head_block_num", head_block_num)
                                          ("last_irreversible_block_num", lib_num);
      }

      fc::variant schedule_protocol_feature_activations(launcher_service::schedule_protocol_feature_activations_param param) {
         return call(param.cluster_id, param.node_id, "/v1/producer/schedule_protocol_feature_activations",
                     fc::mutable_variant_object("protocol_features_to_activate", param.protocol_features_to_activate));
      }

      fc::variant get_table_rows(launcher_service::get_table_rows_param param) {
         return call(param.cluster_id, param.node_id, "/v1/chain/get_table_rows",
                     fc::mutable_variant_object("json", param.json)
                                                ("code", param.code)
                                                ("scope", param.scope)
                                                ("table", param.table)
                                                ("lower_bound", param.lower_bound)
                                                ("upper_bound", param.upper_bound)
                                                ("limit", param.limit)
                                                ("key_type", param.key_type)
                                                ("index_position", param.index_position)
                                                ("encode_type", param.encode_type)
                                                ("reverse", param.reverse)
                                                ("show_payer", param.show_payer));
      }
   };

launcher_service_plugin::launcher_service_plugin():_my(new launcher_service_plugin_impl()){}
launcher_service_plugin::~launcher_service_plugin(){}

void launcher_service_plugin::set_program_options(options_description&, options_description& cfg) {
   std::cout << "launcher_service_plugin::set_program_options()" << std::endl;
   cfg.add_options()
         ("data-dir", bpo::value<string>()->default_value("data-dir"),
         "data directory of clusters")
         ("host-name", bpo::value<string>()->default_value("127.0.0.1"),
         "name or ip address of host")
         ("p2p-listen-address", bpo::value<string>()->default_value("0.0.0.0"),
         "listen address of incoming p2p connection of nodeos")
         ("nodeos-cmd", bpo::value<string>()->default_value("./programs/nodeos/nodeos"),
         "nodeos executable command")
         ("genesis-file", bpo::value<string>()->default_value("genesis.json"),
         "path of genesis file")
         ("abi-serializer-max-time", bpo::value<uint32_t>()->default_value(1000000),
         "abi serializer max time(in us)")
         ("log-file-max", bpo::value<uint16_t>()->default_value(8),
         "max number of stdout/stderr files per node")
         ("print-http", boost::program_options::bool_switch()->notifier([this](bool e){_my->_config.print_http_request = _my->_config.print_http_response = e;}),
         "print http request and response")
         ("default-key", bpo::value<string>()->default_value("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"),
         "default private key for cluster accounts")
         ;
}

void launcher_service_plugin::plugin_initialize(const variables_map& options) {
   std::cout << "launcher_service_plugin::plugin_initialize()" << std::endl;
   try {
      _my->_config.data_dir = options.at("data-dir").as<std::string>();
      _my->_config.host_name = options.at("host-name").as<std::string>();
      _my->_config.p2p_listen_addr = options.at("p2p-listen-address").as<std::string>();
      _my->_config.nodeos_cmd = options.at("nodeos-cmd").as<std::string>();
      _my->_config.genesis_file = options.at("genesis-file").as<std::string>();
      _my->_config.abi_serializer_max_time = fc::microseconds(options.at("abi-serializer-max-time").as<uint32_t>());
      _my->_config.log_file_rotate_max = options.at("log-file-max").as<uint16_t>();
      _my->_config.default_key = private_key_type(options.at("default-key").as<std::string>());
   }
   FC_LOG_AND_RETHROW()
}

void launcher_service_plugin::plugin_startup() {
   // Make the magic happen
   std::cout << "launcher_service_plugin::plugin_startup()" << std::endl;

   auto &httpplugin = app().get_plugin<http_plugin>();
}

void launcher_service_plugin::plugin_shutdown() {
   // OK, that's enough magic
   std::cout << "launcher_service_plugin::plugin_shutdown()" << std::endl;
   _my->stop_all_clusters();
}

fc::variant launcher_service_plugin::get_info(std::string url)
{
   try {
      client::http::http_context context = client::http::create_http_context();
      std::vector<std::string> headers;
      auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/get_info", false, headers);
      auto r = client::http::do_http_call(*sp, fc::variant(),  _my->_config.print_http_request, _my->_config.print_http_response );
      return r;
   } catch (boost::system::system_error& e) {
      return fc::mutable_variant_object("exception", e.what())("url", url);
   }
}

fc::variant launcher_service_plugin::get_cluster_info(int cluster_id)
{
   if (_my->_running_clusters.find(cluster_id) == _my->_running_clusters.end()) {
      return fc::mutable_variant_object("error", "cluster is not running");
   }
   bool print_request = false;
   bool print_response = false;
   std::map<int, fc::variant> res;
   for (auto &itr : _my->_running_clusters[cluster_id].nodes) {
      int id = itr.second.id;
      int port = itr.second.http_port;
      if (port) {
         std::string url;
         try {
            url = "http://" + _my->_config.host_name + ":" + _my->itoa(port);
            client::http::http_context context = client::http::create_http_context();
            std::vector<std::string> headers;
            auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/get_info", false, headers);
            res[id] = client::http::do_http_call(*sp, fc::variant(), _my->_config.print_http_request, _my->_config.print_http_response );
         } catch (boost::system::system_error& e) {
            res[id] = fc::mutable_variant_object("exception", e.what())("url", url);
         }
      }
   }
   return res.size() ? fc::mutable_variant_object("result", res) : fc::mutable_variant_object("error", "cluster is not running");
}

fc::variant launcher_service_plugin::get_cluster_running_state(int cluster_id)
{
   if (_my->_running_clusters.find(cluster_id) == _my->_running_clusters.end()) {
      return fc::mutable_variant_object("error", "cluster is not running");
   }
   for (auto &itr : _my->_running_clusters[cluster_id].nodes) {
      launcher_service_plugin_impl::node_state &state = itr.second;
      if (state.child && !state.child->running()) {
         state.pid = 0;
         state.child.reset();
      }
   }
   return fc::mutable_variant_object("result", _my->_running_clusters[cluster_id]);
}

#define CATCH_LAUCHER_EXCEPTIONS \
   catch (boost::system::system_error& e) { \
      return fc::mutable_variant_object("exception", e.what());\
   } catch (boost::system::error_code& e) {\
      return fc::mutable_variant_object("exception", e.message());\
   } catch (const std::string &s) {\
      return fc::mutable_variant_object("exception", s);\
   } catch (const char *s) {\
      return fc::mutable_variant_object("exception", s);\
   }

fc::variant launcher_service_plugin::launch_cluster(launcher_service::cluster_def def)
{
   try {
      _my->launch_cluster(def);
      return fc::mutable_variant_object("result", _my->_running_clusters[def.cluster_id]);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::stop_all_clusters() {
   try {
      _my->stop_all_clusters();
      return fc::mutable_variant_object("result", "OK");
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::stop_cluster(int cluster_id) {
   try {
      _my->stop_cluster(cluster_id);
      return fc::mutable_variant_object("result", "OK");
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::start_node(int cluster_id, int node_id, std::string args) {
   try {
      _my->start_node(cluster_id, node_id, args);
      return fc::mutable_variant_object("result", "OK");
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::stop_node(int cluster_id, int node_id, int killsig) {
   try {
      _my->stop_node(cluster_id, node_id, killsig);
      return fc::mutable_variant_object("result", "OK");
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::create_bios_accounts(launcher_service::create_bios_accounts_param param) {
   try {
      return _my->create_bios_accounts(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::create_account(launcher_service::new_account_param_ex param) {
   try {
      return _my->create_account(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::get_protocol_features(int cluster_id, int node_id) {
   try {
      return _my->get_protocol_features(cluster_id, node_id);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::schedule_protocol_feature_activations(launcher_service::schedule_protocol_feature_activations_param param) {
   try {
      return _my->schedule_protocol_feature_activations(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::get_block(launcher_service::get_block_param param) {
   try {
      return _my->get_block(param.cluster_id, param.node_id, param.block_num_or_id);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::get_block_header_state(launcher_service::get_block_param param) {
   try {
      return _my->get_block_header_state(param.cluster_id, param.node_id, param.block_num_or_id);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::get_account(launcher_service::get_account_param param) {
   try {
      return _my->get_account(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::get_code_hash(launcher_service::get_account_param param) {
   try {
      return _my->get_code_hash(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::get_table_rows(launcher_service::get_table_rows_param param) {
   try {
      return _my->get_table_rows(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::verify_transaction(launcher_service::verify_transaction_param param) {
   try {
      return _my->verify_transaction(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::set_contract(launcher_service::set_contract_param param) {
   try {
      return _my->set_contract(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::import_keys(launcher_service::import_keys_param param) {
   try {
      return _my->import_keys(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::generate_key(launcher_service::generate_key_param param) {
   try {
      return _my->generate_key(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::push_actions(launcher_service::push_actions_param param) {
   try {
      return _my->push_actions(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

#undef CATCH_LAUCHER_EXCEPTIONS

}

FC_REFLECT(eosio::launcher_service_plugin_impl::node_state, (id)(pid)(http_port)(p2p_port)(is_bios)(stdout_path)(stderr_path))
FC_REFLECT(eosio::launcher_service_plugin_impl::cluster_state, (nodes))
