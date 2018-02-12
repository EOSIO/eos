/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @brief launch testnet nodes
 **/
#include <string>
#include <vector>
#include <math.h>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/program_options.hpp>
#include <boost/process/child.hpp>
#include <boost/process/system.hpp>
#include <boost/process/io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <fc/io/json.hpp>
#include <fc/network/ip.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/log/logger_config.hpp>
#include <ifaddrs.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>

#include "config.hpp"

using namespace std;
namespace bf = boost::filesystem;
namespace bp = boost::process;
namespace bpo = boost::program_options;
using boost::asio::ip::tcp;
using boost::asio::ip::host_name;
using bpo::options_description;
using bpo::variables_map;

const string block_dir = "blocks";
const string shared_mem_dir = "shared_mem";

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


struct keypair {
  string public_key;
  string wif_private_key;

  keypair ()
    : public_key("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"),
      wif_private_key("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3")
  {}
};

class eosd_def;

class host_def {
public:
  host_def ()
    : genesis("./genesis.json"),
      ssh_identity (""),
      ssh_args (""),
      eos_root_dir(),
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
  string           eos_root_dir;
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
    : data_dir (),
      p2p_port(),
      http_port(),
      file_size(),
      has_db(false),
      name(),
      node(),
      host(),
      p2p_endpoint() {}


  string       data_dir;
  uint16_t     p2p_port;
  uint16_t     http_port;
  uint16_t     file_size;
  bool         has_db;
  string       name;
  tn_node_def* node;
  string       host;
  string       p2p_endpoint;

  void set_host (host_def* h);
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
  vector<keypair> keys;
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
eosd_def::set_host( host_def* h ) {
  host = h->host_name;
  p2p_port = h->p2p_port();
  http_port = h->http_port();
  file_size = h->def_file_size;
  p2p_endpoint = h->public_name + ":" + boost::lexical_cast<string, uint16_t>(p2p_port);
}

struct remote_deploy {
  string ssh_cmd = "/usr/bin/ssh";
  string scp_cmd = "/usr/bin/scp";
  string ssh_identity;
  string ssh_args;
  bf::path local_config_file = "temp_config";
};

struct testnet_def {
  string name;
  remote_deploy ssh_helper;
  map <string,tn_node_def> nodes;
};


struct server_name_def {
  string ipaddr;
  string name;
  uint16_t instances;
  server_name_def () : ipaddr(), name(), instances(1) {}
};

struct server_identities {
  vector<server_name_def> producer;
  vector<server_name_def> observer;
  vector<string> db;
  string remote_eos_root;
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
  size_t producers;
  size_t total_nodes;
  size_t prod_nodes;
  size_t next_node;
  string shape;
  allowed_connection allowed_connections = PC_NONE;
  bf::path genesis;
  bf::path output;
  bf::path host_map_file;
  bf::path server_ident_file;
  bf::path stage;

  string erd;
  string data_dir_base;
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
  bool add_enable_stale_production = false;
  string launch_name;
  string launch_time;
  server_identities servers;

  void assign_name (eosd_def &node);

