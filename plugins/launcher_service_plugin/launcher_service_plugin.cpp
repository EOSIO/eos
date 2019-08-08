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
         int id = 0;
         int pid = 0;
         int http_port = 0;
         int p2p_port = 0;
         int is_bios = false;
         std::string stdout_path;
         std::string stderr_path;
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
            cfg << "p2p-listen-endpoint = " << _config.listen_addr << ":" << state.p2p_port << "\n";
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
      }
      void launch_nodes(const cluster_def &def, int node_id, bool restart = false) {
         for (int i = 0; i < def.node_count; ++i) {
            node_state &state = _running_clusters[def.cluster_id].nodes[i];
            node_def node_config = def.get_node_def(i);

            if (i != node_id && node_id != -1) {
               continue;
            }

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
               cmd += " --genesis-json=genesis.json";
               cmd += " --delete-all-blocks";
            }
            if (def.extra_args.length()) {
               cmd += " ";
               cmd += def.extra_args;
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
         launch_nodes(def, -1);
      }
      void stop_node(int cluster_id, int node_id) {
         if (_running_clusters.find(cluster_id) != _running_clusters.end()) {
            if (_running_clusters[cluster_id].nodes.find(node_id) != _running_clusters[cluster_id].nodes.end()) {
               node_state &state = _running_clusters[cluster_id].nodes[node_id];
               if (state.child && state.child->running()) {
                  ilog("killing pid ${p}", ("p", state.child->id()));
                  ::kill(state.child->id(), SIGKILL);
               }
               state.pid = 0;
               state.child.reset();
               _running_clusters[cluster_id].nodes.erase(node_id);
            }
         }
      }
      void stop_cluster(int cluster_id) {
         if (_running_clusters.find(cluster_id) != _running_clusters.end()) {
            while (_running_clusters[cluster_id].nodes.size()) { // don't use ranged for loop
               stop_node(cluster_id, _running_clusters[cluster_id].nodes.begin()->first);
            }
            _running_clusters.erase(cluster_id);
         }
      }
      void stop_all_clusters() {
         while (_running_clusters.size()) { // don't use ranged for loop
            stop_cluster(_running_clusters.begin()->first);
         }
      }

      fc::variant get_info(int cluster_id, int node_id) {
         int port = _running_clusters[cluster_id].nodes[node_id].http_port;
         if (port) {
            std::string url;
            url = "http://" + _config.host_name + ":" + itoa(port);
            client::http::http_context context = client::http::create_http_context();
            std::vector<std::string> headers;
            auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/get_info", false, headers);
            return client::http::do_http_call(*sp, fc::variant(), true, true );
         }
         throw std::string("failed to get_info, nodeos is not running");
      }

      fc::variant get_account(launcher_service::get_account_param param) {
         int port = _running_clusters[param.cluster_id].nodes[param.node_id].http_port;
         if (port) {
            std::string url;
            url = "http://" + _config.host_name + ":" + itoa(port);
            client::http::http_context context = client::http::create_http_context();
            std::vector<std::string> headers;
            auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/get_account", false, headers);
            return client::http::do_http_call(*sp, fc::mutable_variant_object("account_name", param.name), true, true );
         }
         throw std::string("failed to get_account, nodeos is not running");
      }

      fc::variant get_block(int cluster_id, int node_id, std::string block_num_or_id) {
         int port = _running_clusters[cluster_id].nodes[node_id].http_port;
         if (port) {
            std::string url;
            url = "http://" + _config.host_name + ":" + itoa(port);
            client::http::http_context context = client::http::create_http_context();
            std::vector<std::string> headers;
            auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/get_block", false, headers);
            return client::http::do_http_call(*sp, fc::mutable_variant_object("block_num_or_id", block_num_or_id), true, true );
         }
         throw std::string("failed to get_block, nodeos is not running");
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

         // TODO
         //auto required_keys = determine_required_keys(trx);
         //sign_transaction(trx, required_keys, info.chain_id);
         trx.sign(_config.default_key, info.chain_id);

         _running_clusters[cluster_id].transaction_blocknum[trx.id()] = info.head_block_num + 1;

         client::http::http_context context = client::http::create_http_context();
         std::vector<std::string> headers;
         auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/push_transaction", false, headers);
         return client::http::do_http_call(*sp, fc::variant(packed_transaction(trx, compression)), true, true );
      }

      fc::variant push_actions(int cluster_id, int node_id, std::vector<chain::action>&& actions, packed_transaction::compression_type compression = packed_transaction::compression_type::none ) {
         signed_transaction trx;
         trx.actions = std::forward<decltype(actions)>(actions);

         return push_transaction(cluster_id, node_id, trx, compression);
      }

      fc::variant create_bios_accounts(create_bios_accounts_param param) {
         std::vector<eosio::chain::action> actlist;
         for (auto &a : param.accounts) {
            actlist.push_back(create_newaccount(param.creator, a.name, a.owner, a.active));
         }
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
         return push_actions(param.cluster_id, param.node_id, std::move(actlist));
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
   };

launcher_service_plugin::launcher_service_plugin():_my(new launcher_service_plugin_impl()){}
launcher_service_plugin::~launcher_service_plugin(){}

void launcher_service_plugin::set_program_options(options_description&, options_description& cfg) {
   std::cout << "launcher_service_plugin::set_program_options()" << std::endl;
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void launcher_service_plugin::plugin_initialize(const variables_map& options) {
   std::cout << "launcher_service_plugin::plugin_initialize()" << std::endl;
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
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
   bool print_request = false;
   bool print_response = false;
   try {
      client::http::http_context context = client::http::create_http_context();
      std::vector<std::string> headers;
      auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/get_info", false, headers);
      auto r = client::http::do_http_call(*sp, fc::variant(), print_request, print_response );
      return r;
   } catch (boost::system::system_error& e) {
      return fc::mutable_variant_object("exception", e.what())("url", url);
   }
}

fc::variant launcher_service_plugin::get_cluster_info(int cluster_id)
{
   bool print_request = false;
   bool print_response = false;
   std::map<int, fc::variant> res;
   for (auto &itr : _my->_running_clusters[cluster_id].nodes) {
      int id = itr.second.id;
      int port = itr.second.http_port;
      if (port && itr.second.pid) {
         std::string url;
         try {
            url = "http://" + _my->_config.host_name + ":" + _my->itoa(port);
            client::http::http_context context = client::http::create_http_context();
            std::vector<std::string> headers;
            auto sp = std::make_unique<client::http::connection_param>(context, client::http::parse_url(url) + "/v1/chain/get_info", false, headers);
            res[id] = client::http::do_http_call(*sp, fc::variant(), print_request, print_response );
         } catch (boost::system::system_error& e) {
            res[id] = fc::mutable_variant_object("exception", e.what())("url", url);
         }
      }
   }
   return res.size() ? fc::mutable_variant_object("result", res) : fc::mutable_variant_object("error", "cluster is not running");
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

fc::variant launcher_service_plugin::create_bios_accounts(launcher_service::create_bios_accounts_param param) {
   try {
      return _my->create_bios_accounts(param);
   } CATCH_LAUCHER_EXCEPTIONS
}

fc::variant launcher_service_plugin::get_account(launcher_service::get_account_param param) {
   try {
      return _my->get_account(param);
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

#undef CATCH_LAUCHER_EXCEPTIONS

}

FC_REFLECT(eosio::launcher_service_plugin_impl::node_state, (id)(pid)(http_port)(p2p_port)(is_bios)(stdout_path)(stderr_path))
FC_REFLECT(eosio::launcher_service_plugin_impl::cluster_state, (nodes))
