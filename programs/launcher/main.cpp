/**
 * launch testnet nodes
 **/
#include <string>
#include <vector>
#include <math.h>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/process/child.hpp>
#include <boost/process/system.hpp>
#include <boost/process/io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/variant.hpp>

using namespace std;
namespace bf = boost::filesystem;
namespace bp = boost::process;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;


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
      remote(false)
  {}

  string p2p_endpoint () {
    if (p2p_endpoint_str.empty()) {
      p2p_endpoint_str = public_name + ":" + boost::lexical_cast<string, uint16_t>(p2p_port);
    }
    return p2p_endpoint_str;
  }

  string genesis;
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
  bool remote;

private:
  string p2p_endpoint_str;

  };

  struct remote_deploy {
    string ssh_cmd = "/usr/bin/ssh";
    string scp_cmd = "/usr/bin/scp";
    string ssh_identity;
    vector <string> ssh_args;
    string local_config_file = "temp_config";
  };

  struct testnet_def {
    remote_deploy ssh_helper;
    map <string,eosd_def> nodes;
  };

FC_REFLECT( keypair, (public_key)(wif_private_key) )

  FC_REFLECT( remote_deploy,
            (ssh_cmd)(scp_cmd)(ssh_identity)(ssh_args) )

  FC_REFLECT( eosd_def,
              (genesis)(eos_root_dir)(data_dir)(hostname)(public_name)
              (p2p_port)(http_port)(filesize)(keys)(peers)(producers) )

  FC_REFLECT( testnet_def,
              (ssh_helper)(nodes))



struct topology {
  int producers;
  int total_nodes;
  int prod_nodes;
  string shape;
  bf::path genesis;
  bf::path output;
  testnet_def network;
  string data_dir_base = "tn_data_";
  string alias_base = "testnet_";
  vector <string> ssh_cmd_args;
  vector <string> aliases;
  int launch_mode;

  topology ()
    :  producers(21),
       total_nodes(1),
       prod_nodes(1),
       shape("ring"),
       genesis("./genesis.json"),
       output(),
       launch_mode (3)
  {}

  void set_options (bpo::options_description &cli) {
    cli.add_options()
      ("nodes,n",bpo::value<int>()->default_value(1),"total number of nodes to generate")
      ("pnodes,p",bpo::value<int>()->default_value(1),"number of nodes that are producers")
      ("shape,s",bpo::value<string>()->default_value("ring"),"network topology, use \"ring\" \"star\" \"mesh\" or give a filename for custom")
      ("genesis,g",bpo::value<bf::path>(),"set the path to genesis.json")
      ("output,o",bpo::value<bf::path>(),"save a copy of the generated topology in this file")
      ("launch,l",bpo::value<string>(), "select a suset of nodes to launch. Currently may be \"all\", \"none\", or \"local\". If not set, the default is to launch all unless an output file is named, in whichhhh case it starts none.");
  }

  void initialize (const variables_map &vmap) {
    if (vmap.count("nodes"))
      total_nodes = vmap["nodes"].as<int>();
    if (vmap.count("pnodes"))
      prod_nodes = vmap["pnodes"].as<int>();
    if (vmap.count("shape"))
      shape = vmap["shape"].as<string>();
    if (vmap.count("genesis"))
      genesis = vmap["genesis"].as<bf::path>();
    if (vmap.count("output"))
      output = vmap["output"].as<bf::path>();
    if (vmap.count("launch")) {
      string l = vmap["launch"].as<string>();
      if (boost::iequals(l,"all"))
        launch_mode = 3;
      else if (boost::iequals(l,"local"))
        launch_mode = 1;
      else if (boost::iequals(l,"none"))
        launch_mode = 0;
      else {
        cerr << "unrecognized launch mode: " << l << endl;
        exit (-1);
      }
    }
    else {
      launch_mode = output.empty() ? 3 : 0;
    }

    if (prod_nodes > producers)
      prod_nodes = producers;
    if (prod_nodes > total_nodes)
      total_nodes = prod_nodes;
  }

