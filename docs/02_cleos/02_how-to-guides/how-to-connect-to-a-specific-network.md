## Overview
This guide provides instructions on how to connect to specifc EOSIO blockchain when using `cleos`. `Cleos` can connect to a specific node by using the `--url` optional argument, followed by the http address and port number.

The examples use the `--url`optional argument to send commands to the specified blockchain.   

[[info | Default address:port]]
| If no optional arguments are used (i.e. no `--url`), `cleos` attempts to connect to a local `nodeos` running at localhost or `127.0.0.1` and default port `8888`. Use the `nodeos` command line arguments or [config.ini](../../nodeos/usage/nodeos-configuration/#configini-location) file to specify a different address.

<!--
devrel-1602
[config.ini](../../01_nodeos/02_usage/01_nodeos-configuration/#configini-location)
resolves incorrectly to:
/manuals/eos/latest/01_nodeos/usage/nodeos-configuration/#configini-location
(prefix number in "01_nodeos" not removed)
-->

## Before you Begin
Make sure you meet the following requirements: 

* Install the currently supported version of `cleos`.
[[info | Note]]
| `Cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will install the `cleos` and `keosd` command line tools.  
* You have access to an EOSIO blockchain and the http afddress and port number of a `nodeos` instance. 

## Reference
See the following reference guides for command line usage and related options:

* [cleos](../index.md) command

## Example

1. Add the `-url` option to specify the `nodeos` instance 

```shell
cleos -url http://nodeos-host:8888 COMMAND
```
**Where**
* `-url http://nodeos-host:8888` = The http address and port number of the `nodeos` instance to connect to
* COMMAND = The `cleos`command.

## Summary
In conclusion, by following these instructions you are able to connect to a specified `nodeos` instance. 
