# producer_api_plugin

## Description

The `producer_api_plugin` exposes a number of endpoints for the [`producer_plugin`](../producer_plugin/index.md) to the RPC API interface managed by the [`http_plugin`](../http_plugin/index.md).

## Usage

```sh
# config.ini
plugin = apifiny::producer_api_plugin

# nodapifiny startup params
$ nodapifiny ... --plugin apifiny::producer_api_plugin
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
plugin = apifiny::producer_plugin
[options]
plugin = apifiny::chain_plugin
[options]
plugin = apifiny::http_plugin
[options]

# command-line
$ nodapifiny ... --plugin apifiny::producer_plugin [options]  \
             --plugin apifiny::chain_plugin [operations] [options]  \
             --plugin apifiny::http_plugin [options]
```
