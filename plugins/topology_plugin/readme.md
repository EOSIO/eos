#Topology Plugin
This optional plugin facilitates the gathering of network performance metrics for nodes and links. A second plugin, topology api plugin, adds some http commands used to fetch network status info or to add or remove watchers interested in tracking some of the data.

The topology plugin is tied closely to the net plugin, adding a new timer along with an optional reference to the topology plugin and local node and link ids that are globally unique. A new topology message is added to the net protocol list which is used to communicate changes to the network map, periodic updates of perfomance data, or changes of interested watchers.

##identifying nodes and links
The topology plugin maintains a map of the p2p network in terms of nodes, which are instances of nodeos, and the p2p connections between them. Since in many cases nodes, particularly block producing nodes, are sequestered behind firewalls and using otherwise nontroutable endpoints nodes are identified by a hash of a strinified collection of p2p host and port, and an arbitrary "bp identifier" string which is configured via the command line or config.ini file. Additional node identifiers include the role of the particular node and any specific producer account names, if any.

Links are defined by the pair of nodes being connected, which node is the active or passive connectors. Link descriptors also include the number of hops between the nodes and the kind of data the link is used for. The number of hops in this case refers to things such as network bridges, routers, firewalls, load balancers, proxies, or any other layer 3 or lower devices. Intermediate eosio nodes do not count since a link is terminated at a nodeos instance.

The topology plugin will produce a graphvis compatible rendition of the network map to help operators visualize the possible areas of network congestion that leads to missed blocks and microforks.

Ultimately this map could help further reduce network traffic by not forwarding unblocked transactions to _all_ nodes, preferring to route them only to producer nodes which might be responsible for enblocking or verifying. Likewise new block messages can be routed in a way that avoids duplication.
##API
this table highlights the calls available using the http interface. This list is tentative and subject to change.

| request |   arguments   |   result   | description |
|---------|---------------|------------|-------------|
 nodes | string: in\_roles | vector of node descriptors | Nodes have specific roles in the network, producers, api, backup, proxy, etc. This function allows you to retrieve a list of nodes on the network for a specific role, or for any role.
 links | string: with_node | vector of link descriptors | supply a [list of?] node ideentifies as retrived with the /nodes/ function and get back a list of descriptors covering all of the connections to or from the identified node
  graph |  | stringified \"dot\" file | pipe the output to a file or straight into a graphviz command. The rendered graph shows all the nodes in the network, colorized to indicate status. Green means no forks detected, yellow for a few forks and red for many
  report | | stringified \"md\" file | pipe the output to a file with the .md extention. The report is broken into multiple sections to present all the known status 
  
##configuration options
These are the new setting introduced by the topology plugin

| option | values | description |
|--------|--------|-------------|
  bp-name | string | \"block producer name\" used to disambiguate otherwise nonroutable addresses
 sample-interval-seconds | uint16 | delay between sample acquisions
 max-hops | uint16 | maximum number of times a given message is repeated when distributing

##metrics gathered
These are the tentative connection metrics that are accumulated by the topology module. Some metrics refer to flow from the local to the peer and others from the peer to the local node. To disambiguate, the flow is either "up" from the active connector to the passive connector, or "down" which is from passive to active. An "active" connector is typically considered a client, and a "passive" connector is typically a server. However with this protocol the two nodes are equivalent in responsibility.

| name | description |
|------|-------------|
 queue\_depth | how many messages are waiting to be sent
 queue\_latency | how long does a message sit in the queue
 net\_latency | how long does a network traversal take
 bytes\_sent | how many bytes have been sent on a link since its connection
 bytes\_per\_second | average flow rate in bytes/second
 messages\_sent | how many messages have been sent on a link since connection
 messages\_per\_second | average flow rate in messages/second
 fork\_instances | how many forks have been detected from a peer
 fork\_depth | how many blocks we on the most recent / current fork
 fork\_max\_depth | how many blocks on the longest fork since connection

#usage guidelines
Use the cleos "topology graph" and "topology report" commands to be able to observe and get details by looking at the report. The cleos "topology sample" command fetches all of the local information regarding communications performance as a json bundle.

Periodically you can retrieve graphical views of the network using one of the following commands

| command line | 
|--------
|cleos topology graph > demo.dot
cleos toplogy graph \| dot -Tpng > demo.png

In the first example the unrendered dot data is stored in a file that may be monitored by a live renderer such as graphviz and the second example the command line renderer is used to produce a png formatted image file.

If the graph shows a node in yellow or red, retrieve a detailed report from the indicated nodes. In this case the http or https access point for the affected node is displayed. In cases where that node is not directly reachable, you may have to solicit assistance of the local system operator in order to obtain the generated report.

The topology plugin keeps track of major events such as node creation or fork detection that occur anywhere on the network. However in order to prevent spamming the network, the periodic data samples are shared only with a node's immediate peers. 
#implementation details
The remainder of this document is provided as a survey of the plugin contents.

##source file map
The plugin follows the common idiom of a lightweight interface class and a more robust hidden implementation class. To improve the atomicity of included headers, these are broken apart as shown

| filename | definitions | description    |
|----------|-------------|----------------|
 topology_plugin.hpp | topology\_plugin | The accessable interface for the module
 messages.hpp | link\_sample | a collection of metrics related to a single link id.
 | map\_update | a collection of updates to the relationships of nodes and links, for example when a new nodeos instance becomes active or one goes down.
 | topology\_data | a variant that resolves to either of the previous 2 data types
 | topology\_message | the new p2p message type used to communicate network status information between the various peers.
 link\_descriptor.hpp | link\_roles | an enumeration of possible specialized use for this link. This could be blocks, transactions, commands, or any combination of those three.
 | link\_descriptor | the verbose description of the link as defined by the connected nodes. This gets stringified and hashed to generate a universally unique, but otherwise anonymous identifier.
 node\_descriptor | node\_descriptor | the verbose description of a node used to generate a globally unique node id.

##internal structure info
 The implementation details are entirely contained in a single source file for now, topology\_plugin.cpp. The types defined in this source file are described here.

|  typename  | members | description  |
|------------|---------|--------------|
   topo\_node | info | the description of the node along with it's hashed id
   | links | list of links to peer nodes.
   | distance | A map of all known nodes indexed by id and storing the number of hops from this node to the indexed node. This distance value may be used to optimize message routing by computing an optimal path for forwarding broadcasted messages.
   topo\_link | info | the description of the link with an id
   | up | the collection of named metrics on the link from the active connector to the passive.
   | down | the collection of metrics on the link from the passive connector to the active.
