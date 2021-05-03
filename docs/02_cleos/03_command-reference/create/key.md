## Command

```sh
cleos create key [OPTIONS]
```

**Where**:

* [`OPTIONS`] = See **Options** in **Command Usage** section below. 

## Description

Use this command to create a new keypair and print the public and private keys

## Command Usage

```console
Usage: cleos create key [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --r1                        Generate a key using the R1 curve (iPhone), instead of the K1 curve (Bitcoin)
  -f,--file TEXT              Name of file to write private/public key output to. (Must be set, unless "--to-console" is passed
  --to-console                Print private/public keys to console.
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
