# producer_api_plugin

## Description

The `producer_api_plugin` exposes a number of endpoints for the [`producer_plugin`](../producer_plugin/index.md) to the RPC API interface managed by the [`http_plugin`](../http_plugin/index.md).

## Reference Documentation
[`producer_api_plugin` reference documents](./api-reference/)


## Usage

```sh
# config.ini
plugin = eosio::producer_api_plugin

# nodeos startup params
$ nodeos ... --plugin eosio::producer_api_plugin
```

## Options

None

## Dependencies

* [`producer_plugin`](../producer_plugin/index.md)
* [`chain_plugin`](../chain_plugin/index.md)
* [`http_plugin`](../http_plugin/index.md)

### Load Dependency Examples

```sh
# config.ini
plugin = eosio::producer_plugin
[options]
plugin = eosio::chain_plugin
[options]
plugin = eosio::http_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::producer_plugin [options]  \
             --plugin eosio::chain_plugin [operations] [options]  \
             --plugin eosio::http_plugin [options]
```
