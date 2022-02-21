## Command
```sh
cleos net connect [OPTIONS] host
```

**Where:**
* [OPTIONS] = See **Options** in the [**Command Usage**](command-usage) section below.
* host = The hostname:port to connect to

**Note:** The arguments and options enclosed in square brackets are optional.

## Description
Start a new connection to a specified peer. A node operator can use this command to instruct a node to connect to another peer without restarting the node.

## Command Usage
The following information shows the different positionals and options you can use with the `cleos net connect` command:

### Positionals
* `host` _TEXT_ REQUIRED - The hostname:port to connect to

### Options
* `-h,--help` - Print this help message and exit

## Requirements
Make sure you meet the following requirements:

* Install the currently supported version of `cleos`.
[[info | Note]]
| `cleos` is bundled with the EOSIO software. [Installing EOSIO](../../../00_install/index.md) will also install the `cleos` and `keosd` command line tools.
* You have access to a producing node instance with the [`net_api_plugin`](../../../01_nodeos/03_plugins/net_api_plugin/index.md) loaded.

## Examples
The following examples demonstrate how to use the `cleos net connect` command:

* Instruct default local node (listening at default http address `http://127.0.0.1:8888`) to connect to peer node listening at p2p address `localhost:9002`:
```sh
cleos net connect localhost:9002
```
**Output:**
```console
"added connection"
```

* Instruct local node listening at http address `http://127.0.0.1:8001` to connect to peer node listening at p2p address `localhost:9002`:
```sh
cleos -u http://127.0.0.1:8001 net connect localhost:9002
```
**Output:**
```console
"added connection"
```

**Note:** If any of the above commands are re-executed, `cleos` returns the following message as expected:  
```console
"already connected"
```
