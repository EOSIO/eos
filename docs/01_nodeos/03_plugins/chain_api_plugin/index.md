# chain_api_plugin

## Description

The `chain_api_plugin` exposes functionality from the [`chain_plugin`](../chain_plugin/index.md) to the RPC API interface managed by the [`http_plugin`](../http_plugin/index.md).

## Usage

```sh
# config.ini
plugin = apifiny::chain_api_plugin

# command-line
$ nodapifiny ... --plugin apifiny::chain_api_plugin
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
