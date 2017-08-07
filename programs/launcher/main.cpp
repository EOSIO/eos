/*
 * launch test met modes

 **/
#include <string>
#include <sstream>
#include <vector>
#include <math.h>

#include <boost/program_options.hpp>
#include <boost/process/child.hpp>
#include <boost/process/system.hpp>
#include <boost/process/io.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/exception/exception.hpp>

using namespace std;
namespace bf = boost::filesystem;
namespace bp = boost::process;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

namespace eos {
  struct keypair {
    string public_key = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV";
    string wif_private_key = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3";
  };

  struct eosd_def {
    bf::path genesis = "genesis.json";
    string data_dir_base = "tn_data_";
    bf::path data_dir;
    string local_config_file = "temp_config";
    string hostname = "127.0.0.1";
    uint16_t p2p_port = 9876;
    uint16_t http_port = 8888;
    uint16_t filesize = 8192;
    vector<keypair> keys;
    vector<string> peers;
    vector<string> producers;

    string p2p_endpoint () {
      if (p2p_endpoint_str.empty()) {
        ostringstream ep;
        ep << hostname << ":" << p2p_port;
        p2p_endpoint_str = ep.str();
      }
      return p2p_endpoint_str;
    }

    virtual void copy_config_file () {}

    virtual void launch (string &gts) {
      ostringstream cmdl;
      cmdl << "programs/eosd/eosd --data-dir " << data_dir;
      if (gts.length()) {
        cmdl << " --genesis-timestamp " << gts;
      }
      bf::path reout = data_dir / "/stdout.txt";
      bf::path reerr = data_dir / "stderr.txt";
      bp::child c(cmdl.str(), bp::std_out > reout, bp::std_err > reerr );
      bf::path pidfile = data_dir / "eosd.pid";
      bf::ofstream pidf (pidfile);
      pidf << c.id() << endl;
      pidf.close();

      if(!c.running()) {
        cout << "child not running after spawn " << cmdl.str() << endl;
        for (int i = 0; i > 0; i++) {
          if (c.running () ) break;
        }
      }
      c.detach();
    }

  private:
    string p2p_endpoint_str;

  };

  struct remote_eosd_def : public eosd_def {
    string ssh_cmd = "/usr/bin/ssh";
    string scp_cmd = "/usr/bin/scp";
    string ssh_identity;
    vector <string> ssh_args;
    bf::path eos_root_dir;

    void copy_config_file () override {
      prep_remote_config_dir ();
      vector<string> args;
      for (const auto &sa : ssh_args) {
        args.emplace_back (sa);
      }

      args.push_back (local_config_file);
      string dest = hostname;
      if (ssh_identity.length()) {
        dest = ssh_identity + "@" + hostname;
      }
      bf::path dpath = eos_root_dir / data_dir / "config.ini";
      dest += ":" + dpath.string();
      args.emplace_back();
      args.back().swap(dest);
      int res = boost::process::system (scp_cmd, args);
      if (res != 0) {
        cerr << "unable to scp config file to host " << hostname << endl;
        exit(-1);
      }
    }

  private:
    vector <string> ssh_cmd_args;

    bool do_ssh (const string &cmd) {
      if (ssh_cmd_args.empty()) {
        for (const auto &sa : ssh_args) {
          ssh_cmd_args.emplace_back (sa);
        }
        if (ssh_identity.length()) {
          ssh_cmd_args.emplace_back(ssh_identity + "@" + hostname);
        }
        else {
          ssh_cmd_args.emplace_back(hostname);
        }
      }
      string cmdline = "\"" + cmd + "\"";
      ssh_cmd_args.push_back(cmd);
      int res = boost::process::system (ssh_cmd, ssh_cmd_args);
      ssh_cmd_args.pop_back();
      return (res == 0);
    }

    void prep_remote_config_dir () {
      ostringstream cmd;
      bf::path abs_data_dir = eos_root_dir / data_dir;
      cmd << "cd " << eos_root_dir;
      if (!do_ssh(cmd.str())) {
        cerr << "Unable to switch to path " << eos_root_dir
             << " on host " << hostname << endl;
        exit (-1);
      }
      cmd.clear();
      cmd << "cd " << abs_data_dir;
      if (do_ssh(cmd.str())) {
        cmd.clear();
        cmd << "rm -rf " << abs_data_dir / "*";
        if (!do_ssh (cmd.str())) {
          cerr << "Unable to remove old data directories on host "
               << hostname << endl;
          exit (-1);
        }
      }
      else {
        cmd.clear();
        cmd << "mkdir " << abs_data_dir;
        if (!do_ssh (cmd.str())) {
          cerr << "Unable to invoke " << cmd.str() << " on host "
               << hostname << endl;
          exit (-1);
        }
      }
    }

