/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @brief launch testnet nodes
 **/
#include <string>
#include <vector>
#include <math.h>

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
  has_db = false;
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
  string local_config_file = "temp_config";
};

struct host_map_def {
  map <string, host_def> bindings;
};

struct testnet_def {
  remote_deploy ssh_helper;
  map <string,tn_node_def> nodes;
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
  size_t producers;
  size_t total_nodes;
  size_t prod_nodes;
  size_t next_node;
  string shape;
  allowed_connection allowed_connections = PC_NONE;
  bf::path genesis;
  bf::path output;
  bf::path host_map_file;
  string data_dir_base;
  bool skip_transaction_signatures = false;
  string eosd_extra_args;
  testnet_def network;
  string alias_base;
  vector <string> aliases;
  host_map_def host_map;
  last_run_def last_run;
  int start_delay;
  bool nogen;
  string launch_name;

  void assign_name (eosd_def &node);

  void set_options (bpo::options_description &cli);
  void initialize (const variables_map &vmap);
  bool generate ();
  void define_local ();
  void bind_nodes ();
  void write_config_file (tn_node_def &node);
  void make_ring ();
  void make_star ();
  void make_mesh ();
  void make_custom ();
  void write_dot_file ();
  void format_ssh (const string &cmd, const string &host_name, string &ssh_cmd_line);
  bool do_ssh (const string &cmd, const string &host_name);
  void prep_remote_config_dir (eosd_def &node);
  void launch (eosd_def &node, string &gts);
  void kill (launch_modes mode, string sig_opt);
  void start_all (string &gts, launch_modes mode);
};

void
launcher_def::set_options (bpo::options_description &cli) {
  cli.add_options()
    ("nodes,n",bpo::value<size_t>(&total_nodes)->default_value(1),"total number of nodes to configure and launch")
    ("pnodes,p",bpo::value<size_t>(&prod_nodes)->default_value(1),"number of nodes that are producers")
    ("mode,m",bpo::value<vector<string>>()->multitoken()->default_value({"any"}, "any"),"connection mode, combination of \"any\", \"producers\", \"specified\", \"none\"")
    ("shape,s",bpo::value<string>(&shape)->default_value("star"),"network topology, use \"star\" \"mesh\" or give a filename for custom")
    ("genesis,g",bpo::value<bf::path>(&genesis)->default_value("./genesis.json"),"set the path to genesis.json")
    ("output,o",bpo::value<bf::path>(&output),"save a copy of the generated topology in this file")
    ("skip-signature", bpo::bool_switch(&skip_transaction_signatures)->default_value(false), "EOSD does not require transaction signatures.")
    ("eosd", bpo::value<string>(&eosd_extra_args), "forward eosd command line argument(s) to each instance of eosd, enclose arg in quotes")
    ("delay,d",bpo::value<int>(&start_delay)->default_value(0),"seconds delay before starting each node after the first")
    ("nogen",bpo::bool_switch(&nogen)->default_value(false),"launch nodes without writing new config files")
    ("host-map",bpo::value<bf::path>(&host_map_file)->default_value(""),"a file containing mapping specific nodes to hosts. Used to enhance the custom shape argument")
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
      else {
        cerr << "unrecognized connection mode: " << m << endl;
        exit (-1);
      }
    }
  }

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
      fc::json::from_file(host_map_file).as<host_map_def>(host_map);
      for (auto &binding : host_map.bindings) {
        for (auto &eosd : binding.second.instances) {
          aliases.push_back (eosd.name);
        }
      }
    } catch (...) { // this is an optional feature, so an exception is OK
    }
  }

  producers = 21;
  data_dir_base = "tn_data_";
  alias_base = "testnet_";
  next_node = 0;

  if (prod_nodes > producers)
    prod_nodes = producers;
  if (prod_nodes > total_nodes)
    total_nodes = prod_nodes;

  if (host_map.bindings.empty()) {
    define_local ();
  }
}

