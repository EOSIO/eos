# Introduction

`keosd` is a key manager for store private keys and sign transactions created by `cleos`. It provides a secure key storage medium so that when the private keys are stored in a wallet file, it will be encrypted at rest. Only when a wallet is unlocked with the wallet password, `cleos` can request `keosd` to sign a transaction with these private keys. `keosd` provides support for hardware-based wallets as well such as Secure Encalve and YubiHSM

`keosd` is intended to be used by EOSIO developers only