  void set_options (bpo::options_description &cli);
  void initialize (const variables_map &vmap);
  void load_servers ();
  bool generate ();
  void define_network ();
  void bind_nodes ();
  host_def *find_host (const string &name);
  host_def *find_host_by_name_or_address (const string &name);
  host_def *deploy_config_files (tn_node_def &node);
  string compose_scp_command (const host_def &host, const bf::path &source,
                              const bf::path &destination);
  void write_config_file (tn_node_def &node);
  void write_logging_config_file (tn_node_def &node);
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
};

void
launcher_def::set_options (bpo::options_description &cli) {
  cli.add_options()
    ("force,f", bpo::bool_switch(&force_overwrite)->default_value(false), "Force overwrite of existing configuration files and erase blockchain")
    ("nodes,n",bpo::value<size_t>(&total_nodes)->default_value(1),"total number of nodes to configure and launch")
    ("pnodes,p",bpo::value<size_t>(&prod_nodes)->default_value(1),"number of nodes that are producers")
    ("mode,m",bpo::value<vector<string>>()->multitoken()->default_value({"any"}, "any"),"connection mode, combination of \"any\", \"producers\", \"specified\", \"none\"")
    ("shape,s",bpo::value<string>(&shape)->default_value("star"),"network topology, use \"star\" \"mesh\" or give a filename for custom")
    ("genesis,g",bpo::value<bf::path>(&genesis)->default_value("./genesis.json"),"set the path to genesis.json")
    ("output,o",bpo::value<bf::path>(&output),"save a copy of the generated topology in this file")
    ("skip-signature", bpo::bool_switch(&skip_transaction_signatures)->default_value(false), "EOSD does not require transaction signatures.")
    ("eosiod", bpo::value<string>(&eosd_extra_args), "forward eosiod command line argument(s) to each instance of eosiod, enclose arg in quotes")
    ("delay,d",bpo::value<int>(&start_delay)->default_value(0),"seconds delay before starting each node after the first")
    ("nogen",bpo::bool_switch(&nogen)->default_value(false),"launch nodes without writing new config files")
    ("host-map",bpo::value<bf::path>(&host_map_file)->default_value(""),"a file containing mapping specific nodes to hosts. Used to enhance the custom shape argument")
    ("servers",bpo::value<bf::path>(&server_ident_file)->default_value(""),"a file containing ip addresses and names of individual servers to deploy as producers or observers ")
    ("per-host",bpo::value<int>(&per_host)->default_value(0),"specifies how many eosiod instances will run on a single host. Use 0 to indicate all on one.")
    ("network-name",bpo::value<string>(&network.name)->default_value("testnet_"),"network name prefix used in GELF logging source")
    ("enable-gelf-logging",bpo::value<bool>(&gelf_enabled)->default_value(true),"enable gelf logging appender in logging configuration file")
    ("gelf-endpoint",bpo::value<string>(&gelf_endpoint)->default_value("10.160.11.21:12201"),"hostname:port or ip:port of GELF endpoint")
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
    add_enable_stale_production = true;
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
    bf::path src = shape;
    host_map_file = src.stem().string() + "_hosts.json";
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

  producers = 1; // TODO: for now we are restricted to the one eosio producer
  data_dir_base = "tn_data_";
  next_node = 0;

  load_servers ();

  if (prod_nodes > producers)
    prod_nodes = producers;
  if (prod_nodes > total_nodes)
    total_nodes = prod_nodes;

  char* erd_env_var = getenv ("EOS_ROOT_DIR");
  if (erd_env_var == nullptr || std::string(erd_env_var).empty()) {
     erd_env_var = getenv ("PWD");
  }

  if (erd_env_var != nullptr) {
     erd = erd_env_var;
  } else {
     erd.clear();
  }

  stage = bf::path(erd);
  if (!bf::exists(stage)) {
    cerr << erd << " is not a valid path" << endl;
    exit (-1);
  }
  stage /= bf::path("staging");
  bf::create_directory (stage);

  if (bindings.empty()) {
    define_network ();
  }

}

void
launcher_def::load_servers () {
  if (!server_ident_file.empty()) {
    try {
      fc::json::from_file(server_ident_file).as<server_identities>(servers);
      size_t nodes = 0;
      for (auto &s : servers.producer) {
        nodes += s.instances;
      }
      prod_nodes = nodes;
      nodes = 0;
      for (auto &s : servers.observer) {
        nodes += s.instances;
      }

      total_nodes = prod_nodes + nodes;
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
launcher_def::assign_name (eosd_def &node) {
  string dex = next_node < 10 ? "0":"";
  dex += boost::lexical_cast<string,int>(next_node++);
  node.name = network.name + dex;
  node.data_dir = data_dir_base + dex;
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
    for (auto &node : network.nodes) {
      write_config_file(node.second);
      write_logging_config_file(node.second);
    }
  }
  write_dot_file ();

  if (!output.empty()) {
   bf::path savefile = output;
    {
      bf::ofstream sf (savefile);

      sf << fc::json::to_pretty_string (network) << endl;
      sf.close();
    }
    if (host_map_file.empty()) {
      savefile = bf::path (output.stem().string() + "_hosts.json");
    }
    else {
      savefile = bf::path (host_map_file);
    }

    {
      bf::ofstream sf (savefile);

      sf << fc::json::to_pretty_string (bindings) << endl;
      sf.close();
    }
     return false;
  }
  return true;
}

void
launcher_def::write_dot_file () {
  bf::ofstream df ("testnet.dot");
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
    local_host.eos_root_dir = erd;
    local_host.genesis = genesis.string();

    for (size_t i = 0; i < total_nodes; i++) {
      eosd_def eosd;

      assign_name(eosd);
      aliases.push_back(eosd.name);
      eosd.set_host (&local_host);
      local_host.instances.emplace_back(move(eosd));
    }
    bindings.emplace_back(move(local_host));
  }
  else {
    int ph_count = 0;
    host_def *lhost = nullptr;
    size_t host_ndx = 0;
    size_t num_prod_addr = servers.producer.size();
    size_t num_observer_addr = servers.observer.size();
    for (size_t i = total_nodes; i > 0; i--) {
      if (ph_count == 0) {
        if (lhost) {
          bindings.emplace_back(move(*lhost));
          delete lhost;
        }
        lhost = new host_def;
        lhost->genesis = genesis.string();
        if (host_ndx < num_prod_addr ) {
          lhost->host_name = servers.producer[host_ndx].ipaddr;
          lhost->public_name = servers.producer[host_ndx].name;
          ph_count = servers.producer[host_ndx].instances;
        }
        else if (host_ndx - num_prod_addr < num_observer_addr) {
          size_t ondx = host_ndx - num_prod_addr;
          lhost->host_name = servers.observer[ondx].ipaddr;
          lhost->public_name = servers.observer[ondx].name;
          ph_count = servers.observer[ondx].instances;
        }
        else {
          string ext = host_ndx < 10 ? "0" : "";
          ext += boost::lexical_cast<string,int>(host_ndx);
          lhost->host_name = "pseudo_" + ext;
          lhost->public_name = lhost->host_name;
          ph_count = 1;
        }
        lhost->eos_root_dir =
          (local_id.contains (lhost->host_name) || servers.remote_eos_root.empty()) ?
          erd : servers.remote_eos_root;

        host_ndx++;
      }
      eosd_def eosd;

      assign_name(eosd);
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
      eosd.set_host (lhost);
      lhost->instances.emplace_back(move(eosd));
      --ph_count;
    }
    bindings.emplace_back( move(*lhost) );
    delete lhost;
  }
}


void
launcher_def::bind_nodes () {
  int per_node = producers / prod_nodes;
  int extra = producers % prod_nodes;
  unsigned int i = 0;
  for (auto &h : bindings) {
    for (auto &inst : h.instances) {
        tn_node_def node;
        node.name = inst.name;
        node.instance = &inst;
        keypair kp;
        node.keys.emplace_back (move(kp));
        if (i < prod_nodes) {
          int count = per_node;
          if (extra) {
            ++count;
            --extra;
          }
          char ext = 'a' + i;
          string pname = "init";
          while (count--) {
            // TODO: for now we are restricted to the one eosio producer
            node.producers.push_back("eosio");//pname + ext);
            ext += prod_nodes;
          }
        }
        node.gelf_endpoint = gelf_endpoint;
        network.nodes[node.name] = move(node);
        inst.node = &network.nodes[inst.name];
        i++;
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

  bf::path source = stage / instance.data_dir / "config.ini";
  bf::path logging_source = stage / instance.data_dir / "logging.json";
  if (host->is_local()) {
    bf::path dd = bf::path(host->eos_root_dir) / instance.data_dir;
    if (bf::exists (dd)) {
      if (force_overwrite) {
        int64_t count =  bf::remove_all (dd / block_dir, ec);
        if (ec.value() != 0) {
          cerr << "count = " << count << " could not remove old directory: " << dd
               << " " << strerror(ec.value()) << endl;
          exit (-1);
        }
        count = bf::remove_all (dd / shared_mem_dir, ec);
        if (ec.value() != 0) {
          cerr << "count = " << count << " could not remove old directory: " << dd
               << " " << strerror(ec.value()) << endl;
          exit (-1);
        }
      }
      else {
        cerr << dd << " already exists.  Use -f/--force to overwrite configuration and erase blockchain" << endl;
        exit (-1);
      }
    } else if (!bf::create_directory (instance.data_dir, ec) && ec.value()) {
      cerr << "could not create new directory: " << instance.data_dir
           << " errno " << ec.value() << " " << strerror(ec.value()) << endl;
      exit (-1);
    }
    bf::copy_file (logging_source, dd / "logging.json", bf::copy_option::overwrite_if_exists);
    bf::copy_file (source, dd / "config.ini", bf::copy_option::overwrite_if_exists);
  }
  else {
    prep_remote_config_dir (instance, host);

    bf::path dpath = bf::path (host->eos_root_dir) / instance.data_dir / "config.ini";

    auto scp_cmd_line = compose_scp_command(*host, source, dpath);

    cerr << "cmdline = " << scp_cmd_line << endl;
    int res = boost::process::system (scp_cmd_line);
    if (res != 0) {
      cerr << "unable to scp config file to host " << host->host_name << endl;
      exit(-1);
    }

    dpath = bf::path (host->eos_root_dir) / instance.data_dir / "logging.json";

    scp_cmd_line = compose_scp_command(*host, logging_source, dpath);

    res = boost::process::system (scp_cmd_line);
    if (res != 0) {
      cerr << "unable to scp logging config file to host " << host->host_name << endl;
      exit(-1);
    }
  }
  return host;
}

string
launcher_def::compose_scp_command (const host_def& host, const bf::path& source, const bf::path& destination) {
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
  bf::path filename;
  eosd_def &instance = *node.instance;
  host_def *host = find_host (instance.host);

  bf::path dd = stage / instance.data_dir;
  if (!bf::exists(dd)) {
    bf::create_directory(dd);
  }

  filename = dd / "config.ini";

  bf::ofstream cfg(filename);
  if (!cfg.good()) {
    cerr << "unable to open " << filename << " " << strerror(errno) << "\n";
    exit (-1);
  }

  cfg << "genesis-json = " << host->genesis << "\n"
      << "block-log-dir = " << block_dir << "\n"
      << "readonly = 0\n"
      << "send-whole-blocks = true\n"
      << "http-server-address = " << host->host_name << ":" << instance.http_port << "\n"
      << "p2p-listen-endpoint = " << host->listen_addr << ":" << instance.p2p_port << "\n"
      << "p2p-server-address = " << host->public_name << ":" << instance.p2p_port << "\n";
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
      cfg << "peer-key = \"" << node.keys.begin()->public_key << "\"\n";
      cfg << "peer-private-key = [\"" << node.keys.begin()->public_key
          << "\",\"" << node.keys.begin()->wif_private_key << "\"]\n";
    }
  }
  for (const auto &p : node.peers) {
    cfg << "p2p-peer-address = " << network.nodes.find(p)->second.instance->p2p_endpoint << "\n";
  }
  if (instance.has_db || node.producers.size()) {
    cfg << "required-participation = true\n";
    for (const auto &kp : node.keys ) {
      cfg << "private-key = [\"" << kp.public_key
          << "\",\"" << kp.wif_private_key << "\"]\n";
    }
    for (auto &p : node.producers) {
      cfg << "producer-name = " << p << "\n";
    }
    cfg << "plugin = eosio::producer_plugin\n";
  }
  if( instance.has_db ) {
    cfg << "plugin = eosio::mongo_db_plugin\n";
  }
  cfg << "plugin = eosio::chain_api_plugin\n"
      << "plugin = eosio::account_history_api_plugin\n";
  cfg.close();
}

void
launcher_def::write_logging_config_file(tn_node_def &node) {
  bf::path filename;
  eosd_def &instance = *node.instance;

  bf::path dd = stage/ instance.data_dir;
  if (!bf::exists(dd)) {
    bf::create_directory(dd);
  }

  filename = dd / "logging.json";

  bf::ofstream cfg(filename);
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
  }

  auto str = fc::json::to_pretty_string( log_config, fc::json::stringify_large_ints_and_doubles );
  cfg.write( str.c_str(), str.size() );
  cfg.close();
}

void
launcher_def::make_ring () {
  bind_nodes();
  if (total_nodes > 2) {
    for (size_t i = 0; i < total_nodes; i++) {
      size_t front = (i + 1) % total_nodes;
      network.nodes.find(aliases[i])->second.peers.push_back (aliases[front]);
    }
  }
  else if (total_nodes == 2) {
    network.nodes.find(aliases[0])->second.peers.push_back (aliases[1]);
    network.nodes.find(aliases[1])->second.peers.push_back (aliases[0]);
  }
}

void
launcher_def::make_star () {
  bind_nodes();
  if (total_nodes < 4) {
    make_ring ();
    return;
  }

  size_t links = 3;
  if (total_nodes > 12) {
    links = static_cast<size_t>(sqrt(total_nodes)) + 2;
  }
  size_t gap = total_nodes > 6 ? 3 : (total_nodes - links)/2 +1;
  while (total_nodes % gap == 0) {
    ++gap;
  }
  // use to prevent duplicates since all connections are bidirectional
  std::map <string, std::set<string>> peers_to_from;
  for (size_t i = 0; i < total_nodes; i++) {
    const auto& iter = network.nodes.find(aliases[i]);
    auto &current = iter->second;
    const auto& current_name = iter->first;
    for (size_t l = 1; l <= links; l++) {
      size_t ndx = (i + l * gap) % total_nodes;
      if (i == ndx) {
        ++ndx;
        if (ndx == total_nodes) {
          ndx = 0;
        }
      }
      auto &peer = aliases[ndx];
      for (bool found = true; found; ) {
        found = false;
        for (auto &p : current.peers) {
          if (p == peer) {
            ++ndx;
            if (ndx == total_nodes) {
              ndx = 0;
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
  bind_nodes();
  // use to prevent duplicates since all connections are bidirectional
  std::map <string, std::set<string>> peers_to_from;
  for (size_t i = 0; i < total_nodes; i++) {
    const auto& iter = network.nodes.find(aliases[i]);
    auto &current = iter->second;
    const auto& current_name = iter->first;
    for (size_t j = 1; j < total_nodes; j++) {
      size_t ndx = (i + j) % total_nodes;
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
  bf::path source = shape;
  fc::json::from_file(source).as<testnet_def>(network);
  for (auto &h : bindings) {
    for (auto &inst : h.instances) {
      tn_node_def *node = &network.nodes[inst.name];
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
  bf::path abs_data_dir = bf::path(host->eos_root_dir) / node.data_dir;
  string add = abs_data_dir.string();
  string cmd = "cd " + host->eos_root_dir;
  if (!do_ssh(cmd, host->host_name)) {
    cerr << "Unable to switch to path " << host->eos_root_dir
         << " on host " <<  host->host_name << endl;
    exit (-1);
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
    cmd = "mkdir " + add;
    if (!do_ssh (cmd, host->host_name)) {
      cerr << "Unable to invoke " << cmd << " on host "
           << host->host_name << endl;
      exit (-1);
    }
  }
}

void
launcher_def::launch (eosd_def &instance, string &gts) {
  bf::path dd = instance.data_dir;
  bf::path reout = dd / "stdout.txt";
  bf::path reerr_sl = dd / "stderr.txt";
  bf::path reerr_base = bf::path("stderr." + launch_time + ".txt");
  bf::path reerr = dd / reerr_base;
  bf::path pidf  = dd / "eosiod.pid";

  host_def* host = deploy_config_files (*instance.node);
  node_rt_info info;
  info.remote = !host->is_local();

  string eosdcmd = "programs/eosiod/eosiod ";
  if (skip_transaction_signatures) {
    eosdcmd += "--skip-transaction-signatures ";
  }
  if (!eosd_extra_args.empty()) {
    eosdcmd += eosd_extra_args + " ";
  }

  if( add_enable_stale_production ) {
    eosdcmd += "--enable-stale-production true ";
    add_enable_stale_production = false;
  }

  eosdcmd += "--data-dir " + instance.data_dir;
  if (gts.length()) {
    eosdcmd += " --genesis-timestamp " + gts;
  }

  if (!host->is_local()) {
    string cmdl ("cd ");
    cmdl += host->eos_root_dir + "; nohup " + eosdcmd + " > "
      + reout.string() + " 2> " + reerr.string() + "& echo $! > " + pidf.string()
      + "; rm -f " + reerr_sl.string()
      + "; ln -s " + reerr_base.string() + " " + reerr_sl.string();
    if (!do_ssh (cmdl, host->host_name)){
      cerr << "Unable to invoke " << cmdl
           << " on host " << host->host_name << endl;
      exit (-1);
    }

    string cmd = "cd " + host->eos_root_dir + "; kill -9 `cat " + pidf.string() + "`";
    format_ssh (cmd, host->host_name, info.kill_cmd);
  }
  else {
    cerr << "spawning child, " << eosdcmd << endl;

    bp::child c(eosdcmd, bp::std_out > reout, bp::std_err > reerr );
    bf::remove(reerr_sl);
    bf::create_symlink (reerr_base, reerr_sl);

    bf::ofstream pidout (pidf);
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
    bf::path source = "last_run.json";
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
      string cmd = "cd " + host.eos_root_dir + "; "
                 + "export EOSIO_HOME=" + host.eos_root_dir + string("; ")
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
      string cmd = "cd " + host.eos_root_dir + "; "
                 + "export EOSIO_HOME=" + host.eos_root_dir + "; "
                 + "export EOSIO_TN_NODE=" + node_num + "; "
                 + "export EOSIO_TN_RESTART_DATA_DIR=" + node.data_dir + "; "
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
      string cmd = "cd " + host->eos_root_dir + "; "
                 + "export EOSIO_HOME=" + host->eos_root_dir + "; "
                 + "./scripts/eosio-tn_roll.sh";
      if (!do_ssh(cmd, host_name)) {
         cerr << "Unable to roll " << host << endl;
         exit (-1);
      }
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
  bf::path savefile = "last_run.json";
  bf::ofstream sf (savefile);

  sf << fc::json::to_pretty_string (last_run) << endl;
  sf.close();
}

//------------------------------------------------------------

int main (int argc, char *argv[]) {

  variables_map vmap;
  options_description opts ("Testnet launcher options");
  launcher_def top;
  string gts;
  launch_modes mode;
  string kill_arg;
  string bounce_nodes;
  string down_nodes;
  string roll_nodes;

  local_id.initialize();
  top.set_options(opts);

  opts.add_options()
    ("timestamp,i",bpo::value<string>(&gts),"set the timestamp for the first block. Use \"now\" to indicate the current time")
    ("launch,l",bpo::value<string>(), "select a subset of nodes to launch. Currently may be \"all\", \"none\", or \"local\". If not set, the default is to launch all unless an output file is named, in which case it starts none.")
    ("kill,k", bpo::value<string>(&kill_arg),"The launcher retrieves the previously started process ids and issues a kill to each.")
    ("down", bpo::value<string>(&down_nodes),"comma-separated list of node numbers that will be taken down using the eosio-tn_down.sh script")
    ("bounce", bpo::value<string>(&bounce_nodes),"comma-separated list of node numbers that will be restarted using the eosio-tn_bounce.sh script")
    ("roll", bpo::value<string>(&roll_nodes),"comma-separated list of host names where the nodes should be rolled to a new version using the eosio-tn_roll.sh script")
    ("version,v", "print version information")
    ("help,h","print this list");


  try {
    bpo::store(bpo::parse_command_line(argc, argv, opts), vmap);
    bpo::notify(vmap);

    top.initialize(vmap);

    if (vmap.count("help") > 0) {
      opts.print(cerr);
      return 0;
    }
    if (vmap.count("version") > 0) {
      cout << eosio::launcher::config::version_str << endl;
      return 0;
    }

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
    }
  } catch (bpo::unknown_option &ex) {
    cerr << ex.what() << endl;
    opts.print (cerr);
  }
  return 0;
}


//-------------------------------------------------------------


FC_REFLECT( keypair, (public_key)(wif_private_key) )

FC_REFLECT( remote_deploy,
            (ssh_cmd)(scp_cmd)(ssh_identity)(ssh_args) )

FC_REFLECT( host_def,
            (genesis)(ssh_identity)(ssh_args)(eos_root_dir)
            (host_name)(public_name)
            (base_p2p_port)(base_http_port)(def_file_size)
            (instances) )

FC_REFLECT( eosd_def,
            (name)(data_dir)(has_db)
            (p2p_port)(http_port)(file_size) )

FC_REFLECT( tn_node_def, (name)(keys)(peers)(producers) )

FC_REFLECT( testnet_def, (name)(ssh_helper)(nodes) )

FC_REFLECT( server_name_def, (ipaddr) (name) (instances) )

FC_REFLECT( server_identities, (producer) (observer) (db) (remote_eos_root) (ssh) )

FC_REFLECT( node_rt_info, (remote)(pid_file)(kill_cmd) )

FC_REFLECT( last_run_def, (running_nodes) )
