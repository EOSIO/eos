## Command
```sh
cleos wallet keys [OPTIONS]
```
**Where**:
* [`OPTIONS`] = See **Options** in [**Command Usage**](#command-usage) section below

**Note:** The arguments and options enclosed in square brackets are optional.

## Description
Use this command to list all public keys stored in a wallet. You can use keys to sign and verify transactions.

## Command Usage
The following information shows the different positionals and options you can use with the `cleos wallet keys` command:

### Positionals
* none

### Options
* `-h,--help` - Print this help message and exit

## Requirements
For prerequisites to run this command, see the **Before you Begin** section of the [How to List All Keys](../../02_how-to-guides/how-to-list-all-key-pairs.md) topic.

## Examples
The following examples demonstrate the `cleos wallet keys` command:

**Example 1.** List all public keys within the default wallet:

```sh
cleos wallet keys
```
```console
[[
    "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
    "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
  ]
]
```
