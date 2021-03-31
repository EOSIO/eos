## Command

```sh
cleos create key [OPTIONS]
```

**Where**:

* [`OPTIONS`] = See **Options** in **Command Usage** section below. 

## Description

Use this command to create a new keypair and print the public and private keys

## Command Usage

## Description
Creates a new keypair and either prints the public and private keys to the screen or to a file.

## Command Usage
The following information shows the different positionals and options you can use with the `cleos create key` command:

### Positionals:
- none
### Options
- `-h,--help` - Print this help message and exit
- `--r1` - Generate a key using the R1 curve (iPhone), instead of the K1 curve (Bitcoin)
`-f`,`--file` _TEXT_ - Name of file to write private/public key output to. (Must be set, unless "--to-console" is passed
`--to-console` - Print private/public keys to console.

## Requirements
* Install the currently supported version of `cleos`.
[[info | Note]]
| The `cleos` tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will install the `cleos` and `keosd` command line tools.   

## Examples
1. Create a new key pair and output to the screen
```shell
cleos create key --to-console
```
**Where**
`--to-console` = Tells the `cleos create key` command to print the private/public keys to the console.

**Example Output**
```shell
Private key: 5KDNWQvY2seBPVUz7MiiaEDGTwACfuXu78bwZu7w2UDM9A3u3Fs
Public key: EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC
```

## Requirements

For prerequisites to run this command, see the **Before you Begin** section of the [How to Create Keypairs](../02_how-to-guides/how-to-create-key-pairs.md) topic.

## Examples

The following example creates a keypair and prints the output to the console:

```console
cleos create key --to-console
Private key: 5KPzrqNMJdr6AX6abKg*******************************cH
Public key: EOS4wSiQ2jbYGrqiiKCm8oWR88NYoqnmK4nNL1RCtSQeSFkGtqsNc
```

The following example creates a keypair and saves it to a file using the ``--file`` flag: 
```console
cleos create key --file pw.txt         
saving keys to pw.txt
```
