/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @brief launch testnet nodes
 **/
#include <string>
#include <vector>
#include <math.h>
#include <sstream>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/program_options.hpp>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#include <boost/process/child.hpp>
#pragma GCC diagnostic pop
#include <boost/process/system.hpp>
#include <boost/process/io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <fc/crypto/private_key.hpp>
#include <fc/crypto/public_key.hpp>
#include <fc/io/json.hpp>
#include <fc/network/ip.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/log/logger_config.hpp>
#include <ifaddrs.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <eosio/chain/genesis_state.hpp>

#include "config.hpp"

using namespace std;
namespace bfs = boost::filesystem;
namespace bp = boost::process;
namespace bpo = boost::program_options;
using boost::asio::ip::tcp;
using boost::asio::ip::host_name;
using bpo::options_description;
using bpo::variables_map;
using public_key_type = fc::crypto::public_key;
using private_key_type = fc::crypto::private_key;

const string block_dir = "blocks";
const string shared_mem_dir = "state";

struct local_identity {
  vector <fc::ip::address> addrs;
  vector <string> names;

  void initialize () {
    names.push_back ("localhost");
    names.push_back ("127.0.0.1");

    boost::system::error_code ec;
    string hn = host_name (ec);
    if (ec.value() != boost::system::errc::success) {
      cerr << "unable to retrieve host name: " << ec.message() << endl;
    }
    else {
      names.push_back (hn);
      if (hn.find ('.') != string::npos) {
        names.push_back (hn.substr (0,hn.find('.')));
      }
    }

    ifaddrs *ifap = 0;
    if (::getifaddrs (&ifap) == 0) {
      for (ifaddrs *p_if = ifap; p_if != 0; p_if = p_if->ifa_next) {
        if (p_if->ifa_addr != 0 &&
            p_if->ifa_addr->sa_family == AF_INET &&
            (p_if->ifa_flags & IFF_UP) == IFF_UP) {
          sockaddr_in *ifaddr = reinterpret_cast<sockaddr_in *>(p_if->ifa_addr);
          int32_t in_addr = ntohl(ifaddr->sin_addr.s_addr);

          if (in_addr != 0) {
            fc::ip::address ifa(in_addr);
            addrs.push_back (ifa);
          }
        }
      }
      ::freeifaddrs (ifap);
    }
    else {
      cerr << "unable to query local ip interfaces" << endl;
      addrs.push_back (fc::ip::address("127.0.0.1"));
    }
  }

  bool contains (const string &name) const {
    try {
      fc::ip::address test(name);
      for (const auto &a : addrs) {
        if (a == test)
          return true;
      }
    }
    catch (...) {
      // not an ip address
      for (const auto n : names) {
        if (n == name)
          return true;
      }
    }
    return false;
  }

} local_id;

class eosd_def;

class host_def {
public:
  host_def ()
    : genesis("genesis.json"),
      ssh_identity (""),
      ssh_args (""),
      eosio_home(),
      host_name("127.0.0.1"),
      public_name("localhost"),
      listen_addr("0.0.0.0"),
      base_p2p_port(9876),
      base_http_port(8888),
      def_file_size(8192),
      instances(),
      p2p_count(0),
      http_count(0),
      dot_label_str()
  {}

  string           genesis;
  string           ssh_identity;
  string           ssh_args;
  string           eosio_home;
  string           host_name;
  string           public_name;
  string           listen_addr;
  uint16_t         base_p2p_port;
  uint16_t         base_http_port;
  uint16_t         def_file_size;
  vector<eosd_def> instances;

  uint16_t p2p_port() {
    return base_p2p_port + p2p_count++;
  }

  uint16_t http_port() {
    return base_http_port + http_count++;
  }

  uint16_t p2p_bios_port() {
    return base_p2p_port - 100;
  }

  uint16_t http_bios_port() {
     return base_http_port - 100;
  }

  bool is_local( ) {
    return local_id.contains( host_name );
  }

  const string &dot_label () {
    if (dot_label_str.empty() ) {
      mk_dot_label();
    }
    return dot_label_str;
  }


private:
  uint16_t p2p_count;
  uint16_t http_count;
  string   dot_label_str;

protected:
  void mk_dot_label() {
    if (public_name.empty()) {
      dot_label_str = host_name;
    }
    else if (boost::iequals(public_name,host_name)) {
      dot_label_str = public_name;
    }
    else
      dot_label_str = public_name + "/" + host_name;
  }
};

class tn_node_def;

class eosd_def {
public:
  eosd_def()
    : config_dir_name (),
      data_dir_name (),
      p2p_port(),
      http_port(),
      file_size(),
      has_db(false),
      name(),
      node(),
      host(),
      p2p_endpoint() {}



  string       config_dir_name;
  string       data_dir_name;
  uint16_t     p2p_port;
  uint16_t     http_port;
  uint16_t     file_size;
  bool         has_db;
  string       name;
  tn_node_def* node;
  string       host;
  string       p2p_endpoint;

  void set_host (host_def* h, bool is_bios);
  void mk_dot_label ();
  const string &dot_label () {
    if (dot_label_str.empty() ) {
      mk_dot_label();
    }
    return dot_label_str;
  }

private:
  string dot_label_str;
};

class tn_node_def {
public:
  string          name;
  vector<private_key_type> keys;
  vector<string>  peers;
  vector<string>  producers;
  eosd_def*       instance;
  string          gelf_endpoint;
};

void
eosd_def::mk_dot_label () {
  dot_label_str = name + "\\nprod=";
  if (node == 0 || node->producers.empty()) {
    dot_label_str += "<none>";
  }
  else {
    bool docomma = false;
    for (auto &prod: node->producers) {
      if (docomma)
        dot_label_str += ",";
      else
        docomma = true;
      dot_label_str += prod;
    }
  }
}

void
eosd_def::set_host( host_def* h, bool is_bios ) {
  host = h->host_name;
  p2p_port = is_bios ? h->p2p_bios_port() : h->p2p_port();
  http_port = is_bios ? h->http_bios_port() : h->http_port();
  file_size = h->def_file_size;
  p2p_endpoint = h->public_name + ":" + boost::lexical_cast<string, uint16_t>(p2p_port);
}

struct remote_deploy {
  string ssh_cmd = "/usr/bin/ssh";
  string scp_cmd = "/usr/bin/scp";
  string ssh_identity;
  string ssh_args;
  bfs::path local_config_file = "temp_config";
};

struct testnet_def {
  string name;
  remote_deploy ssh_helper;
  map <string,tn_node_def> nodes;
};

struct prodkey_def {
  string producer_name;
  public_key_type block_signing_key;
};

struct producer_set_def {
  vector<prodkey_def> schedule;
};

struct server_name_def {
  string ipaddr;
  string name;
  bool has_bios;
  string eosio_home;
  uint16_t instances;
  server_name_def () : ipaddr(), name(), has_bios(false), eosio_home(), instances(1) {}
};

struct server_identities {
  vector<server_name_def> producer;
  vector<server_name_def> nonprod;
  vector<string> db;
  string default_eosio_home;
  remote_deploy ssh;
};

struct node_rt_info {
  bool remote;
  string pid_file;
  string kill_cmd;
};

struct last_run_def {
  vector <node_rt_info> running_nodes;
};


enum class p2p_plugin {
   NET,
   BNET
};

enum launch_modes {
  LM_NONE,
  LM_LOCAL,
  LM_REMOTE,
  LM_NAMED,
  LM_ALL,
  LM_VERIFY
};

enum allowed_connection : char {
  PC_NONE = 0,
  PC_PRODUCERS = 1 << 0,
  PC_SPECIFIED = 1 << 1,
  PC_ANY = 1 << 2
};