    void launch (string &gts) override {
      ostringstream cmd;
      cmd << "cd " << eos_root_dir
          << "; nohup programs/eosd/eosd --data-dir " << data_dir;
      if (gts.length()) {
        cmd << " --genesis-timestamp " << gts;
      }
      cmd << " > " << data_dir / "stdout.txt"
          << " 2> " << data_dir / "stderr.txt" << "& echo $! > " << data_dir / "eosd.pid";
      if (!do_ssh (cmd.str())){
        cerr << "Unable to invoke " << cmd.str()
             << " on host " << hostname << endl;
        exit (-1);
      }
    }

  };

}

FC_REFLECT( eos::keypair, (public_key)(wif_private_key) )

FC_REFLECT( eos::eosd_def,
            (genesis)(data_dir_base)(data_dir)(local_config_file)(hostname)
            (p2p_port)(http_port)(filesize)(keys)(peers)(producers) )

FC_REFLECT( eos::remote_eosd_def,
            (ssh_cmd)(scp_cmd)(ssh_identity)(ssh_args)(eos_root_dir) )

struct topology {
  int producers = 21;
  int total_nodes = 1;
  int prod_nodes = 1;
  string shape = "ring";
  bf::path genesis = "./genesis.json";
  bf::path save_file = "./testnet.json";

  void generate (vector <eos::eosd_def> &network) {
    if (shape == "ring") {
      make_ring (network);
    }
    else if (shape == "star") {
      make_star (network);
    }
    else {
      make_custom (network);
    }
  }

  void define_nodes (vector <eos::eosd_def> &nodes) {
    int per_node = producers / prod_nodes;
    int extra = producers % prod_nodes;
    for (int i = 0; i < total_nodes; i++) {
      nodes.emplace_back();
      auto &node = nodes.back();
      ostringstream namer;
      namer << node.data_dir_base << i;// << "/";
      node.data_dir += namer.str();

      cout << "node " << i << " dd = " << node.data_dir << endl;
      node.p2p_port += i;
      node.http_port += i;
      if (i < prod_nodes) {
        int count = per_node;
        if (extra) {
          ++count;
          --extra;
        }
        char ext = 'a' + i;
        while (count--) {
          ostringstream pname;
          pname << "init" << ext;
          ext += prod_nodes;
          node.producers.push_back(pname.str());
        }
      }
    }
  }

