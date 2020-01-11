# db_size_api_plugin

## Description

The `db_size_api_plugin` retrieves analytics about the blockchain.

* free_bytes
* used_bytes
* size
* indices

## Reference Documentation
[`db_size_api_plugin` reference documents](./api-reference/)

## Usage

```console
# Not available
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
