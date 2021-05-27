## Command
```sh
cleos wallet import [OPTIONS]
```
**Where**:
* [`OPTIONS`] = See **Options** in [**Command Usage**](command-usage) section below

**Note:** The arguments and options enclosed in square brackets are optional.

## Description
Imports a private key into a local wallet. This command will launch `keosd` if it is not already running. 

## Command Usage
The following information shows the different positionals and options you can use with the `cleos wallet import` command:

### Positionals
* none

### Options
* `-h,--help` - Print this help message and exit
* `-n,--name` _TEXT_ - The name of the wallet to import key into
* `--private-key` _TEXT_ - Private key in WIF format to import

## Requirements
* Install the currently supported version of `cleos`.
[[info | Note]] 
| `Cleos` is bundled with the EOSIO software. [Installing EOSIO](../../../00_install/index.md) will also install the `cleos` and `keosd` comand line tools. 
* For additional prerequisites to run this command, see the **Before you Begin** section of the [How to Import a Key](../../02_how-to-guides/how-to-import-a-key.md) topic.

## Examples
The following examples demonstrate the `cleos wallet import` command:

**Example 1.** Import a private key to the default wallet:
```shell
cleos wallet import
```

The command asks for the private key. Enter it.

```shell
5KDNWQvY2seBPVUz7MiiaEDGTwACfuXu78bwZu7w2UDM9A3u3Fs
```

**Example Output**
```console
imported private key for: EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC
```

**Example 2.** Import a private key to a named wallet and pass the private key on the command line:
```shell
cleos wallet import --name my_wallet --private-key 5KDNWQvY2seBPVUz7MiiaEDGTwACfuXu78bwZu7w2UDM9A3u3Fs
```

**Where:**
`--name` my_wallet = Tells the `cleos wallet import` command to import the key to `my_wallet` 
`--private-key` 5KDNWQvY2seBPVUz7MiiaEDGTwACfuXu78bwZu7w2UDM9A3u3Fs = Tells the `cleos wallet import` command the private key to import 

**Example Output**
```console
imported private key for: EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC
```
