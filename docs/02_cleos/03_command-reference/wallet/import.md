## Command
```sh
cleos wallet import [OPTIONS]
```
**Where**:
* [`OPTIONS`] = See **Options** in [**Command Usage**](command-usage) section below

**Note:** The arguments and options enclosed in square brackets are optional.

## Description
Use this command to imports private key into wallet. 

## Command Usage
The following information shows the different positionals and options you can use with the `cleos wallet import` command:

### Positionals
* none

### Options
* `-h,--help` - Print this help message and exit
* `-n,--name` _TEXT_ - The name of the wallet to import key into
* `--private-key` _TEXT_ - Private key in WIF format to import

## Requirements
For prerequisites to run this command, see the **Before you Begin** section of the [How to Import a Key](../../02_how-to-guides/how-to-import-a-key.md) topic.

## Examples
The following examples demonstrate the `cleos wallet import` command:

**Example 1.** Import a private key to the default wallet:

```sh
cleos wallet import
```
```console
private key: ***
```
```console
imported private key for: EOS8FBXJUfbANf3xeDWPoJxnip3Ych9HjzLBr1VaXRQFdkVAxwLE7
```
