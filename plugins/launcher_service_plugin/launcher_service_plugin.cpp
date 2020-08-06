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
#include <fc/exception/exception.hpp>

#include <eosio/launcher_service_plugin/launcher_service_plugin.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <appbase/application.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/thread_utils.hpp>

namespace eosio {
   static appbase::abstract_plugin& _launcher_service_plugin = app().register_plugin<launcher_service_plugin>();

   namespace bfs = boost::filesystem;
   namespace bp = boost::process;
   namespace bpo = boost::program_options;
   using chain::abi_serializer;
   using namespace chain;

   namespace launcher_service {

      using get_info_results = chain_apis::read_only::get_info_results;
      using get_abi_results = chain_apis::read_only::get_abi_results;

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
   using namespace fc;
   using steady_timer = boost::asio::steady_timer;

   static constexpr uint64_t                 _milliseconds = 1'000;

   class launcher_interval_timer : public std::enable_shared_from_this<launcher_interval_timer> {
   public:
      launcher_interval_timer(launcher_service_plugin_impl& impl) : _impl(impl) {}
      ~launcher_interval_timer();

      void start_timer();
      void stop_timer();

      map<time_point, time_point> missed_intervals(const time_point& report_from, const time_point& oldest_active_time);

      static const microseconds                 _half_timeout;
      static const steady_timer::duration       _timeout;
      static const microseconds                 _timeout_threshold;

   private:
      void schedule_timer();
      void timeout_processing();
      void track_timeouts(const time_point& now);

      optional<eosio::chain::named_thread_pool> _thread_pool;
      optional<steady_timer>                    _timer;
      launcher_service_plugin_impl&             _impl;
      time_point                                _last_timeout;
      mutable std::mutex                        _missed_times_mtx;
      map<time_point, time_point>               _missed_times;
   };

   class launcher_service_plugin_impl : public std::enable_shared_from_this<launcher_service_plugin_impl> {

   public:
      launcher_service_plugin_impl() : _interval_timer(std::make_shared<launcher_interval_timer>(*this)) {}

      using child_ptr = std::shared_ptr<bp::child>;
      struct node_state {
         int                        id = 0;
         int                        pid = 0;
         uint16_t                   http_port = 0;
         uint16_t                   p2p_port = 0;
         bool                       is_bios = false;
         std::string                stdout_path;
         std::string                stderr_path;
         child_ptr                  child;

         std::map<name, fc::optional<abi_serializer> >  abi_cache;
         fc::time_point                                 abi_cache_time;

         get_info_results                               get_info_cache;
         fc::time_point                                 get_info_cache_time;

         void set_child(std::shared_ptr<bp::child>&& c = std::shared_ptr<bp::child>()) {
            if (child)
               dlog("Dropping child for pid ${p} (node ${n})", ("p", child->id())("n", id));

            child = std::move(c);
            if (child) {
               pid = child->id();
               dlog("Adding child for pid ${p} (node ${n})", ("p", child->id())("n", id));
            }
            else
               pid = 0;
         }
      };

      using node_state_ptr = std::shared_ptr<node_state>;
      using weak_node_state_ptr = std::weak_ptr<node_state>;

      struct cluster_state {
         cluster_def                                 def;
         std::map<int, node_state_ptr>               nodes; // node id => node_state
         std::map<public_key_type, private_key_type> imported_keys;
         time_point                                  started = time_point::now();

         // possible transaction block-number for query purpose
         std::map<transaction_id_type, uint32_t>     transaction_blocknum;
      };

      struct to_remove {
         explicit to_remove(const node_state_ptr& n) : child(n->child), node(n) {}
         to_remove() {}

         child_ptr           child;
         weak_node_state_ptr node;
         int                 num_timeouts {0};
      };
      using remove_map = std::map<int, to_remove>;

      static constexpr uint64_t                 _seconds = 1'000 * _milliseconds;
      static constexpr uint64_t                 _minutes = 60 * _seconds;
      launcher_config                           _config;
      std::map<int, cluster_state>              _running_clusters;
      remove_map                                _nodes_to_remove;
      time_point                                _node_remove_deadline;
      static const microseconds                 _node_remove_timeout;
      optional<microseconds>                    _idle_timeout;
      optional<time_point>                      _idle_timeout_deadline;
      uint32_t                                  _timer_correlation_id = 0;
      std::shared_ptr<launcher_interval_timer>  _interval_timer;

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
            ++startport;
         }
         throw std::runtime_error("failed to assign available ports");
      }

