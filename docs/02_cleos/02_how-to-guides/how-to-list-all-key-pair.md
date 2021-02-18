## Goal

List all key pairs

## Before you begin

Make sure you meet the following requirements:

* Create a wallet using the `cleos wallet create` command. See the [How to Create a Wallet](../02_how-to-guides/how-to-create-a-wallet.md) section for instructions. 
* [Create a keypair](../03_command-reference/wallet/create_key.md) within the wallet.
* [Open]() and [unlock]() the created wallet.
* Familiarize with [`cleos wallet`](../03_command-reference/wallet/index.md) commands.

* Install the currently supported version of `cleos`.

[[info | Note]]
| `cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install `cleos`.


* Understand what a [public](https://developers.eos.io/welcome/latest/glossary/index/#public-key) and [private](https://developers.eos.io/welcome/latest/glossary/index/#private-key) key pair is.

## Steps

To list the keypairs within a wallet, open and unlock the wallet first. 



1. Open the wallet:
```sh
cleos wallet open
```
2. Unlock the existing wallet. The command will prompt to enter a password:
```sh
cleos wallet unlock
```
```console
cleos wallet unlock
password:
```
3. Enter the generated key (password) when you created the existing wallet:
```sh
cleos wallet private_keys
password:
```

4. List all public keys within the wallet:
```sh
cleos wallet keys
```
```console
cleos wallet keys
[
  "EOS5VCUBtxS83ZKqVcWtDBF4473P9HyrvnCM9NBc4Upk1C387qmF3"
]
```
5. List all private keys withing an existing wallet:
```sh
cleos wallet private_keys
```
6. Enter the generated key (password) when you created the wallet: 
```sh
cleos wallet private_keys
password:
```
```console
cleos wallet private_keys
password: [[
    "EOS5VCUBt****************************************F3",
    "5JnuuGM1S****************************************4D"
  ]
]
```

[[info | Warning]]
| Never reveal your private keys in a production environment. 






