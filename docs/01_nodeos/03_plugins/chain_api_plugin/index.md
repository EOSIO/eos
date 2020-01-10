# chain_api_plugin

## Description

The `chain_api_plugin` exposes functionality from the [`chain_plugin`](../chain_plugin/index.md) to the RPC API interface managed by the [`http_plugin`](../http_plugin/index.md).

## Usage

```sh
# config.ini
plugin = eosio::chain_api_plugin

# command-line
$ nodeos ... --plugin eosio::chain_api_plugin
```

## Options

None

## Dependencies

* [`chain_plugin`](../chain_plugin/index.md)
* [`http_plugin`](../http_plugin/index.md)

### Load Dependency Examples

```sh
# config.ini
plugin = eosio::chain_plugin
[options]
plugin = eosio::http_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::chain_plugin [operations] [options]  \
             --plugin eosio::http_plugin [options]
```