      // separate config generation logic to make unitest possible
      static std::string generate_node_config(const launcher_config& _config, cluster_def& def, int node_id) {
         std::stringstream cfg;

         const node_def& node_config = def.get_node_def(node_id);

         cfg << "http-server-address = " << _config.host_name << ":" << node_config.http_port(_config, def.cluster_id, node_id) << "\n";
         cfg << "http-validate-host = false\n";
         cfg << "p2p-listen-endpoint = " << _config.p2p_listen_addr << ":" << node_config.p2p_port(_config, def.cluster_id, node_id) << "\n";
         cfg << "p2p-max-nodes-per-host = " << def.node_count << "\n";
         cfg << "agent-name = " << cluster_to_string(def.cluster_id) << "_" << node_to_string(node_id) << "\n";

         if (def.shape == "mesh") {
            for (int peer = 0; peer < node_id; ++peer) {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id, peer) << "\n";
            }
         } else if (def.shape == "star") {
            int peer = def.center_node_id;
            if (peer < 0 || peer >= def.node_count) {
               throw std::runtime_error("invalid center_node_id for star topology");
            }
            if (node_id != peer) {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id, peer) << "\n";
            }
         } else if (def.shape == "bridge") {
            if (def.center_node_id <= 0 || def.center_node_id >= def.node_count - 1) {
               throw std::runtime_error("invalid center_node_id for bridge topology");
            }
            if (node_id < def.center_node_id) {
               for (int peer = 0; peer < node_id; ++peer) {
                  cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id, peer) << "\n";
               }
            } else if (node_id > def.center_node_id) {
               for (int peer = def.center_node_id + 1; peer < node_id; ++peer) {
                  cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id, peer) << "\n";
               }
            } else {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(node_id - 1).p2p_port(_config, def.cluster_id, node_id - 1) << "\n";
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(node_id + 1).p2p_port(_config, def.cluster_id, node_id + 1) << "\n";
            }
         } else if (def.shape == "line") {
            if (node_id) {
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(node_id - 1).p2p_port(_config, def.cluster_id, node_id - 1) << "\n";
            }
         } else if (def.shape == "ring") {
            int peer = (node_id - 1 + def.node_count) % def.node_count;
            cfg << "p2p-peer-address = " << _config.host_name << ":"
                  << def.get_node_def(peer).p2p_port(_config, def.cluster_id, peer) << "\n";
         } else if (def.shape == "tree") {
            if (node_id) {
               int peer = (node_id + 1) / 2 - 1;
               cfg << "p2p-peer-address = " << _config.host_name << ":" << def.get_node_def(peer).p2p_port(_config, def.cluster_id, peer) << "\n";
            }
         } else {
            throw std::runtime_error("invalid shape");
         }

         if (node_config.is_bios()) {
            cfg << "enable-stale-production = true\n";
         }
         cfg << "allowed-connection = any\n";

         for (const auto& priKey: node_config.producing_keys) {
            cfg << "private-key = [\"" << priKey.get_public_key().to_string() << "\",\""
               << priKey.to_string() << "\"]\n";
         }

         if (node_config.producers.size()) {
            cfg << "private-key = [\"" << _config.default_key.get_public_key().to_string() << "\",\""
               << _config.default_key.to_string() << "\"]\n";
         }
         for (auto& p : node_config.producers) {
            cfg << "producer-name = " << p << "\n";
         }
         cfg << "plugin = eosio::net_plugin\n";
         cfg << "plugin = eosio::chain_api_plugin\n";
         cfg << "plugin = eosio::producer_api_plugin\n";

         for (const std::string& s : def.extra_configs) {
            cfg << s << "\n";
         }
         for (const std::string& s : node_config.extra_configs) {
            cfg << s << "\n";
         }
         return cfg.str();
      }

      void setup_cluster(cluster_def& def) {
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
               node_def& node_config = def.get_node_def(i);
               begin_port = next_avail_port(begin_port, max_port);
               node_config.assigned_http_port = begin_port;
               ++begin_port;
               begin_port = next_avail_port(begin_port, max_port);
               node_config.assigned_p2p_port = begin_port;
               ++begin_port;
            }
         }

         auto itr = _running_clusters.find(def.cluster_id);
         if (itr == _running_clusters.end()) {
            itr = _running_clusters.emplace(def.cluster_id, cluster_state{}).first;
         }

         for (int i = 0; i < def.node_count; ++i) {
            node_def& node_config = def.get_node_def(i);
            node_state_ptr state = std::make_shared<node_state>();
            state->id = i;
            state->http_port = node_config.http_port(_config, def.cluster_id, i);
            state->p2p_port = node_config.p2p_port(_config, def.cluster_id, i);
            state->is_bios = node_config.is_bios();

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
            itr->second.nodes[i] = std::move(state);
         }
         itr->second.def = def;
         itr->second.imported_keys[_config.default_key.get_public_key()] = _config.default_key;
      }

      void launch_nodes(cluster_def& def, int node_id, bool restart = false, std::string extra_args = "") {
         auto& cluster_state = get_cluster_state(def.cluster_id)->second;
         for (int i = 0; i < def.node_count; ++i) {
            if (i != node_id && node_id != -1) {
               continue;
            }

            node_state& state = *cluster_state.nodes[i];
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

            std::string cmd = node_config.nodeos_cmd.length() ? node_config.nodeos_cmd : _config.nodeos_cmd;
            cmd += " --config-dir=" + node_path.string();
            cmd += " --data-dir=" + node_path.string() + "/data";
            if (!restart) {
               if (def.extra_args.find("--genesis-json") == string::npos && extra_args.find("--genesis-json") == string::npos) {
                  cmd += " --genesis-json=" + _config.genesis_file;
               }
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

            if (node_id == -1 && node_config.dont_start) {
               state.set_child();
               continue;
            }

            ilog("start to execute:\"${c}\"", ("c", cmd));
            state.set_child(std::make_shared<bp::child>(cmd, bp::std_out > stdout_path, bp::std_err > stderr_path));

            pidout << state.child->id();
            pidout.flush();
            pidout.close();
            state.child->detach();
         }
      }

      void launch_cluster(cluster_def& def) {
         if (def.cluster_id < 0 || def.cluster_id >= _config.max_clusters) {
            throw std::runtime_error("invalid cluster id");
         }
         if (_running_clusters.count(def.cluster_id)) {
            stop_cluster(def.cluster_id);
         }
         setup_cluster(def);
         launch_nodes(def, -1, false);
      }

      void start_node(int cluster_id, int node_id, std::string extra_args) {
         auto& cluster_state = get_cluster_state(cluster_id)->second;
         if (cluster_state.nodes.find(node_id) != cluster_state.nodes.end()) {
            node_state& state = *cluster_state.nodes[node_id];
            if (state.child && state.child->running()) {
               throw std::runtime_error("node already running");
            }
         }
         launch_nodes(cluster_state.def, node_id, true, extra_args);
      }

      void stop_node(int cluster_id, int node_id, int killsig) {
         auto& cluster_state = get_cluster_state(cluster_id)->second;
         if (cluster_state.nodes.find(node_id) != cluster_state.nodes.end()) {
            node_state_ptr state = cluster_state.nodes[node_id];
            if (state->child) {
               if (state->child->running()) {
                  ilog("killing pid ${p} (cluster ${c}, node ${n}) with signal ${s}", ("p", state->child->id())("c", cluster_id)("n", node_id)("s", killsig));
                  string kill_cmd = "kill " + boost::lexical_cast<std::string>(killsig) + " " + boost::lexical_cast<std::string>(state->child->id());
                  boost::process::system( kill_cmd );
               }
               if (!state->child->running()) {
                  dlog("Removing pid ${p} (cluster ${c}, node ${n}) with signal ${s}", ("p", state->child->id())("c", cluster_id)("n", node_id)("s", killsig));
                  state->set_child();
               } else {
                  dlog("Storing pid ${p} (cluster ${c}, node ${n}) to clean up later", ("p", state->child->id())("c", cluster_id)("n", node_id));
                  _nodes_to_remove[state->pid] = to_remove(state);
               }
            }
         }
      }

      void interval_callback(const time_point& now) {
         std::weak_ptr<launcher_service_plugin_impl> weak_this = weak_from_this();
         app().post( priority::low,
                [weak_this, now]() {
                        auto self = weak_this.lock();
                        if( self ) {
                           self->remove_shutdown_nodes(now);

                           // do this last, to allow everything else to happen prior to quit
                           self->check_idle_timeout(now);
                        }
                     } );
      }

      void initialize_timers() {
         schedule_node_remove_timeout();
         schedule_idle_timeout();
         _interval_timer->start_timer();
      }

      void remove_shutdown_nodes(const time_point& now) {
         if (now < _node_remove_deadline)
            return;

         schedule_node_remove_timeout();
         dlog("Checking ${count} nodes to determine if they have shutdown and are ready to be cleaned up",
              ("count", _nodes_to_remove.size()));
         if (_nodes_to_remove.empty())
            return;

         std::vector<int> remove_now;
         dlog("Checking ${count} nodes to determine if they have shutdown and are ready to be cleaned up",
              ("count", _nodes_to_remove.size()));
         for (auto& node_to_remove : _nodes_to_remove ) {
            auto child = node_to_remove.second.child;
            if (!child->running()) {
               auto node = node_to_remove.second.node.lock();
               // if the node is still valid and it is still holding this child, clean up the node's child
               if (node && node->child.get() == child.get()) {
                  node->set_child();
               }
               remove_now.push_back(node_to_remove.first);
            }
            else if (++node_to_remove.second.num_timeouts % 10 == 0) {
               ilog("Node with pid: ${pid} still not shutdown after ${seconds} seconds",
                    ("pid", node_to_remove.first)("seconds",node_to_remove.second.num_timeouts));
            }
         }
         for (const auto& rem_node : remove_now) {
            _nodes_to_remove.erase(rem_node);
         }
      }

      std::map<time_point, time_point> stop_cluster(int cluster_id) {
         auto cluster_state_itr = get_cluster_state(cluster_id);
         ilog("Stopping cluster: ${cid}", ("cid", cluster_id));
         for (auto& itr : cluster_state_itr->second.nodes) {
            stop_node(cluster_id, itr.first, SIGKILL);
         }
         const time_point started = cluster_state_itr->second.started;
         _running_clusters.erase(cluster_state_itr);

         time_point last_needed_time = time_point::now();
         for (const auto& itr : _running_clusters) {
            last_needed_time = std::min(itr.second.started, last_needed_time);
         }

         std::map<time_point, time_point> missed_times = _interval_timer->missed_intervals(started, last_needed_time);
         ilog("cluster_id: ${id} experienced ${count} missed time intervals during operations", ("id", cluster_id)("count", missed_times.size()));
         return missed_times;
      }

      void shutdown() {
         _interval_timer->stop_timer();
         stop_all_clusters();
         _idle_timeout.reset();
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
      fc::variant call_(int cluster_id, int node_id, const std::string& func, const T& args, bool silence) {
         if (_running_clusters.find(cluster_id) == _running_clusters.end()) {
            throw std::runtime_error("cluster is not running");
         }
         if (_running_clusters[cluster_id].nodes.find(node_id) == _running_clusters[cluster_id].nodes.end()) {
            throw std::runtime_error("nodeos is not running");
         }
         const int port = _running_clusters[cluster_id].nodes[node_id]->http_port;
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

      fc::variant call(int cluster_id, int node_id, const std::string& func, fc::variant args = fc::variant(), bool silence = false) {
         return call_<fc::variant>(cluster_id, node_id, func, args, silence);
      }
      fc::variant call_strdata(int cluster_id, int node_id, const std::string& func, const std::string& args, bool silence = false) {
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
         auto ret = call(cluster_id, node_id, "/v1/chain/get_block", fc::mutable_variant_object("block_num_or_id", block_num_or_id));
         return ret;
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
         for (auto& key : _running_clusters[cluster_id].imported_keys) {
            pub_keys.push_back(key.first);
         }
         auto get_arg = fc::mutable_variant_object("transaction", (chain::transaction)trx)
                        ("available_keys", std::move(pub_keys));
         return call(cluster_id, node_id, "/v1/chain/get_required_keys", get_arg);
      }

      fc::optional<abi_serializer> abi_serializer_resolver(int cluster_id, int node_id, const name& account) {
         auto& abi_cache = _running_clusters[cluster_id].nodes[node_id]->abi_cache;
         if (fc::time_point::now() >= _running_clusters[cluster_id].nodes[node_id]->abi_cache_time + fc::milliseconds(config::block_interval_ms)) {
            _running_clusters[cluster_id].nodes[node_id]->abi_cache_time = fc::time_point::now();
            abi_cache.clear();
         }
         auto it = abi_cache.find( account );
         if ( it == abi_cache.end() ) {
            auto result = call(cluster_id, node_id, "/v1/chain/get_abi", fc::mutable_variant_object("account_name", account));
            auto abi_results = result.as<get_abi_results>();
            fc::optional<abi_serializer> abis;
            if( abi_results.abi.valid() ) {
               abis.emplace( *abi_results.abi, abi_serializer::create_yield_function( _config.abi_serializer_max_time ) );
            }
            std::tie( it, std::ignore ) = abi_cache.emplace( account, abis );
         }
         return it->second;
      };

      bytes action_variant_to_bin(int cluster_id, int node_id, const name& account, const name& action,
                                  const fc::variant& action_args_var) {
         auto abis = abi_serializer_resolver(cluster_id, node_id, account);
         if (!abis.valid()) {
            _running_clusters[cluster_id].nodes[node_id]->abi_cache.erase(account);
            FC_THROW_EXCEPTION(chain::abi_not_found_exception, "No ABI found for ${contract}", ("contract", account));
         }

         auto action_type = abis->get_action_type( action );
         if (action_type.empty()) {
            FC_THROW_EXCEPTION(chain::invalid_ricardian_action_exception, "Unknown action ${action} in contract ${contract}", ("action", action)( "contract", account ));
         }

         char temp[512 * 1024];
         fc::datastream<char*> ds(temp, sizeof(temp)-1 );
         abis->variant_to_binary( action_type, action_args_var, ds, abi_serializer::create_yield_function( _config.abi_serializer_max_time ) );
         std::vector<char> r;
         r.assign(temp, temp + ds.tellp());
         return r;
      }

      fc::variant push_transaction(int cluster_id, int node_id, signed_transaction& trx,
                                   const std::vector<public_key_type>& sign_keys = std::vector<public_key_type>(),
                                   packed_transaction::compression_type compression = packed_transaction::compression_type::none) {
         const int port = _running_clusters[cluster_id].nodes[node_id]->http_port;
         auto& info = _running_clusters[cluster_id].nodes[node_id]->get_info_cache;
         if (fc::time_point::now() >= _running_clusters[cluster_id].nodes[node_id]->get_info_cache_time + fc::milliseconds(config::block_interval_ms)) {
            fc::variant info_ = get_info(cluster_id, node_id);
            info = info_.as<get_info_results>();
            _running_clusters[cluster_id].nodes[node_id]->get_info_cache_time = fc::time_point::now();
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

         if (sign_keys.size()) {
            for (const public_key_type& pub_key : sign_keys) {
               auto itr = _running_clusters[cluster_id].imported_keys.find(pub_key);
               if (itr != _running_clusters[cluster_id].imported_keys.end()) {
                  private_key_type pri_key = itr->second;
                  trx.sign(pri_key, *(chain_id_type *)&(info.chain_id));
               } else {
                  throw std::runtime_error("private key of \"" + pub_key.to_string() + "\" not imported");
               }
            }
         } else {
            fc::variant required_keys = determine_required_keys(cluster_id, node_id, trx);
            const fc::variant& keys = required_keys["required_keys"];
            for (const fc::variant& k : keys.get_array()) {
               public_key_type pub_key = public_key_type(k.as_string());
               auto itr = _running_clusters[cluster_id].imported_keys.find(pub_key);
               if (itr != _running_clusters[cluster_id].imported_keys.end()) {
                  private_key_type pri_key = itr->second;
                  trx.sign(pri_key, *(chain_id_type *)&(info.chain_id));
               } else {
                  throw std::runtime_error("private key of \"" + pub_key.to_string() + "\" not imported");
               }
            }
         }
         _running_clusters[cluster_id].transaction_blocknum[trx.id()] = info.head_block_num + 1;

         const bool legacy = true;
         return call(cluster_id, node_id, "/v1/chain/send_transaction", fc::variant(packed_transaction_v0(trx, compression)));
      }

      fc::variant push_actions(int cluster_id, int node_id, std::vector<chain::action>&& actions,
                               const std::vector<public_key_type>& sign_keys = std::vector<public_key_type>(),
                               packed_transaction::compression_type compression = packed_transaction::compression_type::none ) {
         signed_transaction trx;
         trx.actions = std::move(actions);
         return push_transaction(cluster_id, node_id, trx, sign_keys, compression);
      }

      fc::variant push_actions(launcher_service::push_actions_param param) {
         std::vector<chain::action> actlist;
         actlist.reserve(param.actions.size());
         for (const action_param& p : param.actions) {
            bytes data = action_variant_to_bin(param.cluster_id, param.node_id, p.account, p.action, p.data);
            actlist.emplace_back(p.permissions, p.account, p.action, std::move(data));
         }
         return push_actions(param.cluster_id, param.node_id, std::move(actlist), param.sign_keys);
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

       std::map<int, cluster_state>::iterator get_cluster_state(int cluster_id) {
          auto itr = _running_clusters.find(cluster_id);
          if (itr == _running_clusters.end()) {
              throw std::runtime_error("cluster is not running");
          }
          return itr;
      }

      fc::variant import_keys(import_keys_param param) {
         auto& cluster_state = get_cluster_state(param.cluster_id)->second;
         std::vector<std::string> pub_keys;
         for (auto& key : param.keys) {
            auto pkey = key.get_public_key();
            pub_keys.push_back(pkey.to_string());
            cluster_state.imported_keys[pkey] = key;
         }
         return fc::mutable_variant_object("imported_keys", pub_keys);
      }

      fc::variant generate_key(generate_key_param param) {
         auto& cluster_state = get_cluster_state(param.cluster_id)->second;
         fc::sha256 digest(param.seed);
         private_key_type pri_key = private_key_type::regenerate(fc::ecc::private_key::generate_from_seed(digest).get_secret());
         public_key_type pub_key = pri_key.get_public_key();
         cluster_state.imported_keys[pub_key] = pri_key;
         return fc::mutable_variant_object("generated_key", pub_key.to_string());
      }

      fc::variant verify_transaction(launcher_service::verify_transaction_param param) {
         auto& cluster_state = get_cluster_state(param.cluster_id)->second;
         uint32_t txn_block_num = param.block_num_hint;
         if (!txn_block_num) {
            auto itr = cluster_state.transaction_blocknum.find(param.transaction_id);
            if (itr == cluster_state.transaction_blocknum.end()) {
               return fc::mutable_variant_object("result", "transaction not found but block_num_hint was not given");
            }
            txn_block_num = itr->second;
         }
         fc::variant info_ = get_info(param.cluster_id, param.node_id);
         get_info_results info = info_.as<get_info_results>();
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
                  const fc::variant& txns_ = block["transactions"];
                  if (txns_.is_array()) {
                     const variants& txns = txns_.get_array();
                     for (const variant& txn : txns) {
                        const variant& trx = txn["trx"];
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
         size_t datasize = offset >= size ? 0 : std::min(size - offset, (size_t)param.length);
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

      fc::variant send_raw(send_raw_param& param) {
         if (param.string_data.length() == 0) {
            return call(param.cluster_id, param.node_id, param.url, param.json_data);
         } else {
            return call_strdata(param.cluster_id, param.node_id, param.url, param.string_data);
         }
      }

      void schedule_idle_timeout() {
         if (_idle_timeout) {
            _idle_timeout_deadline = time_point::now() + *_idle_timeout;
          }
      }

      void schedule_node_remove_timeout() {
         _node_remove_deadline = time_point::now() + _node_remove_timeout;
      }

      void check_idle_timeout(const time_point& now) {
         if (_idle_timeout_deadline && *_idle_timeout_deadline < now) {
            ilog("No requests made in at least ${timeout} minutes, shutting down service.",("timeout", _idle_timeout->count()/launcher_service_plugin_impl::_minutes));
            app().quit();
         }
      }
   };


const microseconds launcher_interval_timer::_half_timeout {50 * _milliseconds};
const steady_timer::duration launcher_interval_timer::_timeout {std::chrono::microseconds(2 * launcher_interval_timer::_half_timeout.count())};
// threshold set to 50% past duration
const microseconds launcher_interval_timer::_timeout_threshold {3 * launcher_interval_timer::_half_timeout.count()};

launcher_interval_timer::~launcher_interval_timer() {
   stop_timer();
}

void launcher_interval_timer::timeout_processing() {
   const time_point now = time_point::now();
   _impl.interval_callback(now);
   track_timeouts(now);
   schedule_timer();
}

map<time_point, time_point> launcher_interval_timer::missed_intervals(const time_point& report_from, const time_point& oldest_active_time) {
   std::lock_guard<std::mutex> g(_missed_times_mtx);
   if (_missed_times.empty()) {
      return {};
   }
   auto itr = _missed_times.lower_bound(report_from);
   map<time_point, time_point> missed;
   missed.insert(itr, _missed_times.end());
   auto del_itr = _missed_times.upper_bound(oldest_active_time);
   if (del_itr != _missed_times.begin()) {
      _missed_times.erase(_missed_times.begin(), del_itr);
   }
   return missed;
}

void launcher_interval_timer::track_timeouts(const time_point& now) {
   const auto diff = now - _last_timeout;
   if (diff > launcher_interval_timer::_timeout_threshold) {
      const auto over_in_ms = (diff.count() - launcher_interval_timer::_half_timeout.count() * 2) / _milliseconds;
      elog("System responsiveness is poor. Responsiveness delay occurred between ${last} and ${now} "
           "(overran time window by: ${over} ms).",
           ("last", _last_timeout)("now", now)("over", over_in_ms));
      std::lock_guard<std::mutex> g(_missed_times_mtx);
      _missed_times.emplace(_last_timeout, now);
   }
   const auto over_in_ms = (diff.count() - launcher_interval_timer::_half_timeout.count() * 2) / _milliseconds;
   _last_timeout = now;
}

void launcher_interval_timer::stop_timer() {
   if (_timer) {
      _timer->cancel();
      _timer.reset();
   }
   if (_thread_pool) {
      _thread_pool->stop();
      _thread_pool.reset();
   }
}

void launcher_interval_timer::start_timer() {
   ilog("start_timer");
   _last_timeout = time_point::now();
   _thread_pool.emplace("watchdog", 1);
   _timer.emplace(_thread_pool->get_executor());
   schedule_timer();
}

void launcher_interval_timer::schedule_timer() {
   _timer->expires_from_now( _timeout );
   std::weak_ptr<launcher_interval_timer> weak_this = weak_from_this();

   _timer->async_wait([weak_this]( const boost::system::error_code& ec ) {
                         auto self = weak_this.lock();
                         if( self && ec != boost::asio::error::operation_aborted ) {
                            self->timeout_processing();
                         }
                      } );
}

const microseconds launcher_service_plugin_impl::_node_remove_timeout {_seconds};

launcher_service_plugin::reschedule_idle_timeout::reschedule_idle_timeout(launcher_service_plugin_impl_ptr impl)
: _my(std::move(impl)) {
}

launcher_service_plugin::reschedule_idle_timeout::~reschedule_idle_timeout() {
   _my->schedule_idle_timeout();
}

launcher_service_plugin::reschedule_idle_timeout launcher_service_plugin::ensure_idle_timeout_reschedule() {
   return reschedule_idle_timeout(_my);
}


launcher_service_plugin::launcher_service_plugin():_my(new launcher_service_plugin_impl()){}
launcher_service_plugin::~launcher_service_plugin(){}

void launcher_service_plugin::set_program_options(options_description&, options_description& cfg) {
   ilog("launcher_service_plugin::set_program_options()");
   cfg.add_options()
         ("data-dir", bpo::value<string>()->default_value("data-dir"),
         "data directory of clusters")
         ("host-name", bpo::value<string>()->default_value("127.0.0.1"),
         "name or ip address of host")
         ("p2p-listen-address", bpo::value<string>()->default_value("0.0.0.0"),
         "listen address of incoming p2p connection of nodeos")
         ("nodeos-cmd", bpo::value<string>()->default_value("./programs/nodeos/nodeos"),
         "nodeos executable command")
         ("genesis-json", bpo::value<string>()->default_value("genesis.json"),
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
         ("create-default-genesis", bpo::bool_switch()->default_value(false),
         "create the \"genesis-json\" file, using a default genesis")
         ("idle-shutdown", bpo::value<uint16_t>(),
         "timeout in minutes to wait between launcher service requests before automatically shutting down the launcher service")
         ;
}

void launcher_service_plugin::plugin_initialize(const variables_map& options) {
   ilog("launcher_service_plugin::plugin_initialize()");
   try {
      _my->_config.data_dir = options.at("data-dir").as<std::string>();
      _my->_config.host_name = options.at("host-name").as<std::string>();
      _my->_config.p2p_listen_addr = options.at("p2p-listen-address").as<std::string>();
      _my->_config.nodeos_cmd = options.at("nodeos-cmd").as<std::string>();
      _my->_config.genesis_file = options.at("genesis-json").as<std::string>();
      _my->_config.abi_serializer_max_time = microseconds(options.at("abi-serializer-max-time").as<uint32_t>());
      _my->_config.log_file_rotate_max = options.at("log-file-max").as<uint16_t>();
      _my->_config.default_key = private_key_type(options.at("default-key").as<std::string>());
      _my->_config.base_port = options.at("base-port").as<uint16_t>();
      _my->_config.cluster_span = options.at("cluster-port-span").as<uint16_t>();
      _my->_config.node_span = options.at("node-port-span").as<uint16_t>();
      _my->_config.max_nodes_per_cluster = options.at("max-nodes-per-cluster").as<uint16_t>();
      _my->_config.max_clusters = options.at("max-clusters").as<uint16_t>();
      if (options.count("idle-shutdown")) {
          const auto timeout = options.at("idle-shutdown").as<uint16_t>();
          EOS_ASSERT(timeout > 0, chain::plugin_config_exception, "if passing \"idle-shutdown\" it must be greater than 0.");
          ilog("setting \"idle-shutdown\" timeout to ${timeout} seconds",("timeout", timeout));
          _my->_idle_timeout = microseconds(timeout * launcher_service_plugin_impl::_minutes);
      }

      // ephemeral port range starts from 32768 in linux, 49152 on mac
      EOS_ASSERT((int)_my->_config.base_port + (int)_my->_config.cluster_span <= 32768, chain::plugin_config_exception, "base-port + cluster-port-span should not be greater than 32768");
      EOS_ASSERT(_my->_config.node_span >= 2, chain::plugin_config_exception, "node-port-span must at least 2");
      EOS_ASSERT((int)_my->_config.base_port + (int)_my->_config.cluster_span * _my->_config.max_clusters <= 32768, chain::plugin_config_exception, "max-clusters too large, cluster port numbers would exceed 32767");
      EOS_ASSERT((int)_my->_config.max_nodes_per_cluster * _my->_config.node_span <= _my->_config.cluster_span, chain::plugin_config_exception, "cluster-port-span should not less than node-port-span * max-nodes-per-cluster");

      if (options.at("create-default-genesis").as<bool>()) {
          auto genesis_file = bfs::path(_my->_config.genesis_file);
          auto genesis_file_parent_path = genesis_file.parent_path();
          EOS_ASSERT(bfs::exists(genesis_file_parent_path), chain::plugin_config_exception,"genesis-json parent path: ${path} does not exist", ("path", genesis_file_parent_path.string()));
          private_key_type kp(string("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"));
          eosio::chain::genesis_state default_genesis;
          default_genesis.initial_key = kp.get_public_key();
          fc::json::save_to_file( default_genesis, _my->_config.genesis_file, true );
      } else {
          EOS_ASSERT(bfs::exists(_my->_config.genesis_file), chain::plugin_config_exception,"genesis-json ${path} does not exist", ("path", _my->_config.genesis_file));
      }

      EOS_ASSERT(options.count("http-server-address"), chain::plugin_config_exception, "http-server-address must be set");
   } catch (fc::exception& er) {
      elog( "${details}", ("details",er.to_detail_string()) );
      throw er;
   } catch (std::exception& er) {
      elog( "${details}", ("details",er.what()) );
      throw er;
   }
}

void launcher_service_plugin::plugin_startup() {
   ilog("launcher_service_plugin::plugin_startup()");
   _my->initialize_timers();
}

void launcher_service_plugin::plugin_shutdown() {
   ilog("launcher_service_plugin::plugin_shutdown()");
   _my->shutdown();
}

fc::variant launcher_service_plugin::get_cluster_info(launcher_service::cluster_id_param param)
{
   auto& cluster_state = _my->get_cluster_state(param.cluster_id)->second;
   std::map<int, fc::variant> result;
   bool haserror = false, hasok = false;
   const size_t nodecount = cluster_state.nodes.size();
   std::vector<std::thread> threads;
   std::vector<std::pair<int, fc::variant> > thread_results;
   std::vector<std::string> errstrs;
   thread_results.resize(nodecount);
   errstrs.resize(nodecount);
   threads.reserve(nodecount);
   int k = 0;
   bool set_errstr = true;
   for (auto& itr : cluster_state.nodes) {
      const int id = itr.second->id;
      const int port = itr.second->http_port;
      auto& thread_result = thread_results[k];
      thread_result.first = -1;
      if (port) {
         auto& errstr = errstrs[k];
         threads.push_back(std::thread([this, k, cid = param.cluster_id, id, port, &thread_result, &hasok, &haserror, &errstr]() {
            thread_result.first = id;
            try {
               thread_result.second = _my->get_info(cid, id, true);
               hasok = true;
               const auto end = time_point::now();
               return;
            } catch (boost::system::system_error& e) {
               haserror = true;
               errstr = e.what();
               std::string url = "http://" + _my->_config.host_name + ":" + _my->itoa(port);
               thread_result.second = fc::mutable_variant_object("error", e.what())("url", url);
            } catch (fc::exception&  e) {
               haserror = true;
               errstr = e.what();
               std::string url = "http://" + _my->_config.host_name + ":" + _my->itoa(port);
               thread_result.second = fc::mutable_variant_object("error", e.what())("url", url);
            }  catch (...) {
               haserror = true;
               errstr = "unknown error";
               std::string url = "http://" + _my->_config.host_name + ":" + _my->itoa(port);
               thread_result.second = fc::mutable_variant_object("error", "unknown error")("url", url);
            }
         }));
      }
      ++k;
   }
   for (auto& t: threads) {
      t.join();
   }
   for (int i = 0; i < k; ++i) {
      if (thread_results[i].first >= 0) {
         result[thread_results[i].first] = std::move(thread_results[i].second);
      }
   }
   if (!hasok && haserror) {
      for (auto& err: errstrs) {
         if (err.length())
            throw std::runtime_error(err);
      }
   }
   return fc::mutable_variant_object("result", result);
}

fc::variant launcher_service_plugin::get_cluster_running_state(launcher_service::cluster_id_param param)
{
   auto& cluster_state = _my->get_cluster_state(param.cluster_id)->second;
   for (auto& itr : cluster_state.nodes) {
      launcher_service_plugin_impl::node_state& state = *itr.second;
      if (state.child && !state.child->running()) {
         ilog("child not running, removing child pid: ${pid}, node num: ${id}", ("pid", state.pid)("id", state.id));
         const auto pid = state.pid;
         _my->_nodes_to_remove.erase(pid);
         state.set_child();
      }
      else if (state.child && state.child->running()) {
         ilog("child still running, not removing child pid: ${pid}, node num: ${id}", ("pid", state.pid)("id", state.id));
      }
   }
   return fc::mutable_variant_object("result", _my->_running_clusters[param.cluster_id]);
}

fc::variant launcher_service_plugin::get_info(launcher_service::node_id_param param) {
   return _my->get_info(param.cluster_id, param.node_id);
}

fc::variant launcher_service_plugin::launch_cluster(launcher_service::cluster_def def)
{
   _my->launch_cluster(def);
   return fc::mutable_variant_object("result", _my->_running_clusters[def.cluster_id]);
}

fc::variant launcher_service_plugin::stop_all_clusters(launcher_service::empty_param) {
   _my->stop_all_clusters();
   return fc::mutable_variant_object("result", "OK");
}

fc::variant launcher_service_plugin::stop_cluster(launcher_service::cluster_id_param param) {
   auto missed_times = _my->stop_cluster(param.cluster_id);
   vector<missed_interval> missing;
   if (missed_times.size()) {
      for (auto missed : missed_times) {
         missing.push_back({missed.first, missed.second});
      }
   }
   return fc::mutable_variant_object("result", missing);
}

fc::variant launcher_service_plugin::clean_cluster(launcher_service::cluster_id_param param) {
   _my->clean_cluster(param.cluster_id);
   return fc::mutable_variant_object("result", "OK");
}

fc::variant launcher_service_plugin::start_node(launcher_service::start_node_param param) {
   _my->start_node(param.cluster_id, param.node_id, param.extra_args);
   return fc::mutable_variant_object("result", "OK");
}

fc::variant launcher_service_plugin::stop_node(launcher_service::stop_node_param param) {
   _my->stop_node(param.cluster_id, param.node_id, param.kill_sig);
   return fc::mutable_variant_object("result", "OK");
}

fc::variant launcher_service_plugin::get_protocol_features(launcher_service::node_id_param param) {
   return _my->get_protocol_features(param.cluster_id, param.node_id);
}

fc::variant launcher_service_plugin::schedule_protocol_feature_activations(launcher_service::schedule_protocol_feature_activations_param param) {
   return _my->schedule_protocol_feature_activations(param);
}

fc::variant launcher_service_plugin::get_block(launcher_service::get_block_param param) {
   return _my->get_block(param.cluster_id, param.node_id, param.block_num_or_id);
}

fc::variant launcher_service_plugin::get_account(launcher_service::get_account_param param) {
   return _my->get_account(param);
}

fc::variant launcher_service_plugin::get_code_hash(launcher_service::get_account_param param) {
   return _my->get_code_hash(param);
}

fc::variant launcher_service_plugin::get_table_rows(launcher_service::get_table_rows_param param) {
   return _my->get_table_rows(param);
}

fc::variant launcher_service_plugin::verify_transaction(launcher_service::verify_transaction_param param) {
   return _my->verify_transaction(param);
}

fc::variant launcher_service_plugin::set_contract(launcher_service::set_contract_param param) {
   try {
      return _my->set_contract(param);
   } catch (fc::exception& e) {
      elog("FC exception: code ${code}, cluster ${cluster}, node ${node}, details: ${d}",
           ("code", e.code())("cluster", param.cluster_id)("node", param.node_id)("d", e.to_detail_string()));
      throw;
   } catch (std::exception& e) {
      elog("STD exception: cluster ${cluster}, node ${node}, details: ${d}",
           ("cluster", param.cluster_id)("node", param.node_id)("d", e.what()));
      throw;
   }
}

fc::variant launcher_service_plugin::import_keys(launcher_service::import_keys_param param) {
   return _my->import_keys(param);
}

fc::variant launcher_service_plugin::generate_key(launcher_service::generate_key_param param) {
   return _my->generate_key(param);
}

fc::variant launcher_service_plugin::push_actions(launcher_service::push_actions_param param) {
   try {
      return _my->push_actions(param);
   } catch (fc::exception& e) {
      elog("FC exception: code ${code}, cluster ${cluster}, node ${node}, details: ${d}",
           ("code", e.code())("cluster", param.cluster_id)("node", param.node_id)("d", e.to_detail_string()));
      throw;
   } catch (std::exception& e) {
      elog("STD exception: cluster ${cluster}, node ${node}, details: ${d}",
           ("cluster", param.cluster_id)("node", param.node_id)("d", e.what()));
      throw;
   }
}

fc::variant launcher_service_plugin::get_log_data(launcher_service::get_log_data_param param) {
   return _my->get_log_data(param);
}

fc::variant launcher_service_plugin::send_raw(launcher_service::send_raw_param param) {
   try {
      return _my->send_raw(param);
   } catch (fc::exception& e) {
      elog("FC exception: code ${code}, cluster ${cluster}, node ${node}, details: ${d}",
           ("code", e.code())("cluster", param.cluster_id)("node", param.node_id)("d", e.to_detail_string()));
      throw;
   } catch (std::exception& e) {
      elog("STD exception: cluster ${cluster}, node ${node}, details: ${d}",
           ("cluster", param.cluster_id)("node", param.node_id)("d", e.what()));
      throw;
   }
}

std::string launcher_service_plugin::generate_node_config(const launcher_service::launcher_config& _config, launcher_service::cluster_def& def, int node_id) {
   return launcher_service_plugin_impl::generate_node_config(_config, def, node_id);
}

}

// @ignore child, abi_cache, abi_cache_time, get_info_cache, get_info_cache_time
FC_REFLECT(eosio::launcher_service_plugin_impl::node_state, (id)(pid)(http_port)(p2p_port)(is_bios)(stdout_path)(stderr_path))

// @ignore def, imported_keys, started, transaction_blocknum
FC_REFLECT(eosio::launcher_service_plugin_impl::cluster_state, (nodes))

