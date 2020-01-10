# wallet_api_plugin

## Description

The `wallet_api_plugin` exposes functionality from the [`wallet_plugin`](../wallet_plugin/index.md) to the RPC API interface managed by the [`http_plugin`](../http_plugin/index.md).

[[caution | Caution]]
| This plugin exposes wallets. Therefore, running this plugin on a publicly accessible node is not recommended. As of 1.2.0, `nodeos` will no longer allow the `wallet_api_plugin`.

## Usage

```sh
# config.ini
plugin = eosio::wallet_api_plugin

# command-line
$ nodeos ... --plugin eosio::wallet_api_plugin
```

## Options

None

## Dependencies

* [`wallet_plugin`](../wallet_plugin/index.md)
* [`http_plugin`](../http_plugin/index.md)

### Load Dependency Examples

```sh
# config.ini
plugin = eosio::wallet_plugin
[options]
plugin = eosio::http_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::wallet_plugin [options]  \
             --plugin eosio::http_plugin [options]
```
