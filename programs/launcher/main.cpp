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

struct localIdentity {
  vector <fc::ip::address> addrs;
  vector <string> names;

  void initialize () {
    names.push_back ("localhost");
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

struct eosd_def {
  eosd_def ()
    : genesis("genesis.json"),
      remote(false),
      ssh_identity (""),
      ssh_args (""),
      eos_root_dir(),
      data_dir(),
      hostname("127.0.0.1"),
      public_name("localhost"),
      p2p_port(9876),
      http_port(8888),
      filesize (8192),
      keys(),
      peers(),
      producers(),
      onhost_set(false),
      onhost(true),
      localaddrs(),
      dot_alias_str()
  {}

  bool on_host () {
    if (! onhost_set) {
      onhost_set = true;
      onhost = local_id.contains (hostname);
    }
    return onhost;
  }

  string p2p_endpoint () {
    if (p2p_endpoint_str.empty()) {
      p2p_endpoint_str = public_name + ":" + boost::lexical_cast<string, uint16_t>(p2p_port);
    }
    return p2p_endpoint_str;
  }

 const string &dot_alias (const string &name) {
    if (dot_alias_str.empty()) {
      dot_alias_str = name + "\\nprod=";
      if (producers.empty()) {
        dot_alias_str += "<none>";
      }
      else {
        bool docomma=false;
        for (auto &prod: producers) {
          if (docomma)
            dot_alias_str += ",";
          else
            docomma = true;
          dot_alias_str += prod;
        }
      }
    }
    return dot_alias_str;
  }

  string genesis;
  bool remote;
  string ssh_identity;
  string ssh_args;
  string eos_root_dir;
  string data_dir;
  string hostname;
  string public_name;
  uint16_t p2p_port;
  uint16_t http_port;
  uint16_t filesize;
  vector<keypair> keys;
  vector<string> peers;
  vector<string> producers;

private:
  string p2p_endpoint_str;
  bool onhost_set;
  bool onhost;
  vector<fc::ip::address> localaddrs;
  string dot_alias_str;
};

struct remote_deploy {
  string ssh_cmd = "/usr/bin/ssh";
  string scp_cmd = "/usr/bin/scp";
  string ssh_identity;
  string ssh_args;
  string local_config_file = "temp_config";
};

struct testnet_def {
  remote_deploy ssh_helper;
  map <string,eosd_def> nodes;
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
  LM_ALL
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
  string shape;
  allowed_connection allowed_connections = PC_NONE;
  bf::path genesis;
  bf::path output;
  bool skip_transaction_signatures = false;
  string eosd_extra_args;
  testnet_def network;
  string data_dir_base;
  string alias_base;
  vector <string> aliases;
  last_run_def last_run;
  int start_delay;
  bool random_start;

  void set_options (bpo::options_description &cli);
  void initialize (const variables_map &vmap);
  bool generate ();
  void define_nodes ();
  void write_config_file (eosd_def &node);
  void make_ring ();
  void make_star ();
  void make_mesh ();
  void make_custom ();
  void write_dot_file ();
  void format_ssh (const string &cmd, const string &hostname, string &ssh_cmd_line);
  bool do_ssh (const string &cmd, const string &hostname);
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
    ("shape,s",bpo::value<string>(&shape)->default_value("star"),"network topology, use \"ring\" \"star\" \"mesh\" or give a filename for custom")
    ("genesis,g",bpo::value<bf::path>(&genesis)->default_value("./genesis.json"),"set the path to genesis.json")
    ("output,o",bpo::value<bf::path>(&output),"save a copy of the generated topology in this file")
    ("skip-signature", bpo::bool_switch(&skip_transaction_signatures)->default_value(false), "EOSD does not require transaction signatures.")
    ("eosd", bpo::value<string>(&eosd_extra_args), "forward eosd command line argument(s) to each instance of eosd, enclose arg in quotes")
    ("delay,d",bpo::value<int>(&start_delay)->default_value(0),"seconds delay before starting each node after the first")
    ("random", bpo::bool_switch(&random_start)->default_value(false),"start the nodes in a random order")
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
  producers = 21;
  data_dir_base = "tn_data_";
  alias_base = "testnet_";

  if (prod_nodes > producers)
    prod_nodes = producers;
  if (prod_nodes > total_nodes)
    total_nodes = prod_nodes;
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
  for (auto &node : network.nodes) {
      write_config_file(node.second);
  }

  write_dot_file ();

  if (!output.empty()) {
    bf::path savefile = output;
    bf::ofstream sf (savefile);

    sf << fc::json::to_pretty_string (network) << endl;
    sf.close();
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
      string pname=network.nodes.find(p)->second.dot_alias(p);
      df << "\"" << node.second.dot_alias (node.first)
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
launcher_def::define_nodes () {
  int per_node = producers / prod_nodes;
  int extra = producers % prod_nodes;
  for (size_t i = 0; i < total_nodes; i++) {
    eosd_def node;
    string dex = boost::lexical_cast<string,int>(i);
    string name = alias_base + dex;
    aliases.push_back(name);
    node.genesis = genesis.string();
    node.data_dir = data_dir_base + dex;
    node.hostname = "127.0.0.1";
    node.public_name = "localhost";
    node.remote = false;
    node.p2p_port += i;
    node.http_port += i;
    keypair kp;
    node.keys.push_back (kp);
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
    network.nodes.insert (pair<string, eosd_def>(name, node));

  }
}

void
launcher_def::write_config_file (eosd_def &node) {
  bf::path filename;
  boost::system::error_code ec;

  if (node.on_host()) {
    bf::path dd(node.data_dir);
    filename = dd / "config.ini";
    if (bf::exists (dd)) {
      int64_t count =  bf::remove_all (dd, ec);
      if (ec.value() != 0) {
        cerr << "count = " << count << " could not remove old directory: " << dd
             << " " << strerror(ec.value()) << endl;
        exit (-1);
      }
    }
    if (!bf::create_directory (node.data_dir, ec) && ec.value()) {
      cerr << "could not create new directory: " << node.data_dir
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

  cfg << "genesis-json = " << node.genesis << "\n"
      << "block-log-dir = blocks\n"
      << "readonly = 0\n"
      << "send-whole-blocks = true\n"
      << "shared-file-dir = blockchain\n"
      << "shared-file-size = " << node.filesize << "\n"
      << "http-server-endpoint = " << node.hostname << ":" << node.http_port << "\n"
      << "listen-endpoint = 0.0.0.0:" << node.p2p_port << "\n"
      << "public-endpoint = " << node.public_name << ":" << node.p2p_port << "\n";
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
    cfg << "remote-endpoint = " << network.nodes.find(p)->second.p2p_endpoint() << "\n";
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
  cfg << "plugin = eosio::chain_api_plugin\n"
      << "plugin = eosio::wallet_api_plugin\n"
      << "plugin = eosio::db_plugin\n"
      << "plugin = eosio::account_history_plugin\n"
      << "plugin = eosio::account_history_api_plugin\n";
  cfg.close();
  if (!node.on_host()) {
    prep_remote_config_dir (node);
    string scp_cmd_line = network.ssh_helper.scp_cmd + " ";
    const string &args = node.ssh_args.length() ? node.ssh_args : network.ssh_helper.ssh_args;
    if (args.length()) {
      scp_cmd_line += args + " ";
    }
    scp_cmd_line += filename.string() + " ";

    const string &uid = node.ssh_identity.length() ? node.ssh_identity : network.ssh_helper.ssh_identity;
    if (uid.length()) {
      scp_cmd_line += uid + "@";
    }

    bf::path dpath = bf::path (node.eos_root_dir) / node.data_dir / "config.ini";
    scp_cmd_line += node.hostname + ":" + dpath.string();

    cerr << "cmdline = " << scp_cmd_line << endl;
    int res = boost::process::system (scp_cmd_line);
    if (res != 0) {
      cerr << "unable to scp config file to host " << node.hostname << endl;
      exit(-1);
    }
  }
}

void
launcher_def::make_ring () {
  define_nodes ();
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
  if (total_nodes < 4) {
    make_ring ();
    return;
  }
  define_nodes ();
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
  define_nodes ();
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
  //don't need to define nodes here
  bf::path source = shape;
  fc::json::from_file(source).as<testnet_def>(network);
}

void
launcher_def::format_ssh (const string &cmd,
                          const string &hostname,
                          string & ssh_cmd_line) {

  ssh_cmd_line = network.ssh_helper.ssh_cmd + " ";
  if (network.ssh_helper.ssh_args.length()) {
    ssh_cmd_line += network.ssh_helper.ssh_args + " ";
  }
  if (network.ssh_helper.ssh_identity.length()) {
    ssh_cmd_line += network.ssh_helper.ssh_identity + "@";
  }
  ssh_cmd_line += hostname + " \"" + cmd + "\"";
  cerr << "cmdline = " << ssh_cmd_line << endl;
}

bool
launcher_def::do_ssh (const string &cmd, const string &hostname) {
  string ssh_cmd_line;
  format_ssh (cmd, hostname, ssh_cmd_line);
  int res = boost::process::system (ssh_cmd_line);
  return (res == 0);
}

void
launcher_def::prep_remote_config_dir (eosd_def &node) {
  bf::path abs_data_dir = bf::path(node.eos_root_dir) / node.data_dir;
  string add = abs_data_dir.string();
  string cmd = "cd " + node.eos_root_dir;
  if (!do_ssh(cmd, node.hostname)) {
    cerr << "Unable to switch to path " << node.eos_root_dir
         << " on host " <<  node.hostname << endl;
    exit (-1);
  }
  cmd = "cd " + add;
  if (do_ssh(cmd,node.hostname)) {
    cmd = "rm -rf " + add + "/block*";
    if (!do_ssh (cmd, node.hostname)) {
      cerr << "Unable to remove old data directories on host "
           << node.hostname << endl;
      exit (-1);
    }
  }
  else {
    cmd = "mkdir " + add;
    if (!do_ssh (cmd, node.hostname)) {
      cerr << "Unable to invoke " << cmd << " on host "
           << node.hostname << endl;
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

  node_rt_info info;
  info.remote = node.remote;

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

  if (!node.on_host()) {
    string cmdl ("cd ");
    cmdl += node.eos_root_dir + "; nohup " + eosdcmd + " > "
      + reout.string() + " 2> " + reerr.string() + "& echo $! > " + pidf.string();
    if (!do_ssh (cmdl, node.hostname)){
      cerr << "Unable to invoke " << cmdl
           << " on host " << node.hostname << endl;
      exit (-1);
    }

    string cmd = "cd " + node.eos_root_dir + "; kill -9 `cat " + pidf.string() + "`";
    format_ssh (cmd, node.hostname, info.kill_cmd);
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
  last_run.running_nodes.push_back (info);
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
  if (mode == LM_NONE)
    return;

  if (random_start) {
    // recompute start order - implemenation deferred
  } else {
    for (auto &node : network.nodes) {
      if (mode != LM_ALL) {
        if ((mode == LM_LOCAL && node.second.remote) ||
            (mode == LM_REMOTE && !node.second.remote)) {
          continue;
        }
      }
      launch (node.second, gts);
      sleep (start_delay);
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
      else {
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

FC_REFLECT( eosd_def,
            (genesis)(remote)(ssh_identity)(ssh_args)(eos_root_dir)(data_dir)
            (hostname)(public_name)(p2p_port)(http_port)(filesize)
            (keys)(peers)(producers) )

FC_REFLECT( testnet_def,
            (ssh_helper)(nodes) )

FC_REFLECT( node_rt_info,
            (remote)(pid_file)(kill_cmd) )

FC_REFLECT( last_run_def,
            (running_nodes) )
