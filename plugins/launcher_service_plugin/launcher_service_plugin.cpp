/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <string.h>
#include <signal.h> // kill()
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <boost/process/system.hpp>
#include <boost/process/io.hpp>
#include <boost/process/child.hpp>
#include <fc/io/json.hpp>
#include <fc/io/fstream.hpp>
#include <fc/variant.hpp>
#include <fc/crypto/base64.hpp>

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

         std::map<name, fc::optional<abi_serializer> >  abi_cache;
         fc::time_point                                 abi_cache_time;

         eosio::chain_apis::read_only::get_info_results get_info_cache;
         fc::time_point                                 get_info_cache_time;
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

      uint16_t next_avail_port(uint16_t startport, uint16_t maxport) {
         boost::asio::io_service ios;
         while (startport < maxport) {
            boost::asio::ip::tcp::socket socket(ios);
            boost::system::error_code ec;
            socket.open(boost::asio::ip::tcp::v4());
            socket.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), startport), ec);
            socket.close();
            if (!ec) return startport;
            socket.close();
            ++startport;
         }
         throw std::runtime_error("failed to assign available ports");
      }

      // separate config generation logic to make unitest possible
      static std::string generate_node_config(const launcher_config &_config, cluster_def &def, int node_id) {
         std::stringstream cfg;

         const node_def &node_config = def.get_node_def(node_id);

         cfg << "http-server-address = " << _config.host_name << ":" << node_config.http_port(_config, def.cluster_id) << "\n";
         cfg << "http-validate-host = false\n";
         cfg << "p2p-listen-endpoint = " << _config.p2p_listen_addr << ":" << node_config.p2p_port(_config, def.cluster_id) << "\n";
         cfg << "p2p-max-nodes-per-host = " << def.node_count << "\n";
         cfg << "agent-name = " << cluster_to_string(def.cluster_id) << "_" << node_to_string(node_id) << "\n";

         if (def.shape == "mesh") {
            for (int peer = 0; peer < node_id; ++peer) {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id) << "\n";
            }
         } else if (def.shape == "star") {
            int peer = def.center_node_id;
            if (peer < 0 || peer >= def.node_count) {
               throw std::runtime_error("invalid center_node_id for star topology");
            }
            if (node_id != peer) {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id) << "\n";
            }
         } else if (def.shape == "bridge") {
            if (def.center_node_id <= 0 || def.center_node_id >= def.node_count - 1) {
               throw std::runtime_error("invalid center_node_id for bridge topology");
            }
            if (node_id < def.center_node_id) {
               for (int peer = 0; peer < node_id; ++peer) {
                  cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id) << "\n";
               }
            } else if (node_id > def.center_node_id) {
               for (int peer = def.center_node_id + 1; peer < node_id; ++peer) {
                  cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id) << "\n";
               }
            } else {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(node_id - 1).p2p_port(_config, def.cluster_id) << "\n";
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(node_id + 1).p2p_port(_config, def.cluster_id) << "\n";
            }
         } else if (def.shape == "line") {
            if (node_id) {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(node_id - 1).p2p_port(_config, def.cluster_id) << "\n";
            }
         } else if (def.shape == "ring") {
            cfg << "p2p-peer-address = " << _config.host_name << ":"
                  << def.get_node_def((node_id - 1 + def.node_count) % def.node_count).p2p_port(_config, def.cluster_id) << "\n";
         } else if (def.shape == "tree") {
            if (node_id) {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def((node_id + 1) / 2 - 1).p2p_port(_config, def.cluster_id) << "\n";
            }
         } else {
            throw std::runtime_error("invalid shape");
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
         return cfg.str();
      }

      void setup_cluster(cluster_def &def) {
         create_path(bfs::path(_config.data_dir));
         create_path(bfs::path(_config.data_dir) / cluster_to_string(def.cluster_id), true);
         if (def.node_count <= 0 || def.node_count > _config.max_nodes_per_cluster) {
            throw std::runtime_error("invalid node_count");
         }

         std::string logging_json;
         if (def.log_level != fc::log_level::off || def.special_log_levels.size()) {
            fc::logging_config log_config;

            log_config.appenders.emplace_back("stderr", "console", fc::mutable_variant_object("stream", "std_error"));
            log_config.appenders.emplace_back("stdout", "console", fc::mutable_variant_object("stream", "std_out"));

            fc::logger_config def_logger("default");
            def_logger.level = def.log_level;
            def_logger.appenders.push_back("stderr");
            log_config.loggers.push_back(def_logger);

            for (auto& itr : def.special_log_levels) {
               fc::logger_config logger(itr.first);
               logger.level = itr.second;
               logger.appenders.push_back("stderr");
               log_config.loggers.push_back(logger);
            }
            logging_json = json::to_pretty_string(log_config);
         }

         // port assignment
         if (def.auto_port) {
            uint16_t begin_port = _config.base_port + def.cluster_id * _config.cluster_span;
            uint16_t max_port = begin_port + _config.cluster_span;
            for (int i = 0; i < def.node_count; ++i) {
               node_def &node_config = def.get_node_def(i);
               begin_port = next_avail_port(begin_port, max_port);
               node_config.assigned_http_port = begin_port;
               ++begin_port;
               begin_port = next_avail_port(begin_port, max_port);
               node_config.assigned_p2p_port = begin_port;
               ++begin_port;
            }
         }

         for (int i = 0; i < def.node_count; ++i) {
            node_def &node_config = def.get_node_def(i);
            node_state state;
            state.id = i;
            state.http_port = node_config.http_port(_config, def.cluster_id);
            state.p2p_port = node_config.p2p_port(_config, def.cluster_id);
            state.is_bios = node_config.is_bios();

            bfs::path node_path = bfs::path(_config.data_dir) / cluster_to_string(def.cluster_id) / node_to_string(i);
            create_path(bfs::path(node_path));

            bfs::path block_path = node_path / "blocks";
            create_path(bfs::path(block_path));

            if (logging_json.length()) {
               bfs::ofstream logging_json_os(node_path / "logging.json");
               logging_json_os << logging_json;
               logging_json_os.close();
            }

            bfs::ofstream cfg(node_path / "config.ini");
            cfg << generate_node_config(_config, def, i);
            cfg.close();
            _running_clusters[def.cluster_id].nodes[i] = state;
         }
         _running_clusters[def.cluster_id].def = def;
         _running_clusters[def.cluster_id].imported_keys[_config.default_key.get_public_key()] = _config.default_key;
      }

      void launch_nodes(cluster_def &def, int node_id, bool restart = false, std::string extra_args = "") {
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
      void launch_cluster(cluster_def &def) {
         if (def.cluster_id < 0 || def.cluster_id >= _config.max_clusters) {
            throw std::runtime_error("invalid cluster id");
         }
         stop_cluster(def.cluster_id);
         setup_cluster(def);
         launch_nodes(def, -1, true);
      }
      void start_node(int cluster_id, int node_id, std::string extra_args) {
         if (_running_clusters.find(cluster_id) == _running_clusters.end()) {
            throw std::runtime_error("cluster is not running");
         }
         if (_running_clusters[cluster_id].nodes.find(node_id) != _running_clusters[cluster_id].nodes.end()) {
            node_state &state = _running_clusters[cluster_id].nodes[node_id];
            if (state.child && state.child->running()) {
               throw std::runtime_error("node already running");
            }
         }
         launch_nodes(_running_clusters[cluster_id].def, node_id, true, extra_args);
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

      void clean_cluster(int cluster_id) {
         if (_running_clusters.find(cluster_id) != _running_clusters.end()) {
            throw std::runtime_error("cluster is still running");
         }
         bfs::remove_all(bfs::path(_config.data_dir) / cluster_to_string(cluster_id));
      }

      template <typename T>
      fc::variant call_(int cluster_id, int node_id, const std::string &func, const T &args, bool silence) {
         if (_running_clusters.find(cluster_id) == _running_clusters.end()) {
            throw std::runtime_error("cluster is not running");
         }
         if (_running_clusters[cluster_id].nodes.find(node_id) == _running_clusters[cluster_id].nodes.end()) {
            throw std::runtime_error("nodeos is not running");
         }
         int port = _running_clusters[cluster_id].nodes[node_id].http_port;
         if (port) {
            client::http::parsed_url purl;
            purl.scheme = "http";
            purl.server = _config.host_name;
            purl.port = itoa(port);
            purl.path = func;
            client::http::http_context context = client::http::create_http_context();
            std::vector<std::string> headers;
            auto sp = std::make_unique<client::http::connection_param>(context, purl, false, headers);
            return client::http::do_http_call(*sp, args, silence ? false : _config.print_http_request, silence ? false : _config.print_http_response );
         }
         std::string err = "failed to " + func + ", nodeos is not running";
         throw std::runtime_error(err.c_str());
      }

      fc::variant call(int cluster_id, int node_id, const std::string &func, fc::variant args = fc::variant(), bool silence = false) {
         return call_<fc::variant>(cluster_id, node_id, func, args, silence);
      }
      fc::variant call_strdata(int cluster_id, int node_id, const std::string &func, const std::string &args, bool silence = false) {
         return call_<std::string>(cluster_id, node_id, func, args, silence);
      }

      fc::variant get_info(int cluster_id, int node_id, bool silence = false) {
         return call(cluster_id, node_id, "/v1/chain/get_info", fc::variant(), silence);
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
            throw std::runtime_error("cluster is not running");
         }
         std::vector<public_key_type> pub_keys;
         pub_keys.reserve(_running_clusters[cluster_id].imported_keys.size());
         for (auto &key : _running_clusters[cluster_id].imported_keys) {
            pub_keys.push_back(key.first);
         }
         auto get_arg = fc::mutable_variant_object("transaction", (chain::transaction)trx)
                        ("available_keys", std::move(pub_keys));
         return call(cluster_id, node_id, "/v1/chain/get_required_keys", get_arg);
      }

      fc::optional<abi_serializer> abi_serializer_resolver(int cluster_id, int node_id, const name& account) {
         auto &abi_cache = _running_clusters[cluster_id].nodes[node_id].abi_cache;
         if (fc::time_point::now() >= _running_clusters[cluster_id].nodes[node_id].abi_cache_time + fc::milliseconds(500)) {
            _running_clusters[cluster_id].nodes[node_id].abi_cache_time = fc::time_point::now();
            abi_cache.clear();
         }
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
                                  const fc::variant& action_args_var) {
         auto abis = abi_serializer_resolver(cluster_id, node_id, account);
         if (!abis.valid()) {
            _running_clusters[cluster_id].nodes[node_id].abi_cache.clear();
            FC_ASSERT(false, "No ABI found for ${contract}", ("contract", account));
         }

         auto action_type = abis->get_action_type( action );
         if (action_type.empty()) {
            _running_clusters[cluster_id].nodes[node_id].abi_cache.clear();
            FC_ASSERT(false, "Unknown action ${action} in contract ${contract}", ("action", action)( "contract", account ));
         }

         char temp[512 * 1024];
         fc::datastream<char*> ds(temp, sizeof(temp)-1 );
         abis->variant_to_binary( action_type, action_args_var, ds, _config.abi_serializer_max_time );
         std::vector<char> r;
         r.assign(temp, temp + ds.tellp());
         return r;
      }

      fc::variant push_transaction(int cluster_id, int node_id, signed_transaction& trx,
                                   const std::vector<public_key_type> &sign_keys = std::vector<public_key_type>(),
                                   packed_transaction::compression_type compression = packed_transaction::compression_type::none) {
         int port = _running_clusters[cluster_id].nodes[node_id].http_port;
         auto &info = _running_clusters[cluster_id].nodes[node_id].get_info_cache;
         if (fc::time_point::now() >= _running_clusters[cluster_id].nodes[node_id].get_info_cache_time + fc::milliseconds(500)) {
            fc::variant info_ = get_info(cluster_id, node_id);
            info = info_.as<eosio::chain_apis::read_only::get_info_results>();
            _running_clusters[cluster_id].nodes[node_id].get_info_cache_time = fc::time_point::now();
         }

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
         if (sign_keys.size()) {
            for (const public_key_type &pub_key : sign_keys) {
               if (_running_clusters[cluster_id].imported_keys.find(pub_key) != _running_clusters[cluster_id].imported_keys.end()) {
                  private_key_type pri_key = _running_clusters[cluster_id].imported_keys[pub_key];
                  trx.sign(pri_key, info.chain_id);
                  has_key = true;
               } else {
                  throw std::runtime_error("private key of \"" + (std::string)pub_key + "\" not imported");
               }
            }
         } else {
            fc::variant required_keys = determine_required_keys(cluster_id, node_id, trx);
            const fc::variant &keys = required_keys["required_keys"];
            for (const fc::variant &k : keys.get_array()) {
               public_key_type pub_key = public_key_type(k.as_string());
               private_key_type pri_key = _running_clusters[cluster_id].imported_keys[pub_key];
               trx.sign(pri_key, info.chain_id);
               has_key = true;
            }
            if (!has_key) {
               throw std::runtime_error("failed to determine required keys");
            }
         }
         _running_clusters[cluster_id].transaction_blocknum[trx.id()] = info.head_block_num + 1;

         return call(cluster_id, node_id, "/v1/chain/push_transaction", fc::variant(packed_transaction(trx, compression)));
      }

      fc::variant push_actions(int cluster_id, int node_id, std::vector<chain::action>&& actions,
                               const std::vector<public_key_type> &sign_keys = std::vector<public_key_type>(),
                               packed_transaction::compression_type compression = packed_transaction::compression_type::none ) {
         signed_transaction trx;
         trx.actions = std::move(actions);
         return push_transaction(cluster_id, node_id, trx, sign_keys, compression);
      }

      fc::variant push_actions(launcher_service::push_actions_param param) {
         std::vector<chain::action> actlist;
         actlist.reserve(param.actions.size());
         for (const action_param &p : param.actions) {
            bytes data = action_variant_to_bin(param.cluster_id, param.node_id, p.account, p.action, p.data);
            actlist.emplace_back(p.permissions, p.account, p.action, std::move(data));
         }
         return push_actions(param.cluster_id, param.node_id, std::move(actlist), param.sign_keys);
      }

      fc::variant link_auth(link_auth_param param) {
         std::vector<eosio::chain::action> actlist;
         actlist.push_back(action{
            vector<chain::permission_level>{{param.account, N(owner)}},linkauth(param.account, param.code, param.type, param.requirement)});
         return push_actions(param.cluster_id, param.node_id, std::move(actlist));
      }

      fc::variant unlink_auth(unlink_auth_param param) {
         std::vector<eosio::chain::action> actlist;
         actlist.push_back(action{
            vector<chain::permission_level>{{param.account, N(owner)}},unlinkauth(param.account, param.code, param.type)});
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
            action_variant_to_bin(param.cluster_id, param.node_id, N(eosio), N(delegatebw), delegate_act));

         // buyram
         fc::variant buyram_act = fc::mutable_variant_object()
               ("payer", param.creator.to_string())
               ("receiver", param.name.to_string())
               ("bytes", param.buy_ram_bytes);
         actlist.emplace_back(vector<chain::permission_level>{{param.creator, N(active)}}, N(eosio), N(buyrambytes),
            action_variant_to_bin(param.cluster_id, param.node_id, N(eosio), N(buyrambytes), buyram_act));

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
            throw std::runtime_error("contract_file and abi_file are both empty");
         }
         return push_actions(param.cluster_id, param.node_id, std::move(actlist), std::vector<public_key_type>(), packed_transaction::compression_type::zlib);
      }

      fc::variant import_keys(import_keys_param param) {
         if (_running_clusters.find(param.cluster_id) == _running_clusters.end()) {
            throw std::runtime_error("cluster is not running");
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
            throw std::runtime_error("cluster is not running");
         }
         fc::sha256 digest(param.seed);
         private_key_type pri_key = private_key_type::regenerate(fc::ecc::private_key::generate_from_seed(digest).get_secret());
         public_key_type pub_key = pri_key.get_public_key();
         _running_clusters[param.cluster_id].imported_keys[pub_key] = pri_key;
         return fc::mutable_variant_object("generated_key", std::string(pub_key));
      }

      fc::variant verify_transaction(launcher_service::verify_transaction_param param) {
         if (_running_clusters.find(param.cluster_id) == _running_clusters.end()) {
            throw std::runtime_error("cluster is not running");
         }
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

      fc::variant get_log_data(get_log_data_param param) {
         if (param.cluster_id < 0 || param.cluster_id >= _config.max_clusters) {
            throw std::runtime_error("invalid cluster id");
         }
         bfs::path path = bfs::path(_config.data_dir) / cluster_to_string(param.cluster_id) / node_to_string(param.node_id) / param.filename;
         size_t size = bfs::file_size(path);
         std::vector<char> data;
         size_t offset = 0;
         if (param.offset < 0) {
            if (-param.offset < size) offset = size + param.offset;
         } else {
            offset = param.offset;
         }
         size_t datasize = offset >= size ? 0 : std::min(size - offset, (size_t)param.len);
         data.resize(datasize);
         bfs::ifstream fs(path);
         fs.seekg(offset, fs.beg);
         if (datasize) {
            fs.read(&(data[0]), datasize);
            data.resize(fs.gcount());
         }
         fs.close();
         std::string base64str;
         if (data.size())
            base64str = base64_encode(&(data[0]), data.size());
         return fc::mutable_variant_object("filesize", size)("offset", offset)("data", std::move(base64str));
      }

      fc::variant send_raw(send_raw_param &param) {
         if (param.string_data.length() == 0) {
            return call(param.cluster_id, param.node_id, param.url, param.json_data);
         } else {
            return call_strdata(param.cluster_id, param.node_id, param.url, param.string_data);
         }
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

         ("base-port", bpo::value<uint16_t>()->default_value(1600),
         "base port number of clusters")
         ("cluster-port-span", bpo::value<uint16_t>()->default_value(1000),
         "length of port range reserved per cluster")
         ("node-port-span", bpo::value<uint16_t>()->default_value(10),
         "length of port range reserved per node in a cluster")
         ("max-nodes-per-cluster", bpo::value<uint16_t>()->default_value(100),
         "max nodes per cluster")
         ("max-clusters", bpo::value<uint16_t>()->default_value(30),
         "max number of clusters")
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
      _my->_config.base_port = options.at("base-port").as<uint16_t>();
      _my->_config.cluster_span = options.at("cluster-port-span").as<uint16_t>();
      _my->_config.node_span = options.at("node-port-span").as<uint16_t>();
      _my->_config.max_nodes_per_cluster = options.at("max-nodes-per-cluster").as<uint16_t>();
      _my->_config.max_clusters = options.at("max-clusters").as<uint16_t>();
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

fc::variant launcher_service_plugin::get_cluster_info(launcher_service::cluster_id_param param)
{
   if (_my->_running_clusters.find(param.cluster_id) == _my->_running_clusters.end()) {
      throw std::runtime_error("cluster is not running");
   }
   std::map<int, fc::variant> result;
   bool haserror = false, hasok = false;
   size_t nodecount = _my->_running_clusters[param.cluster_id].nodes.size();
   std::vector<std::thread> threads;
   std::vector<std::pair<int, fc::variant> > thread_result;
   std::vector<std::string> errstrs;
   thread_result.resize(nodecount);
   errstrs.resize(nodecount);
   threads.reserve(nodecount);
   int k = 0;
   bool set_errstr = true;
   for (auto &itr : _my->_running_clusters[param.cluster_id].nodes) {
      int id = itr.second.id;
      int port = itr.second.http_port;
      thread_result[k].first = -1;
      if (port) {
         threads.push_back(std::thread([this, k, cid = param.cluster_id, id, port, &thread_result, &hasok, &haserror, &errstrs]() {
            thread_result[k].first = id;
            try {
               thread_result[k].second = _my->get_info(cid, id, true);
               hasok = true;
            } catch (boost::system::system_error& e) {
               haserror = true;
               errstrs[k] = e.what();
               std::string url = "http://" + _my->_config.host_name + ":" + _my->itoa(port);
               thread_result[k].second = fc::mutable_variant_object("error", e.what())("url", url);
            } catch (fc::exception & e) {
               haserror = true;
               errstrs[k] = e.what();
               std::string url = "http://" + _my->_config.host_name + ":" + _my->itoa(port);
               thread_result[k].second = fc::mutable_variant_object("error", e.what())("url", url);
            }  catch (...) {
               haserror = true;
               errstrs[k] = "unknown error";
               std::string url = "http://" + _my->_config.host_name + ":" + _my->itoa(port);
               thread_result[k].second = fc::mutable_variant_object("error", "unknown error")("url", url);
            }
         }));
      }
      ++k;
   }
   for (auto &t: threads) {
      t.join();
   }
   for (int i = 0; i < k; ++i) {
      if (thread_result[i].first >= 0) {
         result[thread_result[i].first] = std::move(thread_result[i].second);
      }
   }
   if (!hasok && haserror) {
      for (auto &err: errstrs) {
         if (err.length())
            throw std::runtime_error(err);
      }
   }
   return fc::mutable_variant_object("result", result);
}

fc::variant launcher_service_plugin::get_cluster_running_state(launcher_service::cluster_id_param param)
{
   if (_my->_running_clusters.find(param.cluster_id) == _my->_running_clusters.end()) {
      throw std::runtime_error("cluster is not running");
   }
   for (auto &itr : _my->_running_clusters[param.cluster_id].nodes) {
      launcher_service_plugin_impl::node_state &state = itr.second;
      if (state.child && !state.child->running()) {
         state.pid = 0;
         state.child.reset();
      }
   }
   return fc::mutable_variant_object("result", _my->_running_clusters[param.cluster_id]);
}

#define CATCH_LAUCHER_EXCEPTIONS \
   catch (const std::string &s) {\
      throw std::runtime_error(s.c_str());\
   } catch (const char *s) { \
      throw std::runtime_error(s); \
   }

fc::variant launcher_service_plugin::get_info(launcher_service::node_id_param param) {
   try {
      return _my->get_info(param.cluster_id, param.node_id);
   } CATCH_LAUCHER_EXCEPTIONS
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

fc::variant launcher_service_plugin::stop_cluster(launcher_service::cluster_id_param param) {
   try {
      _my->stop_cluster(param.cluster_id);
      return fc::mutable_variant_object("result", "OK");
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::clean_cluster(launcher_service::cluster_id_param param) {
   try {
      _my->clean_cluster(param.cluster_id);
      return fc::mutable_variant_object("result", "OK");
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::start_node(launcher_service::start_node_param param) {
   try {
      _my->start_node(param.cluster_id, param.node_id, param.extra_args);
      return fc::mutable_variant_object("result", "OK");
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::stop_node(launcher_service::stop_node_param param) {
   try {
      _my->stop_node(param.cluster_id, param.node_id, param.kill_sig);
      return fc::mutable_variant_object("result", "OK");
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::link_auth(launcher_service::link_auth_param param) {
   try {
      return _my->link_auth(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::unlink_auth(launcher_service::unlink_auth_param param) {
   try {
      return _my->unlink_auth(param);
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

fc::variant launcher_service_plugin::get_protocol_features(launcher_service::node_id_param param) {
   try {
      return _my->get_protocol_features(param.cluster_id, param.node_id);
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

fc::variant launcher_service_plugin::get_log_data(launcher_service::get_log_data_param param) {
   try {
      return _my->get_log_data(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::send_raw(launcher_service::send_raw_param param) {
   try {
      return _my->send_raw(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

std::string launcher_service_plugin::generate_node_config(const launcher_service::launcher_config &_config, launcher_service::cluster_def &def, int node_id) {
   return launcher_service_plugin_impl::generate_node_config(_config, def, node_id);
}

#undef CATCH_LAUCHER_EXCEPTIONS

}

FC_REFLECT(eosio::launcher_service_plugin_impl::node_state, (id)(pid)(http_port)(p2p_port)(is_bios)(stdout_path)(stderr_path))
FC_REFLECT(eosio::launcher_service_plugin_impl::cluster_state, (nodes))