struct launcher_def {
  bool force_overwrite;
  size_t total_nodes;
  size_t prod_nodes;
  size_t producers;
  size_t next_node;
  string shape;
  p2p_plugin p2p;
  allowed_connection allowed_connections = PC_NONE;
  bfs::path genesis;
  bfs::path output;
  bfs::path host_map_file;
  bfs::path server_ident_file;
  bfs::path stage;

  string erd;
  bfs::path config_dir_base;
  bfs::path data_dir_base;
  bool skip_transaction_signatures = false;
  string eosd_extra_args;
  testnet_def network;
  string gelf_endpoint;
  vector <string> aliases;
  vector <host_def> bindings;
  int per_host = 0;
  last_run_def last_run;
  int start_delay = 0;
  bool gelf_enabled;
  bool nogen;
  bool boot;
  bool add_enable_stale_production = false;
  string launch_name;
  string launch_time;
  server_identities servers;
  producer_set_def producer_set;
   vector<string> genesis_block;
   string start_temp;
   string start_script;

   void assign_name (eosd_def &node, bool is_bios);

  void set_options (bpo::options_description &cli);
  void initialize (const variables_map &vmap);
   void init_genesis ();
  void load_servers ();
  bool generate ();
  void define_network ();
  void bind_nodes ();
  host_def *find_host (const string &name);
  host_def *find_host_by_name_or_address (const string &name);
  host_def *deploy_config_files (tn_node_def &node);
  string compose_scp_command (const host_def &host, const bfs::path &source,
                              const bfs::path &destination);
  void write_config_file (tn_node_def &node);
  void write_logging_config_file (tn_node_def &node);
  void write_genesis_file (tn_node_def &node);
  void write_setprods_file ();
   void write_bios_boot ();

   bool   is_bios_ndx (size_t ndx);
   size_t start_ndx();
   bool   next_ndx(size_t &ndx);
   size_t skip_ndx (size_t from, size_t offset);

  void make_ring ();
  void make_star ();
  void make_mesh ();
  void make_custom ();
  void write_dot_file ();
  void format_ssh (const string &cmd, const string &host_name, string &ssh_cmd_line);
  bool do_ssh (const string &cmd, const string &host_name);
  void prep_remote_config_dir (eosd_def &node, host_def *host);
  void launch (eosd_def &node, string &gts);
  void kill (launch_modes mode, string sig_opt);
  pair<host_def, eosd_def> find_node(uint16_t node_num);
  vector<pair<host_def, eosd_def>> get_nodes(const string& node_number_list);
  void bounce (const string& node_numbers);
  void down (const string& node_numbers);
  void roll (const string& host_names);
  void start_all (string &gts, launch_modes mode);
  void ignite ();
};

void
launcher_def::set_options (bpo::options_description &cfg) {
  cfg.add_options()
    ("force,f", bpo::bool_switch(&force_overwrite)->default_value(false), "Force overwrite of existing configuration files and erase blockchain")
    ("nodes,n",bpo::value<size_t>(&total_nodes)->default_value(1),"total number of nodes to configure and launch")
    ("pnodes,p",bpo::value<size_t>(&prod_nodes)->default_value(1),"number of nodes that contain one or more producers")
    ("producers",bpo::value<size_t>(&producers)->default_value(21),"total number of non-bios producer instances in this network")
    ("mode,m",bpo::value<vector<string>>()->multitoken()->default_value({"any"}, "any"),"connection mode, combination of \"any\", \"producers\", \"specified\", \"none\"")
    ("shape,s",bpo::value<string>(&shape)->default_value("star"),"network topology, use \"star\" \"mesh\" or give a filename for custom")
    ("p2p-plugin", bpo::value<string>()->default_value("net"),"select a p2p plugin to use (either net or bnet). Defaults to net.")
    ("genesis,g",bpo::value<bfs::path>(&genesis)->default_value("./genesis.json"),"set the path to genesis.json")
    ("skip-signature", bpo::bool_switch(&skip_transaction_signatures)->default_value(false), "nodeos does not require transaction signatures.")
    ("nodeos", bpo::value<string>(&eosd_extra_args), "forward nodeos command line argument(s) to each instance of nodeos, enclose arg in quotes")
    ("delay,d",bpo::value<int>(&start_delay)->default_value(0),"seconds delay before starting each node after the first")
    ("boot",bpo::bool_switch(&boot)->default_value(false),"After deploying the nodes and generating a boot script, invoke it.")
    ("nogen",bpo::bool_switch(&nogen)->default_value(false),"launch nodes without writing new config files")
    ("host-map",bpo::value<bfs::path>(&host_map_file)->default_value(""),"a file containing mapping specific nodes to hosts. Used to enhance the custom shape argument")
    ("servers",bpo::value<bfs::path>(&server_ident_file)->default_value(""),"a file containing ip addresses and names of individual servers to deploy as producers or non-producers ")
    ("per-host",bpo::value<int>(&per_host)->default_value(0),"specifies how many nodeos instances will run on a single host. Use 0 to indicate all on one.")
    ("network-name",bpo::value<string>(&network.name)->default_value("testnet_"),"network name prefix used in GELF logging source")
    ("enable-gelf-logging",bpo::value<bool>(&gelf_enabled)->default_value(true),"enable gelf logging appender in logging configuration file")
    ("gelf-endpoint",bpo::value<string>(&gelf_endpoint)->default_value("10.160.11.21:12201"),"hostname:port or ip:port of GELF endpoint")
    ("template",bpo::value<string>(&start_temp)->default_value("testnet.template"),"the startup script template")
    ("script",bpo::value<string>(&start_script)->default_value("bios_boot.sh"),"the generated startup script name")
        ;
}