  void write_config_file (eos::eosd_def &node) {
    bf::path filename;
    boost::system::error_code ec;
    eos::remote_eosd_def *remote = dynamic_cast<eos::remote_eosd_def *>(&node);
    if (remote == 0) {
      filename = node.data_dir;
      filename /= "config.ini";
      if (bf::exists (node.data_dir)) {
        errno=0;
        ec.clear();
        bf::path sub = node.data_dir;
        int64_t count =  bf::remove_all (sub, ec);
        if (ec.value() != 0) {
          cerr << "count = " << count << " could not remove old directory: " << sub
               << " " << strerror(ec.value()) << endl;
          exit (-1);
        }
        cerr << "count = " << count << " remove all success" << endl;
      }
      if (!bf::create_directory (node.data_dir, ec) && ec.value()) {
        cerr << "could not create new directory: " << node.data_dir
             << " errno " << ec.value() << " " << strerror(ec.value()) << endl;
        exit (-1);
      }
    }
    else {
      filename = node.local_config_file.length()? node.local_config_file : "remote_config";
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
        << "public-endpoint = " << node.hostname << ":" << node.p2p_port << "\n";
    for (const auto &p : node.peers) {
      cfg << "remote-endpoint = " << p << "\n";
    }
    if (node.producers.size()) {
      cfg << "enable-stale-production = true\n"
          << "required-participation = true\n";
      for (const auto &kp : node.keys ) {
        cfg << "private-key = [\"" << kp.public_key
            << "\",\"" << kp.wif_private_key << "\"]\n";
      }
      cfg << "plugin = eos::producer_plugin\n"
          << "plugin = eos::chain_api_plugin\n";
      for (auto &p : node.producers) {
        cfg << "producer-name = " << p << "\n";
      }
    }
    cfg.close();

    node.copy_config_file ();
  }

  void make_ring (vector <eos::eosd_def> &nodes) {
    define_nodes (nodes);
    if (total_nodes > 2) {
      for (size_t i = 0; i < nodes.size(); i++) {
        size_t front = (i + 1) % total_nodes;
        size_t rear = (i > 0 ? i : total_nodes) - 1;
        nodes[i].peers.push_back (nodes[front].p2p_endpoint());
        nodes[i].peers.push_back (nodes[rear].p2p_endpoint());
      }
    }
    else if (total_nodes == 2) {
      nodes[0].peers.push_back (nodes[1].p2p_endpoint());
      nodes[1].peers.push_back (nodes[0].p2p_endpoint());
    }
    for (auto &node: nodes) {
      write_config_file(node);
    }
    if (!save_file.empty()) {
      bf::ofstream sf(save_file);
      if (sf.good()) {
        cerr << "could not open " << save_file << " for writing" << endl;
        exit (-1);
      }
      //    sf << fc::json::to_pretty_string (nodes) << endl;
      sf.close();
      // write json encoded network definition to save file
    }
  }

  void make_star (vector <eos::eosd_def> &nodes) {
    define_nodes (nodes);

    if (total_nodes < 4) {
      make_ring (nodes);
      return;
    }
    size_t links = 3;
    if (total_nodes > 12) {
      links = (size_t)sqrt(total_nodes);
    }
    size_t gap = total_nodes > 6 ? 4 : total_nodes - links;

    for (size_t i = 0; i < total_nodes; i++) {
      for (size_t l = 1; l <= links; l++) {
        size_t ndx = (i + l * gap) % total_nodes;
        if (i == ndx) {
          ++ndx;
        }
        string peer = nodes[ndx].p2p_endpoint();
        for (bool found = true; found; ) {
          found = false;
          for (auto &p : nodes[i].peers) {
            if (p == peer) {
              peer = nodes[++ndx].p2p_endpoint();
              found = true;
              break;
            }
          }
        }
      }
    }
    for (auto &node: nodes) {
      write_config_file(node);
    }
    if (!save_file.empty()) {
      // write json encoded network definition to save file
    }

  }

  void make_custom (vector <eos::eosd_def> &nodes) {
    // read "shape" file as a json struct containing the network definition
  }


};

int main (int argc, char *argv[]) {

  variables_map vmap;
  options_description opts ("Testnet launcher options");
  topology top;
  vector <eos::eosd_def> testnet;
  string gts;

  opts.add_options()
    ("nodes",bpo::value<int>()->default_value(1),"total number of nodes to generate")
    ("pnodes,p",bpo::value<int>()->default_value(1),"number of nodes that are producers")
    ("topo,t",bpo::value<string>()->default_value("ring"),"network topology, use \"ring\" \"star\" or give a filename for custom")
    ("timestamp,i",bpo::value<string>(),"set the timestamp for the first block. Use \"now\" to indicate the current time")
    ("genesis,g",bpo::value<bf::path>(),"set the path to genesis.json")
    ("savefile,s",bpo::value<bf::path>(),"save a copy of the generated topology in this file");

  bpo::store(bpo::parse_command_line(argc, argv, opts), vmap);
  if (vmap.count("nodes"))
    top.total_nodes = vmap["nodes"].as<int>();
  if (vmap.count("pnodes"))
    top.prod_nodes = vmap["pnodes"].as<int>();
  if (vmap.count("topo"))
    top.shape = vmap["topo"].as<string>();
  if (vmap.count("genesis"))
    top.genesis = vmap["genesis"].as<string>();
  if (vmap.count("savefile"))
    top.save_file = vmap["savefile"].as<bf::path>();
  if (vmap.count("timestamp"))
    gts = vmap["timestamp"].as<string>();

  top.generate(testnet);

  for (auto node : testnet) {
    node.launch(gts);
    sleep(4);
  }

  return 0;
}