  bool generate () {
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
    if (!output.empty()) {
      bf::path savefile = output;
      bf::ofstream sf (savefile);

      sf << fc::json::to_pretty_string (network) << endl;
      sf.close();
      return false;
    }
    return true;
  }

  void define_nodes () {
    int per_node = producers / prod_nodes;
    int extra = producers % prod_nodes;
    for (int i = 0; i < total_nodes; i++) {
      eosd_def node;
      string dex = boost::lexical_cast<string,int>(i);
      string key = alias_base + dex;
      aliases.push_back(key);
      node.genesis = genesis.string();
      node.data_dir = data_dir_base + dex;
      node.public_name = node.hostname = "127.0.0.1";
      node.remote = false;
      node.p2p_port += i;
      node.http_port += i;
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
      network.nodes.insert (pair<string, eosd_def>(key, node));

    }
  }

  void write_config_file (eosd_def &node) {
    bf::path filename;
    boost::system::error_code ec;

    if (!node.remote) {
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
        << "shared-file-dir = blockchain\n"
        << "shared-file-size = " << node.filesize << "\n"
        << "http-server-endpoint = " << node.hostname << ":" << node.http_port << "\n"
        << "listen-endpoint = 0.0.0.0:" << node.p2p_port << "\n"
        << "public-endpoint = " << node.public_name << ":" << node.p2p_port << "\n";
    for (const auto &p : node.peers) {
      cfg << "remote-endpoint = " << network.nodes.find(p)->second.p2p_endpoint() << "\n";
    }
    if (node.producers.size()) {
      cfg << "enable-stale-production = true\n"
          << "required-participation = true\n";
      for (const auto &kp : node.keys ) {
        cfg << "private-key = [\"" << kp.public_key
            << "\",\"" << kp.wif_private_key << "\"]\n";
      }
      cfg << "plugin = producer_plugin\n"
          << "plugin = chain_api_plugin\n";
      for (auto &p : node.producers) {
        cfg << "producer-name = " << p << "\n";
      }
    }
    cfg.close();
    if (node.remote) {
      prep_remote_config_dir (node);
      vector<string> args;
      for (const auto &sa : network.ssh_helper.ssh_args) {
        args.emplace_back (sa);
      }

      args.push_back (filename.string());
      string dest = node.hostname;
      if (network.ssh_helper.ssh_identity.length()) {
        dest = network.ssh_helper.ssh_identity + "@" + node.hostname;
      }
      bf::path dpath = bf::path (node.eos_root_dir) / node.data_dir / "config.ini";
      dest += ":" + dpath.string();
      args.emplace_back();
      args.back().swap(dest);
      int res = boost::process::system (network.ssh_helper.scp_cmd, args);
      if (res != 0) {
        cerr << "unable to scp config file to host " << node.hostname << endl;
        exit(-1);
      }
    }
  }

  void make_ring () {
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

  void make_star () {
    if (total_nodes < 4) {
      make_ring ();
      return;
    }
    define_nodes ();
    size_t links = 3;
    if (total_nodes > 12) {
      links = (size_t)sqrt(total_nodes);
    }
    size_t gap = total_nodes > 6 ? 4 : total_nodes - links;

    for (size_t i = 0; i < total_nodes; i++) {
      auto &current = network.nodes.find(aliases[i])->second;
      for (size_t l = 1; l <= links; l++) {
        size_t ndx = (i + l * gap) % total_nodes;
        if (i == ndx) {
          ++ndx;
        }
        auto &peer = aliases[ndx];
        for (bool found = true; found; ) {
          found = false;
          for (auto &p : current.peers) {
            if (p == peer) {
              peer = aliases[++ndx];
              found = true;
              break;
            }
          }
        }
        current.peers.push_back (peer);
      }
    }
  }

void make_mesh () {
    define_nodes ();
    for (size_t i = 0; i < total_nodes; i++) {
      auto &current = network.nodes.find(aliases[i])->second;
      for (size_t j = 1; j < total_nodes; j++) {
        size_t ndx = (i + j) % total_nodes;
        current.peers.push_back (aliases[ndx]);
      }
    }
  }

  void make_custom () {
    //don't need to define nodes here
    bf::path source = shape;
    fc::json::from_file(source).as<testnet_def>(network);
  }


  bool do_ssh (const string &cmd, const string &hostname) {
    if (ssh_cmd_args.empty()) {
      for (const auto &sa : network.ssh_helper.ssh_args) {
        ssh_cmd_args.emplace_back (sa);
      }
      if (network.ssh_helper.ssh_identity.length()) {
        ssh_cmd_args.emplace_back(network.ssh_helper.ssh_identity + "@" + hostname);
      }
      else {
        ssh_cmd_args.emplace_back(hostname);
      }
    }
    string cmdline = "\"" + cmd + "\"";
    ssh_cmd_args.push_back(cmdline);
    cerr << "pre invoke cmd = " << cmd << endl;


    string ccc = network.ssh_helper.ssh_cmd + " ";
    for (auto &a : ssh_cmd_args)
      ccc += a + " ";
    cerr << "ccc = " << ccc << endl;
    int res = boost::process::system (ccc); //network.ssh_helper.ssh_cmd, ssh_cmd_args);
    cerr << "\nres = " << res << endl;

    ssh_cmd_args.pop_back();

    cerr << "post invoke ";
        for (auto &a : ssh_cmd_args)
      cerr << a << " ";
    cerr << endl;

    return (res == 0);
  }

  void prep_remote_config_dir (eosd_def &node) {
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

  void launch (eosd_def &node, string &gts) {
    bf::path dd = node.data_dir;
    bf::path reout = dd / "stdout.txt";
    bf::path reerr = dd / "stderr.txt";
    bf::path pidf  = dd / "eosd.pid";

    if (node.remote && (launch_mode & 2) == 0)
      return;

    string eosdcmd =  "programs/eosd/eosd --data-dir " + node.data_dir;
    if (gts.length()) {
      eosdcmd += " --genesis-timestamp " + gts;
    }

    if (node.remote) {
      string cmdl ("cd ");
      cmdl += node.eos_root_dir + "; nohup " + eosdcmd + " > "
        + reout.string() + " 2> " + reerr.string() + "& echo $! > " + pidf.string();
      if (!do_ssh (cmdl, node.hostname)){
        cerr << "Unable to invoke " << cmdl
             << " on host " << node.hostname << endl;
        exit (-1);
      }
      return;
    }

    bp::child c(eosdcmd, bp::std_out > reout, bp::std_err > reerr );

    bf::ofstream pidout (pidf);
    pidout << c.id() << flush;
    pidout.close();

    if(!c.running()) {
      cout << "child not running after spawn " << eosdcmd << endl;
      for (int i = 0; i > 0; i++) {
        if (c.running () ) break;
      }
    }
    c.detach();
  }

  void start_all (string &gts) {
    if (launch_mode == 0)
      return;

    for (auto &node : network.nodes) {
      launch (node.second, gts);
    }
  }
};

int main (int argc, char *argv[]) {

  variables_map vmap;
  options_description opts ("Testnet launcher options");
  topology top;
  string gts;

  top.set_options(opts);

  opts.add_options()
    ("timestamp,i",bpo::value<string>(),"set the timestamp for the first block. Use \"now\" to indicate the current time");

  bpo::store(bpo::parse_command_line(argc, argv, opts), vmap);
  if (vmap.count("timestamp"))
    gts = vmap["timestamp"].as<string>();

  top.initialize(vmap);
  top.generate();
  top.start_all(gts);

  return 0;
}
