# Non-Producing Node

[[info]]
| This article assumes you have already [installed Nodeos](../../02_installation/index.md).

A non-producing node is a node that is not configured to produce blocks. An example use case would be to provide a synchronized node that provides a public HTTP-RPC API for developers or as a dedicated private endpoint for your app.

## Steps
To setup a non-producing node is simple. 

1. [Set Peers](#set-peers)

### Set Peers

You need to set some peers in your config ini, for example:

```console
# config.ini:

p2p-peer-address = 106.10.42.238:9876
```

Or you can include the peer in as a boot flag when running `nodeos`, as follows:

```sh
$ nodeos ... --p2p-peer-address=106.10.42.238:9876
```