template<class enum_type, class=typename std::enable_if<std::is_enum<enum_type>::value>::type>
inline enum_type& operator|=(enum_type&lhs, const enum_type& rhs)
{
  using T = std::underlying_type_t <enum_type>;
  return lhs = static_cast<enum_type>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

void
launcher_def::initialize (const variables_map &vmap) {
  if (vmap.count("mode")) {
    const vector<string> modes = vmap["mode"].as<vector<string>>();
    for(const string&m : modes)
    {
      if (boost::iequals(m, "any"))
        allowed_connections |= PC_ANY;
      else if (boost::iequals(m, "producers"))
        allowed_connections |= PC_PRODUCERS;
      else if (boost::iequals(m, "specified"))
        allowed_connections |= PC_SPECIFIED;
      else if (boost::iequals(m, "none"))
        allowed_connections = PC_NONE;
      else {
        cerr << "unrecognized connection mode: " << m << endl;
        exit (-1);
      }
    }
  }

  using namespace std::chrono;
  system_clock::time_point now = system_clock::now();
  std::time_t now_c = system_clock::to_time_t(now);
  ostringstream dstrm;
  dstrm << std::put_time(std::localtime(&now_c), "%Y_%m_%d_%H_%M_%S");
  launch_time = dstrm.str();

  if ( ! (shape.empty() ||
          boost::iequals( shape, "ring" ) ||
          boost::iequals( shape, "star" ) ||
          boost::iequals( shape, "mesh" )) &&
       host_map_file.empty()) {
    bfs::path src = shape;
    host_map_file = src.stem().string() + "_hosts.json";
  }

  string nc = vmap["p2p-plugin"].as<string>();
  if ( !nc.empty() ) {
     if (boost::iequals(nc,"net"))
        p2p = p2p_plugin::NET;
     else if (boost::iequals(nc,"bnet"))
        p2p = p2p_plugin::BNET;
     else {
        p2p = p2p_plugin::NET;
     }
  }
  else {
     p2p = p2p_plugin::NET;
  }

  if( !host_map_file.empty() ) {
    try {
      fc::json::from_file(host_map_file).as<vector<host_def>>(bindings);
      for (auto &binding : bindings) {
        for (auto &eosd : binding.instances) {
          eosd.host = binding.host_name;
          eosd.p2p_endpoint = binding.public_name + ":" + boost::lexical_cast<string,uint16_t>(eosd.p2p_port);

          aliases.push_back (eosd.name);
        }
      }
    } catch (...) { // this is an optional feature, so an exception is OK
    }
  }

  config_dir_base = "etc/eosio";
  data_dir_base = "var/lib";
  next_node = 0;
  ++prod_nodes; // add one for the bios node
  ++total_nodes;

  load_servers ();

  if (prod_nodes > (producers + 1))
    prod_nodes = producers;
  if (prod_nodes > total_nodes)
    total_nodes = prod_nodes;

  char* erd_env_var = getenv ("EOSIO_HOME");
  if (erd_env_var == nullptr || std::string(erd_env_var).empty()) {
     erd_env_var = getenv ("PWD");
  }

  if (erd_env_var != nullptr) {
     erd = erd_env_var;
  } else {
     erd.clear();
  }

  stage = bfs::path(erd);
  if (!bfs::exists(stage)) {
    cerr << erd << " is not a valid path" << endl;
    exit (-1);
  }
  stage /= bfs::path("staging");
  bfs::create_directories (stage);
  if (bindings.empty()) {
    define_network ();
  }

}

void
launcher_def::load_servers () {
  if (!server_ident_file.empty()) {
    try {
      fc::json::from_file(server_ident_file).as<server_identities>(servers);
      prod_nodes = 0;
      for (auto &s : servers.producer) {
         prod_nodes += s.instances;
      }

      total_nodes = prod_nodes;
      for (auto &s : servers.nonprod) {
         total_nodes += s.instances;
      }

      per_host = 1;
      network.ssh_helper = servers.ssh;
    }
    catch (...) {
      cerr << "unable to load server identity file " << server_ident_file << endl;
      exit (-1);
    }
  }
}


void
launcher_def::assign_name (eosd_def &node, bool is_bios) {
   string node_cfg_name;

   if (is_bios) {
      node.name = "bios";
      node_cfg_name = "node_bios";
   }
   else {
      string dex = next_node < 10 ? "0":"";
      dex += boost::lexical_cast<string,int>(next_node++);
      node.name = network.name + dex;
      node_cfg_name = "node_" + dex;
   }
  node.config_dir_name = (config_dir_base / node_cfg_name).string();
  node.data_dir_name = (data_dir_base / node_cfg_name).string();
}

bool
launcher_def::generate () {

  if (boost::iequals (shape,"ring")) {
    make_ring ();
  }
  else if (boost::iequals (shape, "star")) {
    make_star ();
  }
  else if (boost::iequals (shape, "mesh")) {
    make_mesh ();
  }
  else {
    make_custom ();
  }

  if( !nogen ) {
     write_setprods_file();
     write_bios_boot();
     init_genesis();
     for (auto &node : network.nodes) {
        write_config_file(node.second);
        write_logging_config_file(node.second);
        write_genesis_file(node.second);
     }
  }
  write_dot_file ();

  if (!output.empty()) {
   bfs::path savefile = output;
    {
      bfs::ofstream sf (savefile);
      sf << fc::json::to_pretty_string (network) << endl;
      sf.close();
    }
    if (host_map_file.empty()) {
      savefile = bfs::path (output.stem().string() + "_hosts.json");
    }
    else {
      savefile = bfs::path (host_map_file);
    }

    {
      bfs::ofstream sf (savefile);

      sf << fc::json::to_pretty_string (bindings) << endl;
      sf.close();
    }
     return false;
  }
  return true;
}

void
launcher_def::write_dot_file () {
  bfs::ofstream df ("testnet.dot");
  df << "digraph G\n{\nlayout=\"circo\";\n";
  for (auto &node : network.nodes) {
    for (const auto &p : node.second.peers) {
      string pname=network.nodes.find(p)->second.instance->dot_label();
      df << "\"" << node.second.instance->dot_label ()
         << "\"->\"" << pname
         << "\" [dir=\"forward\"];" << std::endl;
    }
  }
  df << "}\n";
}

void
launcher_def::define_network () {

  if (per_host == 0) {
    host_def local_host;
    local_host.eosio_home = erd;
    local_host.genesis = genesis.string();
    for (size_t i = 0; i < (total_nodes); i++) {
      eosd_def eosd;
      assign_name(eosd, i == 0);
      aliases.push_back(eosd.name);
      eosd.set_host (&local_host, i == 0);
      local_host.instances.emplace_back(move(eosd));
    }
    bindings.emplace_back(move(local_host));
  }
  else {
    int ph_count = 0;
    host_def *lhost = nullptr;
    size_t host_ndx = 0;
    size_t num_prod_addr = servers.producer.size();
    size_t num_nonprod_addr = servers.nonprod.size();
    for (size_t i = total_nodes; i > 0; i--) {
       bool do_bios = false;
      if (ph_count == 0) {
        if (lhost) {
          bindings.emplace_back(move(*lhost));
          delete lhost;
        }
        lhost = new host_def;
        lhost->genesis = genesis.string();
        if (host_ndx < num_prod_addr ) {
           do_bios = servers.producer[host_ndx].has_bios;
          lhost->host_name = servers.producer[host_ndx].ipaddr;
          lhost->public_name = servers.producer[host_ndx].name;
          ph_count = servers.producer[host_ndx].instances;
        }
        else if (host_ndx - num_prod_addr < num_nonprod_addr) {
          size_t ondx = host_ndx - num_prod_addr;
           do_bios = servers.nonprod[ondx].has_bios;
          lhost->host_name = servers.nonprod[ondx].ipaddr;
          lhost->public_name = servers.nonprod[ondx].name;
          ph_count = servers.nonprod[ondx].instances;
        }
        else {
          string ext = host_ndx < 10 ? "0" : "";
          ext += boost::lexical_cast<string,int>(host_ndx);
          lhost->host_name = "pseudo_" + ext;
          lhost->public_name = lhost->host_name;
          ph_count = 1;
        }
        lhost->eosio_home =
          (local_id.contains (lhost->host_name) || servers.default_eosio_home.empty()) ?
          erd : servers.default_eosio_home;
        host_ndx++;
      } // ph_count == 0

      eosd_def eosd;
      assign_name(eosd, do_bios);
      eosd.has_db = false;

      if (servers.db.size()) {
        for (auto &dbn : servers.db) {
          if (lhost->host_name == dbn) {
            eosd.has_db = true;
            break;
         }
        }
      }
      aliases.push_back(eosd.name);
      eosd.set_host (lhost, do_bios);
      do_bios = false;
      lhost->instances.emplace_back(move(eosd));
      --ph_count;
    } // for i
    bindings.emplace_back( move(*lhost) );
    delete lhost;
  }
}


void
launcher_def::bind_nodes () {
   if (prod_nodes < 2) {
      cerr << "Unable to allocate producers due to insufficient prod_nodes = " << prod_nodes << "\n";
      exit (10);
   }
   int non_bios = prod_nodes - 1;
   int per_node = producers / non_bios;
   int extra = producers % non_bios;
   unsigned int i = 0;
   for (auto &h : bindings) {
      for (auto &inst : h.instances) {
         bool is_bios = inst.name == "bios";
         tn_node_def node;
         node.name = inst.name;
         node.instance = &inst;
         auto kp = is_bios ?
            private_key_type(string("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3")) :
            private_key_type::generate();
         auto pubkey = kp.get_public_key();
         node.keys.emplace_back (move(kp));
         if (is_bios) {
            string prodname = "eosio";
            node.producers.push_back(prodname);
            producer_set.schedule.push_back({prodname,pubkey});
         }
        else {
           if (i < non_bios) {
              int count = per_node;
              if (extra ) {
                 ++count;
                 --extra;
              }
              char ext = 'a' + i;
              string pname = "defproducer";
              while (count--) {
                 string prodname = pname+ext;
                 node.producers.push_back(prodname);
                 producer_set.schedule.push_back({prodname,pubkey});
                 ext += non_bios;
              }
           }
        }
        node.gelf_endpoint = gelf_endpoint;
        network.nodes[node.name] = move(node);
        inst.node = &network.nodes[inst.name];
        if (!is_bios) i++;
      }
  }
}

host_def *
launcher_def::find_host (const string &name)
{
  host_def *host = nullptr;
  for (auto &h : bindings) {
    if (h.host_name == name) {
      host = &h;
      break;
    }
  }
  if (host == 0) {
    cerr << "could not find host for " << name << endl;
    exit(-1);
  }
  return host;
}

host_def *
launcher_def::find_host_by_name_or_address (const string &host_id)
{
  host_def *host = nullptr;
  for (auto &h : bindings) {
    if ((h.host_name == host_id) || (h.public_name == host_id)) {
      host = &h;
      break;
    }
  }
  if (host == 0) {
    cerr << "could not find host for " << host_id << endl;
    exit(-1);
  }
  return host;
}

host_def *
launcher_def::deploy_config_files (tn_node_def &node) {
  boost::system::error_code ec;
  eosd_def &instance = *node.instance;
  host_def *host = find_host (instance.host);

  bfs::path source = stage / instance.config_dir_name / "config.ini";
  bfs::path logging_source = stage / instance.config_dir_name / "logging.json";
  bfs::path genesis_source = stage / instance.config_dir_name / "genesis.json";

  if (host->is_local()) {
    bfs::path cfgdir = bfs::path(host->eosio_home) / instance.config_dir_name;
    bfs::path dd = bfs::path(host->eosio_home) / instance.data_dir_name;

    if (!bfs::exists (cfgdir)) {
       if (!bfs::create_directories (cfgdir, ec) && ec.value()) {
          cerr << "could not create new directory: " << instance.config_dir_name
               << " errno " << ec.value() << " " << strerror(ec.value()) << endl;
          exit (-1);
       }
    }
    else if (bfs::exists (cfgdir / "config.ini") && !force_overwrite) {
       cerr << cfgdir / "config.ini" << " exists. Use -f|--force to overwrite configuration\n";
       exit (-1);
    }

    if (!bfs::exists (dd)) {
       if (!bfs::create_directories (dd, ec) && ec.value()) {
          cerr << "could not create new directory: " << instance.config_dir_name
               << " errno " << ec.value() << " " << strerror(ec.value()) << endl;
          exit (-1);
       }
    }
    else if (force_overwrite) {
       int64_t count =  bfs::remove_all (dd / block_dir, ec);
       if (ec.value() != 0) {
          cerr << "count = " << count << " could not remove old directory: " << dd
               << " " << strerror(ec.value()) << endl;
          exit (-1);
        }
        count = bfs::remove_all (dd / shared_mem_dir, ec);
        if (ec.value() != 0) {
          cerr << "count = " << count << " could not remove old directory: " << dd
               << " " << strerror(ec.value()) << endl;
          exit (-1);
        }
    }
    else if (bfs::exists (dd/ block_dir) || bfs::exists (dd / shared_mem_dir)) {
       cerr << "either " << block_dir << " or " << shared_mem_dir << " exist in \n";
       cerr << dd << ". Use -f|--force to erase blockchain data" << endl;
        exit (-1);
    }

    bfs::copy_file (genesis_source, cfgdir / "genesis.json", bfs::copy_option::overwrite_if_exists);
    bfs::copy_file (logging_source, cfgdir / "logging.json", bfs::copy_option::overwrite_if_exists);
    bfs::copy_file (source, cfgdir / "config.ini", bfs::copy_option::overwrite_if_exists);
  }
  else {
    prep_remote_config_dir (instance, host);

    bfs::path rfile = bfs::path (host->eosio_home) / instance.config_dir_name / "config.ini";
    auto scp_cmd_line = compose_scp_command(*host, source, rfile);

    cerr << "cmdline = " << scp_cmd_line << endl;
    int res = boost::process::system (scp_cmd_line);
    if (res != 0) {
      cerr << "unable to scp config file to host " << host->host_name << endl;
      exit(-1);
    }

    rfile = bfs::path (host->eosio_home) / instance.config_dir_name / "logging.json";

    scp_cmd_line = compose_scp_command(*host, logging_source, rfile);

    res = boost::process::system (scp_cmd_line);
    if (res != 0) {
      cerr << "unable to scp logging config file to host " << host->host_name << endl;
      exit(-1);
    }

    rfile = bfs::path (host->eosio_home) / instance.config_dir_name / "genesis.json";

    scp_cmd_line = compose_scp_command(*host, genesis_source, rfile);

    res = boost::process::system (scp_cmd_line);
    if (res != 0) {
       cerr << "unable to scp genesis.json file to host " << host->host_name << endl;
       exit(-1);
    }
  }
  return host;
}

string
launcher_def::compose_scp_command (const host_def& host, const bfs::path& source, const bfs::path& destination) {
  string scp_cmd_line = network.ssh_helper.scp_cmd + " ";
  const string &args = host.ssh_args.length() ? host.ssh_args : network.ssh_helper.ssh_args;
  if (args.length()) {
    scp_cmd_line += args + " ";
  }
  scp_cmd_line += source.string() + " ";

  const string &uid = host.ssh_identity.length() ? host.ssh_identity : network.ssh_helper.ssh_identity;
  if (uid.length()) {
    scp_cmd_line += uid + "@";
  }

  scp_cmd_line += host.host_name + ":" + destination.string();

  return scp_cmd_line;
}

void
launcher_def::write_config_file (tn_node_def &node) {
   bool is_bios = (node.name == "bios");
   bfs::path filename;
   eosd_def &instance = *node.instance;
   host_def *host = find_host (instance.host);

   bfs::path dd = stage / instance.config_dir_name;
   if (!bfs::exists(dd)) {
      try {
         bfs::create_directories (dd);
      } catch (const bfs::filesystem_error &ex) {
         cerr << "write_config_files threw " << ex.what() << endl;
         exit (-1);
      }
   }

   filename = dd / "config.ini";

   bfs::ofstream cfg(filename);
   if (!cfg.good()) {
      cerr << "unable to open " << filename << " " << strerror(errno) << "\n";
      exit (-1);
   }

   cfg << "blocks-dir = " << block_dir << "\n";
   cfg << "readonly = 0\n";
   cfg << "send-whole-blocks = true\n";
   cfg << "http-server-address = " << host->host_name << ":" << instance.http_port << "\n";
   if (p2p == p2p_plugin::NET) {
      cfg << "p2p-listen-endpoint = " << host->listen_addr << ":" << instance.p2p_port << "\n";
      cfg << "p2p-server-address = " << host->public_name << ":" << instance.p2p_port << "\n";
   } else {
      cfg << "bnet-endpoint = " << host->listen_addr << ":" << instance.p2p_port << "\n";
      // Include the net_plugin endpoint, because the plugin is always loaded (even if not used).
      cfg << "p2p-listen-endpoint = " << host->listen_addr << ":" << instance.p2p_port + 1000 << "\n";
   }

   if (is_bios) {
    cfg << "enable-stale-production = true\n";
  }
  if (allowed_connections & PC_ANY) {
    cfg << "allowed-connection = any\n";
  }
  else if (allowed_connections == PC_NONE) {
    cfg << "allowed-connection = none\n";
  }
  else
  {
    if (allowed_connections & PC_PRODUCERS) {
      cfg << "allowed-connection = producers\n";
    }
    if (allowed_connections & PC_SPECIFIED) {
      cfg << "allowed-connection = specified\n";
      cfg << "peer-key = \"" << string(node.keys.begin()->get_public_key()) << "\"\n";
      cfg << "peer-private-key = [\"" << string(node.keys.begin()->get_public_key())
          << "\",\"" << string(*node.keys.begin()) << "\"]\n";
    }
  }

  if(!is_bios) {
     auto &bios_node = network.nodes["bios"];
     if (p2p == p2p_plugin::NET) {
        cfg << "p2p-peer-address = " << bios_node.instance->p2p_endpoint<< "\n";
     } else {
        cfg << "bnet-connect = " << bios_node.instance->p2p_endpoint<< "\n";
     }
  }
  for (const auto &p : node.peers) {
     if (p2p == p2p_plugin::NET) {
        cfg << "p2p-peer-address = " << network.nodes.find(p)->second.instance->p2p_endpoint << "\n";
     } else {
        cfg << "bnet-connect = " << network.nodes.find(p)->second.instance->p2p_endpoint << "\n";
     }
  }
  if (instance.has_db || node.producers.size()) {
    cfg << "required-participation = 33\n";
    for (const auto &kp : node.keys ) {
       cfg << "private-key = [\"" << string(kp.get_public_key())
           << "\",\"" << string(kp) << "\"]\n";
    }
    for (auto &p : node.producers) {
      cfg << "producer-name = " << p << "\n";
    }
    cfg << "plugin = eosio::producer_plugin\n";
  }
  if( instance.has_db ) {
    cfg << "plugin = eosio::mongo_db_plugin\n";
  }
  if ( p2p == p2p_plugin::NET ) {
    cfg << "plugin = eosio::net_plugin\n";
  } else {
    cfg << "plugin = eosio::bnet_plugin\n";
  }
  cfg << "plugin = eosio::chain_api_plugin\n"
      << "plugin = eosio::history_api_plugin\n";
  cfg.close();
}

void
launcher_def::write_logging_config_file(tn_node_def &node) {
  bfs::path filename;
  eosd_def &instance = *node.instance;

  bfs::path dd = stage / instance.config_dir_name;
  if (!bfs::exists(dd)) {
    bfs::create_directories(dd);
  }

  filename = dd / "logging.json";

  bfs::ofstream cfg(filename);
  if (!cfg.good()) {
    cerr << "unable to open " << filename << " " << strerror(errno) << "\n";
    exit (9);
  }

  auto log_config = fc::logging_config::default_config();
  if(gelf_enabled) {
    log_config.appenders.push_back(
          fc::appender_config( "net", "gelf",
              fc::mutable_variant_object()
                  ( "endpoint", node.gelf_endpoint )
                  ( "host", instance.name )
             ) );
    log_config.loggers.front().appenders.push_back("net");
    fc::logger_config p2p ("net_plugin_impl");
    p2p.level=fc::log_level::debug;
    p2p.appenders.push_back ("stderr");
    p2p.appenders.push_back ("net");
    log_config.loggers.emplace_back(p2p);
  }

  auto str = fc::json::to_pretty_string( log_config, fc::json::stringify_large_ints_and_doubles );
  cfg.write( str.c_str(), str.size() );
  cfg.close();
}

void
launcher_def::init_genesis () {
  bfs::path genesis_path = bfs::current_path() / "genesis.json";
   bfs::ifstream src(genesis_path);
   if (!src.good()) {
      cout << "generating default genesis file " << genesis_path << endl;
      eosio::chain::genesis_state default_genesis;
      fc::json::save_to_file( default_genesis, genesis_path, true );
      src.open(genesis_path);
   }
   string bioskey = string(network.nodes["bios"].keys[0].get_public_key());
   string str;
   string prefix("initial_key");
   while(getline(src,str)) {
      size_t pos = str.find(prefix);
      if (pos != string::npos) {
         size_t cut = str.find("EOS",pos);
         genesis_block.push_back(str.substr(0,cut) + bioskey + "\",");
      }
      else {
         genesis_block.push_back(str);
      }
   }
}

void
launcher_def::write_genesis_file(tn_node_def &node) {
  bfs::path filename;
  eosd_def &instance = *node.instance;

  bfs::path dd = stage / instance.config_dir_name;
  if (!bfs::exists(dd)) {
    bfs::create_directories(dd);
  }

  filename = dd / "genesis.json";
  bfs::ofstream gf ( dd / "genesis.json");
  for (auto &line : genesis_block) {
     gf << line << "\n";
  }
}

void
launcher_def::write_setprods_file() {
   bfs::path filename = bfs::current_path() / "setprods.json";
   bfs::ofstream psfile (filename);
   if(!psfile.good()) {
      cerr << "unable to open " << filename << " " << strerror(errno) << "\n";
    exit (9);
  }
   producer_set_def no_bios;
   for (auto &p : producer_set.schedule) {
      if (p.producer_name != "eosio")
         no_bios.schedule.push_back(p);
   }
  auto str = fc::json::to_pretty_string( no_bios, fc::json::stringify_large_ints_and_doubles );
  psfile.write( str.c_str(), str.size() );
  psfile.close();
}

void
launcher_def::write_bios_boot () {
   bfs::ifstream src(bfs::path(config_dir_base) / "launcher" / start_temp);
   if(!src.good()) {
      cerr << "unable to open " << config_dir_base << "launcher/" << start_temp << " " << strerror(errno) << "\n";
    exit (9);
  }

   bfs::ofstream brb (bfs::current_path() / start_script);
   if(!brb.good()) {
      cerr << "unable to open " << bfs::current_path() << "/" << start_script << " " << strerror(errno) << "\n";
    exit (9);
  }

   auto &bios_node = network.nodes["bios"];
   uint16_t biosport = bios_node.instance->http_port;
   string bhost = bios_node.instance->host;
   string line;
   string prefix = "###INSERT ";
   size_t len = prefix.length();
   while (getline(src,line)) {
      if (line.substr(0,len) == prefix) {
         string key = line.substr(len);
         if (key == "envars") {
            brb << "bioshost=" << bhost << "\nbiosport=" << biosport << "\n";
         }
         else if (key == "prodkeys" ) {
            for (auto &node : network.nodes) {
               brb << "wcmd import -n ignition " << string(node.second.keys[0]) << "\n";
            }
         }
         else if (key == "cacmd") {
            for (auto &p : producer_set.schedule) {
               if (p.producer_name == "eosio") {
                  continue;
               }
               brb << "cacmd " << p.producer_name
                   << " " << string(p.block_signing_key) << " " << string(p.block_signing_key) << "\n";
            }
         }
      }
      brb << line << "\n";
   }
   src.close();
   brb.close();
}

bool launcher_def::is_bios_ndx (size_t ndx) {
   return aliases[ndx] == "bios";
}

size_t launcher_def::start_ndx() {
   return is_bios_ndx(0) ? 1 : 0;
}

bool launcher_def::next_ndx(size_t &ndx) {
   ++ndx;
   bool loop = ndx == total_nodes;
   if (loop)
      ndx = start_ndx();
   else
      if (is_bios_ndx(ndx)) {
         loop = next_ndx(ndx);
      }
   return loop;
}

size_t launcher_def::skip_ndx (size_t from, size_t offset) {
   size_t ndx = (from + offset) % total_nodes;
   if (total_nodes > 2) {
      size_t attempts = total_nodes - 1;
      while (--attempts && (is_bios_ndx(ndx) || ndx == from)) {
         next_ndx(ndx);
      }
   }
   return ndx;
}

void
launcher_def::make_ring () {
  bind_nodes();
  size_t non_bios = total_nodes - 1;
  if (non_bios > 2) {
     bool loop = false;
     for (size_t i = start_ndx(); !loop; loop = next_ndx(i)) {
        size_t front = i;
        loop = next_ndx (front);
        network.nodes.find(aliases[i])->second.peers.push_back (aliases[front]);
     }
  }
  else if (non_bios == 2) {
     size_t n0 = start_ndx();
     size_t n1 = n0;
     next_ndx(n1);
    network.nodes.find(aliases[n0])->second.peers.push_back (aliases[n1]);
    network.nodes.find(aliases[n1])->second.peers.push_back (aliases[n0]);
  }
}

void
launcher_def::make_star () {
  size_t non_bios = total_nodes - 1;
  if (non_bios < 4) {
    make_ring ();
    return;
  }
  bind_nodes();

  size_t links = 3;
  if (non_bios > 12) {
    links = static_cast<size_t>(sqrt(non_bios)) + 2;
  }
  size_t gap = non_bios > 6 ? 3 : (non_bios - links)/2 +1;
  while (non_bios % gap == 0) {
    ++gap;
  }
  // use to prevent duplicates since all connections are bidirectional
  std::map <string, std::set<string>> peers_to_from;
  bool loop = false;
  for (size_t i = start_ndx(); !loop; loop = next_ndx(i)) {
     const auto& iter = network.nodes.find(aliases[i]);
    auto &current = iter->second;
    const auto& current_name = iter->first;
    size_t ndx = i;
    for (size_t l = 1; l <= links; l++) {
       ndx = skip_ndx(ndx, l * gap);
       auto &peer = aliases[ndx];
      for (bool found = true; found; ) {
        found = false;
        for (auto &p : current.peers) {
          if (p == peer) {
             next_ndx(ndx);
             if (ndx == i) {
                next_ndx(ndx);
             }
            peer = aliases[ndx];
            found = true;
            break;
          }
        }
      }
      // if already established, don't add to list
      if (peers_to_from[peer].count(current_name) < 2) {
        current.peers.push_back(peer); // current_name -> peer
        // keep track of bidirectional relationships to prevent duplicates
        peers_to_from[current_name].insert(peer);
      }
    }
  }
}

void
launcher_def::make_mesh () {
  size_t non_bios = total_nodes - 1;
  bind_nodes();
  // use to prevent duplicates since all connections are bidirectional
  std::map <string, std::set<string>> peers_to_from;
  bool loop = false;
  for (size_t i = start_ndx();!loop; loop = next_ndx(i)) {
    const auto& iter = network.nodes.find(aliases[i]);
    auto &current = iter->second;
    const auto& current_name = iter->first;

    for (size_t j = 1; j < non_bios; j++) {
       size_t ndx = skip_ndx(i,j);
      const auto& peer = aliases[ndx];
      // if already established, don't add to list
      if (peers_to_from[peer].count(current_name) < 2) {
        current.peers.push_back (peer);
        // keep track of bidirectional relationships to prevent duplicates
        peers_to_from[current_name].insert(peer);
      }
    }
  }
}

void
launcher_def::make_custom () {
  bfs::path source = shape;
  fc::json::from_file(source).as<testnet_def>(network);
  for (auto &h : bindings) {
    for (auto &inst : h.instances) {
      tn_node_def *node = &network.nodes[inst.name];
      for (auto &p : node->producers) {
         producer_set.schedule.push_back({p,node->keys[0].get_public_key()});
      }
      node->instance = &inst;
      inst.node = node;
    }
  }
}

void
launcher_def::format_ssh (const string &cmd,
                          const string &host_name,
                          string & ssh_cmd_line) {

  ssh_cmd_line = network.ssh_helper.ssh_cmd + " ";
  if (network.ssh_helper.ssh_args.length()) {
    ssh_cmd_line += network.ssh_helper.ssh_args + " ";
  }
  if (network.ssh_helper.ssh_identity.length()) {
    ssh_cmd_line += network.ssh_helper.ssh_identity + "@";
  }
  ssh_cmd_line += host_name + " \"" + cmd + "\"";
  cerr << "cmdline = " << ssh_cmd_line << endl;
}

bool
launcher_def::do_ssh (const string &cmd, const string &host_name) {
  string ssh_cmd_line;
  format_ssh (cmd, host_name, ssh_cmd_line);
  int res = boost::process::system (ssh_cmd_line);
  return (res == 0);
}

void
launcher_def::prep_remote_config_dir (eosd_def &node, host_def *host) {
  bfs::path abs_config_dir = bfs::path(host->eosio_home) / node.config_dir_name;
  bfs::path abs_data_dir = bfs::path(host->eosio_home) / node.data_dir_name;

  string acd = abs_config_dir.string();
  string add = abs_data_dir.string();
  string cmd = "cd " + host->eosio_home;

  cmd = "cd " + host->eosio_home;
  if (!do_ssh(cmd, host->host_name)) {
    cerr << "Unable to switch to path " << host->eosio_home
         << " on host " <<  host->host_name << endl;
    exit (-1);
  }

  cmd = "cd " + acd;
  if (!do_ssh(cmd,host->host_name)) {
     cmd = "mkdir -p " + acd;
     if (!do_ssh (cmd, host->host_name)) {
        cerr << "Unable to invoke " << cmd << " on host " << host->host_name << endl;
        exit (01);
     }
  }
  cmd = "cd " + add;
  if (do_ssh(cmd,host->host_name)) {
    if(force_overwrite) {
      cmd = "rm -rf " + add + "/" + block_dir + " ;"
          + "rm -rf " + add + "/" + shared_mem_dir;
      if (!do_ssh (cmd, host->host_name)) {
        cerr << "Unable to remove old data directories on host "
             << host->host_name << endl;
        exit (-1);
      }
    }
    else {
      cerr << add << " already exists on host " << host->host_name << ".  Use -f/--force to overwrite configuration and erase blockchain" << endl;
      exit (-1);
    }
  }
  else {
    cmd = "mkdir -p " + add;
    if (!do_ssh (cmd, host->host_name)) {
      cerr << "Unable to invoke " << cmd << " on host "
           << host->host_name << endl;
      exit (-1);
    }
  }
}

void
launcher_def::launch (eosd_def &instance, string &gts) {
  bfs::path dd = instance.data_dir_name;
  bfs::path reout = dd / "stdout.txt";
  bfs::path reerr_sl = dd / "stderr.txt";
  bfs::path reerr_base = bfs::path("stderr." + launch_time + ".txt");
  bfs::path reerr = dd / reerr_base;
  bfs::path pidf  = dd / "nodeos.pid";
  host_def* host;
  try {
     host = deploy_config_files (*instance.node);
  } catch (const bfs::filesystem_error &ex) {
     cerr << "deploy_config_files threw " << ex.what() << endl;
     exit (-1);
  }

  node_rt_info info;
  info.remote = !host->is_local();

  string eosdcmd = "programs/nodeos/nodeos ";
  if (skip_transaction_signatures) {
    eosdcmd += "--skip-transaction-signatures ";
  }
  if (!eosd_extra_args.empty()) {
    if (instance.name == "bios") {
       // Strip the mongo-related options out of the bios node so
       // the plugins don't conflict between 00 and bios.
       regex r("--plugin +eosio::mongo_db_plugin");
       string args = std::regex_replace (eosd_extra_args,r,"");
       regex r2("--mongodb-uri +[^ ]+");
       args = std::regex_replace (args,r2,"");
       eosdcmd += args + " ";
    }
    else {
       eosdcmd += eosd_extra_args + " ";
    }
  }

  if( add_enable_stale_production ) {
    eosdcmd += "--enable-stale-production true ";
    add_enable_stale_production = false;
  }

  eosdcmd += " --config-dir " + instance.config_dir_name + " --data-dir " + instance.data_dir_name;
  eosdcmd += " --genesis-json " + genesis.string();
  if (gts.length()) {
    eosdcmd += " --genesis-timestamp " + gts;
  }

  if (!host->is_local()) {
    string cmdl ("cd ");
    cmdl += host->eosio_home + "; nohup " + eosdcmd + " > "
      + reout.string() + " 2> " + reerr.string() + "& echo $! > " + pidf.string()
      + "; rm -f " + reerr_sl.string()
      + "; ln -s " + reerr_base.string() + " " + reerr_sl.string();
    if (!do_ssh (cmdl, host->host_name)){
      cerr << "Unable to invoke " << cmdl
           << " on host " << host->host_name << endl;
      exit (-1);
    }

    string cmd = "cd " + host->eosio_home + "; kill -15 $(cat " + pidf.string() + ")";
    format_ssh (cmd, host->host_name, info.kill_cmd);
  }
  else {
    cerr << "spawning child, " << eosdcmd << endl;

    bp::child c(eosdcmd, bp::std_out > reout, bp::std_err > reerr );
    bfs::remove(reerr_sl);
    bfs::create_symlink (reerr_base, reerr_sl);

    bfs::ofstream pidout (pidf);
    pidout << c.id() << flush;
    pidout.close();

    info.pid_file = pidf.string();
    info.kill_cmd = "";

    if(!c.running()) {
      cerr << "child not running after spawn " << eosdcmd << endl;
      for (int i = 0; i > 0; i++) {
        if (c.running () ) break;
      }
    }
    c.detach();
  }
  last_run.running_nodes.emplace_back (move(info));
}

#if 0
void
launcher_def::kill_instance(eosd_def, string sig_opt) {
}
#endif

void
launcher_def::kill (launch_modes mode, string sig_opt) {
  switch (mode) {
  case LM_NONE:
    return;
  case LM_VERIFY:
    // no-op
    return;
  case LM_NAMED: {
    cerr << "feature not yet implemented " << endl;
    #if 0
      auto node = network.nodes.find(launch_name);
      kill_instance (node.second.instance, sig_opt);
      #endif
    break;
  }
  case LM_ALL:
  case LM_LOCAL:
  case LM_REMOTE : {
    bfs::path source = "last_run.json";
    fc::json::from_file(source).as<last_run_def>(last_run);
    for (auto &info : last_run.running_nodes) {
      if (mode == LM_ALL || (info.remote && mode == LM_REMOTE) ||
          (!info.remote && mode == LM_LOCAL)) {
        if (info.pid_file.length()) {
          string pid;
          fc::json::from_file(info.pid_file).as<string>(pid);
          string kill_cmd = "kill " + sig_opt + " " + pid;
          boost::process::system (kill_cmd);
        }
        else {
          boost::process::system (info.kill_cmd);
        }
      }
    }
  }
  }
}

pair<host_def, eosd_def>
launcher_def::find_node(uint16_t node_num) {
   string dex = node_num < 10 ? "0":"";
   dex += boost::lexical_cast<string,uint16_t>(node_num);
   string node_name = network.name + dex;
   for (const auto& host: bindings) {
      for (const auto& node: host.instances) {
         if (node_name == node.name) {
            return make_pair(host, node);
         }
      }
   }
   cerr << "Unable to find node " << node_num << endl;
   exit (-1);
}

vector<pair<host_def, eosd_def>>
launcher_def::get_nodes(const string& node_number_list) {
   vector<pair<host_def, eosd_def>> node_list;
   if (fc::to_lower(node_number_list) == "all") {
      for (auto host: bindings) {
         for (auto node: host.instances) {
            cout << "host=" << host.host_name << ", node=" << node.name << endl;
            node_list.push_back(make_pair(host, node));
         }
      }
   }
   else {
      vector<string> nodes;
      boost::split(nodes, node_number_list, boost::is_any_of(","));
      for (string node_number: nodes) {
         uint16_t node = -1;
         try {
            node = boost::lexical_cast<uint16_t,string>(node_number);
         }
         catch(boost::bad_lexical_cast &) {
            // This exception will be handled below
         }
         if (node < 0 || node > 99) {
            cerr << "Bad node number found in node number list: " << node_number << endl;
            exit(-1);
         }
         node_list.push_back(find_node(node));
      }
   }
   return node_list;
}

void
launcher_def::bounce (const string& node_numbers) {
   auto node_list = get_nodes(node_numbers);
   for (auto node_pair: node_list) {
      const host_def& host = node_pair.first;
      const eosd_def& node = node_pair.second;
      string node_num = node.name.substr( node.name.length() - 2 );
      string cmd = "cd " + host.eosio_home + "; "
                 + "export EOSIO_HOME=" + host.eosio_home + string("; ")
                 + "export EOSIO_TN_NODE=" + node_num + "; "
                 + "./scripts/eosio-tn_bounce.sh";
      cout << "Bouncing " << node.name << endl;
      if (!do_ssh(cmd, host.host_name)) {
         cerr << "Unable to bounce " << node.name << endl;
         exit (-1);
      }
   }
}

void
launcher_def::down (const string& node_numbers) {
   auto node_list = get_nodes(node_numbers);
   for (auto node_pair: node_list) {
      const host_def& host = node_pair.first;
      const eosd_def& node = node_pair.second;
      string node_num = node.name.substr( node.name.length() - 2 );
      string cmd = "cd " + host.eosio_home + "; "
                 + "export EOSIO_HOME=" + host.eosio_home + "; "
                 + "export EOSIO_TN_NODE=" + node_num + "; "
         + "export EOSIO_TN_RESTART_CONFIG_DIR=" + node.config_dir_name + "; "
                 + "./scripts/eosio-tn_down.sh";
      cout << "Taking down " << node.name << endl;
      if (!do_ssh(cmd, host.host_name)) {
         cerr << "Unable to down " << node.name << endl;
         exit (-1);
      }
   }
}

void
launcher_def::roll (const string& host_names) {
   vector<string> hosts;
   boost::split(hosts, host_names, boost::is_any_of(","));
   for (string host_name: hosts) {
      cout << "Rolling " << host_name << endl;
      auto host = find_host_by_name_or_address(host_name);
      string cmd = "cd " + host->eosio_home + "; "
                 + "export EOSIO_HOME=" + host->eosio_home + "; "
                 + "./scripts/eosio-tn_roll.sh";
      if (!do_ssh(cmd, host_name)) {
         cerr << "Unable to roll " << host << endl;
         exit (-1);
      }
   }
}

void
launcher_def::ignite() {
   if (boot) {
      cerr << "Invoking the blockchain boot script, " << start_script << "\n";
      string script("bash " + start_script);
      bp::child c(script);
      try {
         boost::system::error_code ec;
         cerr << "waiting for script completion\n";
         c.wait();
      } catch (bfs::filesystem_error &ex) {
         cerr << "wait threw error " << ex.what() << "\n";
      }
      catch (...) {
         // when script dies wait throws an exception but that is ok
      }
   } else {
      cerr << "**********************************************************************\n"
           << "run 'bash " << start_script << "' to kick off delegated block production\n"
           << "**********************************************************************\n";
   }

 }

void
launcher_def::start_all (string &gts, launch_modes mode) {
  switch (mode) {
  case LM_NONE:
    return;
  case LM_VERIFY:
    //validate configuration, report findings, exit
    return;
  case LM_NAMED : {
    try {
      add_enable_stale_production = false;
      auto node = network.nodes.find(launch_name);
      launch(*node->second.instance, gts);
    } catch (fc::exception& fce) {
       cerr << "unable to launch " << launch_name << " fc::exception=" << fce.to_detail_string() << endl;
    } catch (std::exception& stde) {
       cerr << "unable to launch " << launch_name << " std::exception=" << stde.what() << endl;
    } catch (...) {
      cerr << "Unable to launch " << launch_name << endl;
      exit (-1);
    }
    break;
  }
  case LM_ALL:
  case LM_REMOTE:
  case LM_LOCAL: {

    for (auto &h : bindings ) {
      if (mode == LM_ALL ||
          (h.is_local() ? mode == LM_LOCAL : mode == LM_REMOTE)) {
        for (auto &inst : h.instances) {
          try {
             cerr << "launching " << inst.name << endl;
             launch (inst, gts);
          } catch (fc::exception& fce) {
             cerr << "unable to launch " << inst.name << " fc::exception=" << fce.to_detail_string() << endl;
          } catch (std::exception& stde) {
             cerr << "unable to launch " << inst.name << " std::exception=" << stde.what() << endl;
          } catch (...) {
            cerr << "unable to launch " << inst.name << endl;
          }
          sleep (start_delay);
        }
      }
    }
    break;
  }
  }
  bfs::path savefile = "last_run.json";
  bfs::ofstream sf (savefile);

  sf << fc::json::to_pretty_string (last_run) << endl;
  sf.close();
}

//------------------------------------------------------------

void write_default_config(const bfs::path& cfg_file, const options_description &cfg ) {
   bfs::path parent = cfg_file.parent_path();
   if (parent.empty()) {
      parent = ".";
   }
   if(!bfs::exists(parent)) {
      try {
         bfs::create_directories(parent);
      } catch (bfs::filesystem_error &ex) {
         cerr << "could not create new directory: " << cfg_file.parent_path()
              << " caught " << ex.what() << endl;
         exit (-1);
      }
   }

   std::ofstream out_cfg( bfs::path(cfg_file).make_preferred().string());
   for(const boost::shared_ptr<bpo::option_description> od : cfg.options())
   {
      if(!od->description().empty()) {
         out_cfg << "# " << od->description() << std::endl;
      }
      boost::any store;
      if(!od->semantic()->apply_default(store))
         out_cfg << "# " << od->long_name() << " = " << std::endl;
      else {
         auto example = od->format_parameter();
         if(example.empty())
            // This is a boolean switch
            out_cfg << od->long_name() << " = " << "false" << std::endl;
         else {
            // The string is formatted "arg (=<interesting part>)"
            example.erase(0, 6);
            example.erase(example.length()-1);
            out_cfg << od->long_name() << " = " << example << std::endl;
         }
      }
      out_cfg << std::endl;
   }
   out_cfg.close();
}


int main (int argc, char *argv[]) {

  variables_map vmap;
  options_description cfg ("Testnet launcher config options");
  options_description cli ("launcher command line options");
  launcher_def top;
  string gts;
  launch_modes mode;
  string kill_arg;
  string bounce_nodes;
  string down_nodes;
  string roll_nodes;
  bfs::path config_dir;
  bfs::path config_file;

  local_id.initialize();
  top.set_options(cfg);

  cli.add_options()
    ("timestamp,i",bpo::value<string>(&gts),"set the timestamp for the first block. Use \"now\" to indicate the current time")
    ("launch,l",bpo::value<string>(), "select a subset of nodes to launch. Currently may be \"all\", \"none\", or \"local\". If not set, the default is to launch all unless an output file is named, in which case it starts none.")
    ("output,o",bpo::value<bfs::path>(&top.output),"save a copy of the generated topology in this file")
    ("kill,k", bpo::value<string>(&kill_arg),"The launcher retrieves the previously started process ids and issues a kill to each.")
    ("down", bpo::value<string>(&down_nodes),"comma-separated list of node numbers that will be taken down using the eosio-tn_down.sh script")
    ("bounce", bpo::value<string>(&bounce_nodes),"comma-separated list of node numbers that will be restarted using the eosio-tn_bounce.sh script")
    ("roll", bpo::value<string>(&roll_nodes),"comma-separated list of host names where the nodes should be rolled to a new version using the eosio-tn_roll.sh script")
    ("version,v", "print version information")
    ("help,h","print this list")
    ("config-dir", bpo::value<bfs::path>(), "Directory containing configuration files such as config.ini")
    ("config,c", bpo::value<bfs::path>()->default_value( "config.ini" ), "Configuration file name relative to config-dir");

  cli.add(cfg);

  try {
    bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
    bpo::notify(vmap);

    top.initialize(vmap);

    if (vmap.count("help") > 0) {
      cli.print(cerr);
      return 0;
    }
    if (vmap.count("version") > 0) {
      cout << eosio::launcher::config::version_str << endl;
      return 0;
    }

    if( vmap.count( "config-dir" ) ) {
      config_dir = vmap["config-dir"].as<bfs::path>();
      if( config_dir.is_relative() )
         config_dir = bfs::current_path() / config_dir;
    }

   bfs::path config_file_name = config_dir / "config.ini";
   if( vmap.count( "config" ) ) {
      config_file_name = vmap["config"].as<bfs::path>();
      if( config_file_name.is_relative() )
         config_file_name = config_dir / config_file_name;
   }

   if(!bfs::exists(config_file_name)) {
      if(config_file_name.compare(config_dir / "config.ini") != 0)
      {
         cout << "Config file " << config_file_name << " missing." << std::endl;
         return -1;
      }
      write_default_config(config_file_name, cfg);
   }


   bpo::store(bpo::parse_config_file<char>(config_file_name.make_preferred().string().c_str(),
                                           cfg, true), vmap);



    if (vmap.count("launch")) {
      string l = vmap["launch"].as<string>();
      if (boost::iequals(l,"all"))
        mode = LM_ALL;
      else if (boost::iequals(l,"local"))
        mode = LM_LOCAL;
      else if (boost::iequals(l,"remote"))
        mode = LM_REMOTE;
      else if (boost::iequals(l,"none"))
        mode = LM_NONE;
      else if (boost::iequals(l,"verify"))
        mode = LM_VERIFY;
      else {
        mode = LM_NAMED;
        top.launch_name = l;
      }
    }
    else {
      mode = !kill_arg.empty() || top.output.empty() ? LM_ALL : LM_NONE;
    }

    if (!kill_arg.empty()) {
      cout << "killing" << std::endl;
      if (kill_arg[0] != '-') {
        kill_arg = "-" + kill_arg;
      }
      top.kill (mode, kill_arg);
    }
    else if (!bounce_nodes.empty()) {
       top.bounce(bounce_nodes);
    }
    else if (!down_nodes.empty()) {
       top.down(down_nodes);
    }
    else if (!roll_nodes.empty()) {
       top.roll(roll_nodes);
    }
    else {
      top.generate();
      top.start_all(gts, mode);
      top.ignite();
    }
  } catch (bpo::unknown_option &ex) {
    cerr << ex.what() << endl;
    cli.print (cerr);
  }
  return 0;
}


//-------------------------------------------------------------

FC_REFLECT( remote_deploy,
            (ssh_cmd)(scp_cmd)(ssh_identity)(ssh_args) )

FC_REFLECT( prodkey_def,
            (producer_name)(block_signing_key))

FC_REFLECT( producer_set_def,
            (schedule))

FC_REFLECT( host_def,
            (genesis)(ssh_identity)(ssh_args)(eosio_home)
            (host_name)(public_name)
            (base_p2p_port)(base_http_port)(def_file_size)
            (instances) )

FC_REFLECT( eosd_def,
            (name)(config_dir_name)(data_dir_name)(has_db)
            (p2p_port)(http_port)(file_size) )

FC_REFLECT( tn_node_def, (name)(keys)(peers)(producers) )

FC_REFLECT( testnet_def, (name)(ssh_helper)(nodes) )

FC_REFLECT( server_name_def, (ipaddr) (name) (has_bios) (eosio_home) (instances) )

FC_REFLECT( server_identities, (producer) (nonprod) (db) (default_eosio_home) (ssh) )

FC_REFLECT( node_rt_info, (remote)(pid_file)(kill_cmd) )

FC_REFLECT( last_run_def, (running_nodes) )
