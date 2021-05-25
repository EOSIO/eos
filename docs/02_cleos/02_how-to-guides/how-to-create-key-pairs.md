## Overview

This how-to guide provides instructions on how to create a keypair consisting of a public key and a private key for signing transactions in an EOSIO blockchain.

## Before you begin

Make sure you meet the following requirements:
* Install the currently supported version of `cleos`
[[info | Note]]
| The cleos tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the cleos tool. 
* Learn about asymmetric cryptography (public and private keypair) in the context of an EOSIO blockchain.

## Command Reference

See the following reference guide for `cleos` command line usage and related options:
* [`cleos create key`](../03_command-reference/create/key.md) command and its parameters

## Procedure

The following steps show how to create a public/private keypair, display them on the console, and save them to a file:

1. Create a public/private keypair and print them to the console:

```sh
cleos create key --to-console
```

**Where**:

* `--to-console` = The option parameter to print the keypair to the console

**Example Output**

```console
Private key: 5KPzrqNMJdr6AX6abKg*******************************cH
Public key: EOS4wSiQ2jbYGrqiiKCm8oWR88NYoqnmK4nNL1RCtSQeSFkGtqsNc
```

2. Create a public/private keypair and save it to a file:

```sh
cleos create key --file pw.txt
```
**Where**: 

* `--file` = The option parameter to save the keypair to a file
* `FILE_TO_SAVEKEY` = The name of the file to save the keypair

**Example Output**

```console
saving keys to pw.txt
```

To view the saved keypair stored in the file:

```sh
cat pw.txt
```
```console
Private key: 5K7************************************************
Public key: EOS71k3WdpLDeqeyqVRAAxwpz6TqXwDo9Brik5dQhdvvpeTKdNT59
```

## Summary

By following these instructions, you are able to create public/private keypairs, print them to the console, and save them to a file. 
