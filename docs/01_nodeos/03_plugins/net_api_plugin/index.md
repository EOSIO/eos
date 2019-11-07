# net_api_plugin

## Description

The `net_api_plugin` exposes functionality from the `net_plugin` to the RPC API interface managed by the `http_plugin`.

The `net_api_plugin` provides four RPC API endpoints:

* connect
* disconnect
* connections
* status

See [Net section of RPC API](https://developers.eos.io/eosio-nodeos/reference).

[[warning]]
| This plugin exposes endpoints that allow management of p2p connections, running this plugin on a publicly accessible node is not recommended as it can be exploited.

## Usage

```sh
# config.ini
plugin = eosio::net_api_plugin
[options]

# command-line
$ nodeos ... -- plugin eosio::net_api_plugin [options]
```

## Options

None

## Dependencies

* [`net_plugin`](../net_plugin/index.md)
* [`http_plugin`](../http_plugin/index.md)

### Load Dependency Examples

```sh
# config.ini
plugin = eosio::net_plugin
[options]
plugin = eosio::http_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::net_plugin [options]  \
             --plugin eosio::http_plugin [options]
```
