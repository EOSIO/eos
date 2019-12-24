# test_control_plugin

## Description

The `test_control_plugin` is designed to cause a graceful shutdown when reaching a particular block in a sequence of blocks produced by a specific block producer. It can be invoked to either shutdown on the **head block** or the **last irreversible block**.

This is intended for testing, to determine exactly when a nodapifiny instance will shutdown.

## Usage

```sh
# config.ini
plugin = apifiny::test_control_plugin

# command-line
$ nodapifiny ... --plugin apifiny::test_control_plugin
```

## Options

None

## Dependencies

* [`chain_plugin`](../chain_plugin/index.md)

### Load Dependency Examples

```sh
# config.ini
plugin = apifiny::chain_plugin
[options]

# command-line
$ nodapifiny ... --plugin apifiny::chain_plugin [operations] [options]
```
