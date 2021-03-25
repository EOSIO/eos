## Overview

This how-to guide provides instructions on how to import a private key into a wallet. You can use the private key to authorize transactions in an EOSIO blockchain. 

## Before you Begin

Make sure you meet the following requirements:

* Create a wallet using the `cleos wallet create` command. See the [How to Create a Wallet](../02_how-to-guides/how-to-create-a-wallet.md) section for instructions. 
* [Open]() and [unlock]() the created wallet.
* Familiarize with the [`cleos wallet import`](../03_command-reference/wallet/import.md) command.

* Install the currently supported version of `cleos`.

[[info | Note]]
| `cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install `cleos`.


* Understand what a [public](https://developers.eos.io/welcome/latest/glossary/index/#public-key) and [private](https://developers.eos.io/welcome/latest/glossary/index/#private-key) key pair is.

## Command Reference
See the following reference guide for command line usage and related options for the cleos command:
* [cleos wallet import](../03_command-reference/wallet/import.md) command and its parameters

## Procedure

The following steps show how to import a private key to an existing `keosd` wallet:

1. Run the following command to import a private key into an existing wallet:
```sh
cleos wallet import
```

2. Enter the private key and hit Enter. 
```console
cleos wallet import
private key:
```

**Example Output**

The following example confirms that the private key is imported for the corresponding public key. 

```console
private key: imported private key for: EOS8FBXJUfbANf3xeDWPoJxnip3Ych9HjzLBr1VaXRQFdkVAxwLE7
```
## Summary
By following these instructions, you are able to import a private key to an existing wallet. 
