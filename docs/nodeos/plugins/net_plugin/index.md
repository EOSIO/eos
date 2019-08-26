# net_plugin

## Description

The `net_plugin` provides an authenticated p2p protocol to persistently synchronize nodes.

## Usage

```sh
# config.ini
plugin = eosio::net_plugin [options]

# nodeos startup params
$ nodeos ... -- plugin eosio::net_plugin [options]
```

## Options

```sh
  --p2p-listen-endpoint arg        (default=0.0.0.0:9876)
                                   The actual host:port used to listen for
                                   incoming p2p connections.
  --p2p-server-address arg         An externally accessible host:port for
                                   identifying this node. Defaults to
                                   p2p-listen-endpoint.
  --p2p-peer-address arg           The public endpoint of a peer node to connect                                    to. Use multiple p2p-peer-address options as
                                   needed to compose a network.
  --p2p-max-nodes-per-host arg     (default=1)     
                                   Maximum number of client nodes from any single
                                   IP address
  --agent-name arg                 (default="EOS Test Agent")  
                                   The name supplied to identify this node     
                                   amongst the peers.
  --allowed-connection arg         (default=any)
                                   Can be 'any' or 'producers' or 'specified' or
                                   'none'. If 'specified', peer-key must be
                                   specified at least once. If only 'producers',
                                   peer-key is not required. 'producers' and
                                   'specified' may be combined.
  --peer-key arg                   Optional public key of peer allowed to
                                   connect.  May be used multiple times.
  --peer-private-key arg           Tuple of [PublicKey, WIF private key] (may
                                   specify multiple times)
  --max-clients arg                (default=25)
                                   Maximum number of clients from which
                                   connections are accepted, use 0 for no
                                   limit
  --connection-cleanup-period arg  (default=30) 
                                   Number of seconds to wait before cleaning up
                                   dead connections
  --max-cleanup-time-msec arg      (default=10)
                                   Max connection cleanup time per cleanup call
                                   in millisec
  --network-version-match arg      (default=False)  
                                   True to require exact match of peer network
                                   version.
  --sync-fetch-span arg            (default=100)
                                   Number of blocks to retrieve in a chunk from
                                   any individual peer during synchronization
  --use-socket-read-watermark arg  (default=False)  
                                   Enable expirimental socket read watermark
                                   optimization
  --peer-log-format arg            (default=["${_name}" ${_ip}:${_port}])
                                   The string used to format peers when logging
                                   messages about them. Variables are escaped
                                   with ${<variable name>}.
                                   Available Variables:
                                      _name  self-reported name
                                      _id    self-reported ID (64 hex characters)
                                      _sid   first 8 characters of_peer.id
                                      _ip    remote IP address of peer
                                      _port  remote port number of peer
                                      _lip   local IP address connected to peer
                                      _lport local port number connected to peer
```

## Dependencies

None