void
launcher_def::assign_name (eosd_def &node) {
  string dex = boost::lexical_cast<string,int>(next_node++);
  node.name = alias_base + dex;
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
    }
    write_dot_file ();
  }

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

      sf << fc::json::to_pretty_string (host_map) << endl;
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
  #if 0
  // this is an experiment. I was thinking of adding a "notes" box but I
  // can't figure out hot to force it to the lower left corner of the diagram
  df << " { rank = sink;\n"
     << "   Notes  [shape=none, margin=0, label=<\n"
     << "    <TABLE BORDER=\"1\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"1\">\n"
     << "     <TR> <TD align=\"left\">Data flow between nodes is bidirectional</TD> </TR>\n"
     << "     <TR> <TD align=\"left\">Arrows point from client to server</TD> </TR>\n"
     << "    </TABLE>\n"
     << "   >];"
     << " }\n";
#endif
  df << "}\n";
}

void
launcher_def::define_local () {
  host_def local_host;
  char * erd = getenv ("EOS_ROOT_DIR");
  if (erd == 0) {
    cerr << "environment variable \"EOS_ROOT_DIR\" unset. defaulting to current dir" << endl;
    erd = getenv ("PWD");
  }
  local_host.eos_root_dir = erd;
  local_host.genesis = genesis.string();

  for (size_t i = 0; i < total_nodes; i++) {
    eosd_def eosd;

    assign_name(eosd);
    aliases.push_back(eosd.name);
    eosd.set_host (&local_host);
    local_host.instances.emplace_back(move(eosd));
  }
  host_map.bindings[local_host.host_name] = move(local_host);
}

void
launcher_def::bind_nodes () {
  int per_node = producers / prod_nodes;
  int extra = producers % prod_nodes;
  int i = 0;
  for (auto &h : host_map.bindings) {
    for (auto &inst : h.second.instances) {
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
          node.producers.push_back(pname + ext);
          ext += prod_nodes;
        }
      }
      network.nodes[node.name] = move(node);
      inst.node = &network.nodes[inst.name];
      i++;
    }
  }
}

