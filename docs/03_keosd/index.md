# Kapifinyd

## Introduction

`kapifinyd` is a key manager service daemon for storing private keys and signing digital messages. It provides a secure key storage medium for keys to be encrypted at rest in the associated wallet file. `kapifinyd` also defines a secure enclave for signing transaction created by `clapifiny` or a third part library.

## Installation

`kapifinyd` is distributed as part of the [EOSIO software suite](https://github.com/EOSIO/apifiny/blob/master/README.md). To install `kapifinyd` just visit the [EOSIO Software Installation](../00_install/index.md) section.

## Operation

When a wallet is unlocked with the corresponding password, `clapifiny` can request `kapifinyd` to sign a transaction with the appropriate private keys. Also, `kapifinyd` provides support for hardware-based wallets such as Secure Encalve and YubiHSM.

[[info | Audience]]
| `kapifinyd` is intended to be used by EOSIO developers only.
