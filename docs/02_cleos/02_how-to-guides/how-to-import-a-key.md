## Goal

Import a private key into a wallet

## Before you begin

Make sure you meet the following requirements:

* Create a wallet using the `cleos wallet create` command. See the [How to Create a Wallet](../02_how-to-guides/how-to-create-a-wallet.md) section for instructions. 
* [Open]() and [unlock]() the created wallet.
* Familiarize with the [`cleos wallet import`](../03_command-reference/wallet/import.md) command.

* Install the currently supported version of `cleos`.

[[info | Note]]
| `cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install `cleos`.


* Understand what a [public](https://developers.eos.io/welcome/latest/glossary/index/#public-key) and [private](https://developers.eos.io/welcome/latest/glossary/index/#private-key) key pair is.

## Steps

Perform the following steps:

1. Run the following command to import a private key into an existing wallet:
```sh
cleos wallet import
```

2. Enter your private key. 
```console
cleos wallet import
private key:
```

**Example Output**

The following example confirms that the private key is imported for the corresponding public key. 

```console
private key: imported private key for: EOS8FBXJUfbANf3xeDWPoJxnip3Ych9HjzLBr1VaXRQFdkVAxwLE7
```
