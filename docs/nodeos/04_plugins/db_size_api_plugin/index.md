# db_size_api_plugin

## Description

The `db_size_api_plugin` retrieves analytics about the blockchain.

* free_bytes
* used_bytes
* size
* indices

## Usage

[TODO]

## Options

None

## Dependencies

* [`chain_plugin`](../chain_plugin/index.md)
* [`http_plugin`](../http_plugin/index.md)

### Load Dependency Examples

```sh
# config.ini
plugin = eosio::chain_plugin [functions] [options]
plugin = eosio::http_plugin [options]

# nodeos startup params
$ nodeos ... --plugin eosio::chain_plugin [functions] [options] --plugin eosio::http_plugin [options]
```
