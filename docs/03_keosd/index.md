# Keosd

## Introduction

`keosd` is a key manager service daemon for storing private keys and signing digital messages. It provides a secure key storage medium for keys to be encrypted at rest in the associated wallet file. `keosd` also defines a secure enclave for signing transaction created by `cleos` or a third part library.

## Installation

`keosd` is distributed as part of the [EOSIO software suite](https://github.com/EOSIO/eos/blob/master/README.md). To install `keosd` just visit the [EOSIO Software Installation](../00_install/index.md) section.

## Operation

When a wallet is unlocked with the corresponding password, `cleos` can request `keosd` to sign a transaction with the appropriate private keys. Also, `keosd` provides support for hardware-based wallets such as Secure Encalve and YubiHSM.

[[info | Audience]]
| `keosd` is intended to be used by EOSIO developers only.
