## Goal

List all key pairs

## Before you begin

* Install the currently supported version of `cleos`

* Understand the following:
  * What is a public and private key pair

## Steps

Unlock your wallet

```sh
cleos wallet unlock
```

List all public keys:

```sh
cleos wallet keys
```

List all private keys:

```sh
cleos wallet private_keys

```

[[warning]]
| Be careful never real your private keys in a production enviroment
