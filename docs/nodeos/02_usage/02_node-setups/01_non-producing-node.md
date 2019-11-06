
[[info]]
| This article assumes you have already [installed `nodeos`](../../01_install/index.md).

A non-producing node is a node that is not configured to produce blocks, instead it is connected and synchronized with other peers from an `EOSIO` based blockchain, exposing one or more services publicly or privately by enabling one or more [`nodeos` available plugins](../../03_plugins/index.md) except the `producer_plugin`.

## Steps
To setup a non-producing node is simple. 

1. [Set Peers](#1-set-peers)
2. [Enable one or more available plugins](#2-enable-one-or-more-available-plugins)

### 1. Set Peers

You need to set some peers in your config ini, for example:

```console
# config.ini:

p2p-peer-address = 106.10.42.238:9876
```

Or you can include the peer in as a boot flag when running `nodeos`, as follows:

```sh
$ nodeos ... --p2p-peer-address=106.10.42.238:9876
```

### 2. Enable one or more available plugins

Each available plugin is listed and detailed in [nodeos available plugins](../../03_plugins/index.md) section.
When you start `nodeos` it will expose the functionality provided by the plugins it was started with, and it is said about those plugins they are `enabled`. For example, if you start `nodeos` with [`state_history_plugin`](state_history_plugin/index.md) plugin you will have a non-producing node that offers full blockchain history, and if you start `nodeos` with [`http_plugin`](http_plugin/index.md) plugin additionally enabled you will have a non-producing node which exposes the EOSIO RPC API as well. You can enable any number of existing plugins and thus have a good amount of flexibility what the non-producing node function/s will be. Another aspect you need to know is that some plugins have dependencies on other plugins and therefor you'll have to satisfy all dependencies for a plugin in order to enable it.

