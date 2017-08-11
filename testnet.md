This document describes how to use the launcher application to provision and launch a number of eosd nodes to form a test network. The launcher came from my need to frequently start and stop eosd instances with varying configurations, and to clean up stale db and log files later.


THis document is broken into the following sections
1. What is the launcher and how do you run it?
2. Local vs distributed test networks, network topology
3. Customizing





For the moment the laucher only activates platform-native nodes, dockerized nodes will be added later. It should be straight forward to use the generated configuration files with dockerized nodes.



These nodes may exist all on local host, or they may be distributed across a cluster of hosts, assuming you have ssh access to each host. EOS must be built or installed on each.

There is a new app, launcher, which is able to configure and deploy an arbitrary number of test net nodes.

usage:
start_testnet.pl [--nodes=<n>] [--pnodes=<n>] [--topo=<"ring"|"star"|filename>] [--time-stamp=<time>]

where:
--nodes, -n n (default = 1) sets the number of eosd instances to launch
--pnodes, -p n (default = 1) sets the number nodes that will also be producers
--shape, -a name (default = ring) sets the network topology to either a ring shape or a star shape
--time-stamp, -t YYYY-MM-DD"T"HH:MM:DD (default "") sets the timestamp in UTC for the genesis block. use "now" to mean the current time
--savefile, -s filename (default "") when specified, triggers the writing of the current network configuration as json into the named file. You can then use a JSON aware editor to modify this file in order to provide alternative peer connections, or to distribute acros multiple hosts in a cluster.


test network description file.

node
  eos-root-dir
  data-dir
  hostname/ip
  p2p-port
  https-port
  ws-port
  peer-list [<alias>, <alias>, ...]
  producers [<name>, <name>, ...]

node
...
-----------------------

To deploy




-----------------------
Functionality that remains to be implemented: caching signed transactions then purging them on a schedule. Sending summaries of blocks rather than whole blocks. Optimizing the routing between nodes. Failover during new node synchronization if a peer fails to respond timely enough.

Also need to prepare tests that are distributed, deterministic, and repeatable.
