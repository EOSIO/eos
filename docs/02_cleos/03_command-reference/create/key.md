## Command
```sh
cleos create key [OPTIONS]
```
**Where**:
* [`OPTIONS`] = See **Options** in [**Command Usage**](command-usage) section below. 

**Note:** The arguments and options enclosed in square brackets are optional.

## Description
Use this command to create a new keypair and print the public and private keys

## Command Usage
The following information shows the different positionals and options you can use with the `cleos create key` command:

### Positionals
* none

### Options
* `-h,--help` - Print this help message and exit
* `--r1` - Generate a key using the R1 curve (iPhone), instead of the K1 curve (Bitcoin)
* `-f`,`--file` _TEXT_ - Name of file to write private/public key output to. (Must be set, unless "--to-console" is passed
* `--to-console` - Print private/public keys to console

## Requirements
For prerequisites to run this command, see the **Before you Begin** section of the [How to Create Keypairs](../../02_how-to-guides/how-to-create-key-pairs.md) topic.

## Examples
The following examples demonstrate the `cleos create key` command:

**Example 1.** Create a new public/private keypair and print the output to the console:
```sh
cleos create key --to-console
```
```console
Private key: 5KPzrqNMJdr6AX6abKg*******************************cH
Public key: EOS4wSiQ2jbYGrqiiKCm8oWR88NYoqnmK4nNL1RCtSQeSFkGtqsNc
```

**Example 2.** Create a new public/private keypair and save it to a file named `pw.txt`: 
```sh
cleos create key --file pw.txt         
```
```console
saving keys to pw.txt
```
