## Overview
This how-to guide provides instructions on how to create a keypair consisting of a public and a private key for signing transactions in the EOSIO blockchain.

## Before you begin
Make sure you meet the following requirements:


* Install the currently supported version of `cleos`

[[info | Note]]
| The cleos tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the cleos tool. 

* Acquire functional understanding of asymmetric cryptography (public and private keypair) in the context of blockchain

## Command Reference
See the following reference guide for command line usage and related options for the cleos command:

* [`cleos create key`](../03_command-reference/create/key.md) command and its parameters

## Procedure

The following steps show how to create key pairs and display them on the console and save to a file:

1. Create a keypair and print the result to the console:

```sh
cleos create key --to-console
```

**Where**:

* `--to-console` = The option parameter to print the keypair to console

**Example Output**

```console
cleos create key --to-console
Private key: 5KPzrqNMJdr6AX6abKg*******************************cH
Public key: EOS4wSiQ2jbYGrqiiKCm8oWR88NYoqnmK4nNL1RCtSQeSFkGtqsNc
```


2. Create a keypair and save it to a file:

```sh
cleos create key --file pw.txt
```
**Where**: 

* `--file` = The option parameter to save the keypair to a file
* `FILE_TO_SAVEKEY` = The name of the file where to save the keypair

**Example Output**
```console
cleos create key --file pw.txt         
saving keys to pw.txt
```

To view the saved keypair in the file:
```console
cat pw.txt
Private key: 5K7************************************************
Public key: EOS71k3WdpLDeqeyqVRAAxwpz6TqXwDo9Brik5dQhdvvpeTKdNT59
```

## Summary
By following these instructions, you are able to print keypairs to console and save keypairs to a file. 
