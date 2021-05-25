## Overview
This guide provides instructions on how to connect to specifc wallet when using `cleos`. `Cleos` can connect to a specific wallet by using the `--wallet-url` optional argument, followed by the http address and port number.

The example uses the `wallet-url` optional arguments to request data from the the specified `keosd` instance.

[[info | Default address:port]]
| If no optional arguments are used (i.e. no `--wallet-url`), `cleos` attempts to connect to a local `nodeos` or `keosd` running at localhost or `127.0.0.1` and default port `8900`. Use the `keosd` command line arguments or [config.ini](../../03_keosd/10_usage.md/#launching-keosd-manually) file to specify a different address.

## Before you Begin
Make sure you meet the following requirements: 

* Install the currently supported version of `cleos` and `keosd`.
[[info | Note]]
| The `cleos` tool and `keosd` are bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will install the `cleos` and `keosd` command line tools. 
* You have access to an EOSIO blockchain and the http address and port number of a `nodeos` instance. 

## Reference
See the following reference guides for command line usage and related options:

* [cleos](../index.md) command

## Example

1. Add the `--wallet-url` option to specify the `keosd` instance

```shell
cleos --wallet-url http://keosd-host:8900 COMMAND
```

**where**
* `--wallet-url http://keosd-host:8900` = The http address and port number of the `keosd` instance to connect to
* COMMAND = The `cleos`command.

## Summary
In conclusion, by following these instructions you are able to connect to a specified `keosd` instance. 
