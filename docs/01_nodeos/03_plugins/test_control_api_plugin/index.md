# test_control_api_plugin

## Description

The `test_control_api_plugin` allows to send a control message to the [test_control_plugin](../test_control_plugin/index.md) telling the plugin to shut down the `nodapifiny` instance when reaching a particular block. It is intended for testing.

## Usage

```sh
# config.ini
plugin = apifiny::test_control_api_plugin

# command-line
$ nodapifiny ... --plugin apifiny::test_control_api_plugin
```

## Options

None

## Usage Example

```sh
curl %s/v1/test_control/kill_node_on_producer -d '{ \"producer\":\"%s\", \"where_in_sequence\":%d, \"based_on_lib\":\"%s\" }' -X POST -H \"Content-Type: application/json\"" %
```

## Dependencies

* [`test_control_plugin`](../test_control_plugin/index.md)
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