void
launcher_def::write_config_file (tn_node_def &node) {
  bf::path filename;
  boost::system::error_code ec;
  eosd_def &instance = *node.instance;
  host_def *host = &host_map.bindings[instance.host];
  if (host->is_local()) {
    bf::path dd = bf::path(host->eos_root_dir) / instance.data_dir;
    filename = dd / "config.ini";
    if (bf::exists (dd)) {
      int64_t count =  bf::remove_all (dd, ec);
      if (ec.value() != 0) {
        cerr << "count = " << count << " could not remove old directory: " << dd
             << " " << strerror(ec.value()) << endl;
        exit (-1);
      }
    }
    if (!bf::create_directory (instance.data_dir, ec) && ec.value()) {
      cerr << "could not create new directory: " << instance.data_dir
           << " errno " << ec.value() << " " << strerror(ec.value()) << endl;
      exit (-1);
    }
  }
  else {
    filename = network.ssh_helper.local_config_file;
  }

  bf::ofstream cfg(filename);
  if (!cfg.good()) {
    cerr << "unable to open " << filename << " " << strerror(errno) << "\n";
    exit (-1);
  }

  cfg << "genesis-json = " << host->genesis << "\n"
      << "block-log-dir = blocks\n"
      << "readonly = 0\n"
      << "send-whole-blocks = true\n"
      << "shared-file-dir = blockchain\n"
      << "shared-file-size = " << instance.file_size << "\n"
      << "http-server-endpoint = " << host->host_name << ":" << instance.http_port << "\n"
      << "listen-endpoint = " << host->listen_addr << ":" << instance.p2p_port << "\n"
      << "public-endpoint = " << host->public_name << ":" << instance.p2p_port << "\n";
  if (allowed_connections & PC_ANY) {
    cfg << "allowed-connection = any\n";
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
    cfg << "remote-endpoint = " << network.nodes.find(p)->second.instance->p2p_endpoint << "\n";
  }
  if (node.producers.size()) {
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
    if( !node.producers.size() ) {
      cfg << "plugin = eosio::producer_plugin\n";
    }
    cfg << "plugin = eosio::db_plugin\n";
  }
  cfg << "plugin = eosio::chain_api_plugin\n"
      << "plugin = eosio::wallet_api_plugin\n"
      << "plugin = eosio::account_history_plugin\n"
      << "plugin = eosio::account_history_api_plugin\n";
  cfg.close();
  if (!host->is_local()) {
    prep_remote_config_dir (instance);
    string scp_cmd_line = network.ssh_helper.scp_cmd + " ";
    const string &args = host->ssh_args.length() ? host->ssh_args : network.ssh_helper.ssh_args;
    if (args.length()) {
      scp_cmd_line += args + " ";
    }
    scp_cmd_line += filename.string() + " ";

    const string &uid = host->ssh_identity.length() ? host->ssh_identity : network.ssh_helper.ssh_identity;
    if (uid.length()) {
      scp_cmd_line += uid + "@";
    }

    bf::path dpath = bf::path (host->eos_root_dir) / instance.data_dir / "config.ini";
    scp_cmd_line += host->host_name + ":" + dpath.string();

    cerr << "cmdline = " << scp_cmd_line << endl;
    int res = boost::process::system (scp_cmd_line);
    if (res != 0) {
      cerr << "unable to scp config file to host " << host->host_name << endl;
      exit(-1);
    }
  }
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
    links = (size_t)sqrt(total_nodes);
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
      if (peers_to_from[peer].count(current_name) == 0) {
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
      if (peers_to_from[peer].count(current_name) == 0) {
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
  for (auto &h : host_map.bindings) {
    for (auto &inst : h.second.instances) {
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
launcher_def::prep_remote_config_dir (eosd_def &node) {
  host_def* host = &host_map.bindings[node.host];
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
    cmd = "rm -rf " + add + "/block*";
    if (!do_ssh (cmd, host->host_name)) {
      cerr << "Unable to remove old data directories on host "
           << host->host_name << endl;
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
launcher_def::launch (eosd_def &node, string &gts) {
  bf::path dd = node.data_dir;
  bf::path reout = dd / "stdout.txt";
  bf::path reerr = dd / "stderr.txt";
  bf::path pidf  = dd / "eosd.pid";

  host_def* host = &host_map.bindings[node.host];

  node_rt_info info;
  info.remote = !host->is_local();

  string eosdcmd = "programs/eosd/eosd ";
  if (skip_transaction_signatures) {
    eosdcmd += "--skip-transaction-signatures ";
  }
  eosdcmd += eosd_extra_args + " ";
  eosdcmd += "--enable-stale-production true ";
  eosdcmd += "--data-dir " + node.data_dir;
  if (gts.length()) {
    eosdcmd += " --genesis-timestamp " + gts;
  }

  if (info.remote) {
    string cmdl ("cd ");
    cmdl += host->eos_root_dir + "; nohup " + eosdcmd + " > "
      + reout.string() + " 2> " + reerr.string() + "& echo $! > " + pidf.string();
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

void
launcher_def::kill (launch_modes mode, string sig_opt) {
  if (mode == LM_NONE) {
    return;
  }
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
      auto node = network.nodes.find(launch_name);
      launch(*node->second.instance, gts);
    } catch (...) {
      cerr << "Unable to launch " << launch_name << endl;
      exit (-1);
    }
    break;
  }
  case LM_ALL:
  case LM_REMOTE:
  case LM_LOCAL: {
    for (auto &h : host_map.bindings ) {
      if (mode == LM_ALL ||
          (h.second.is_local() ? mode == LM_LOCAL : mode == LM_REMOTE)) {
        for (auto &inst : h.second.instances) {
          try {
            launch (inst, gts);
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

  local_id.initialize();
  top.set_options(opts);

  opts.add_options()
    ("timestamp,i",bpo::value<string>(&gts),"set the timestamp for the first block. Use \"now\" to indicate the current time")
    ("launch,l",bpo::value<string>(), "select a subset of nodes to launch. Currently may be \"all\", \"none\", or \"local\". If not set, the default is to launch all unless an output file is named, in which case it starts none.")
    ("kill,k", bpo::value<string>(&kill_arg),"The launcher retrieves the previously started process ids and issue a kill signal to each.")
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
        cerr << "unrecognized launch mode: " << l << endl;
        exit (-1);
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
            (name)(data_dir)(has_db)(host)(p2p_endpoint)
            (p2p_port)(http_port)(file_size) )

FC_REFLECT( tn_node_def, (name)(keys)(peers)(producers) )

FC_REFLECT( testnet_def, (ssh_helper)(nodes) )

FC_REFLECT( host_map_def, (bindings) )

FC_REFLECT( node_rt_info, (remote)(pid_file)(kill_cmd) )

FC_REFLECT( last_run_def, (running_nodes) )
