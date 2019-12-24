# db_size_api_plugin

## Description

The `db_size_api_plugin` retrieves analytics about the blockchain.

* free_bytes
* used_bytes
* size
* indices

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
plugin = apifiny::chain_plugin
[options]
plugin = apifiny::http_plugin
[options]

# command-line
$ nodapifiny ... --plugin apifiny::chain_plugin [operations] [options]  \
             --plugin apifiny::http_plugin [options]
```
