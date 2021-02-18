## Goal

Transfer token created by `eosio.token` contract

## Before you begin

* Install the currently supported version of `cleos`.

* `eosio.token` contract is deployed on the network you are connected to.

* Understand the following:
  * What is a [transaction](https://developers.eos.io/welcome/v2.1/glossary/index/#transaction).
  * Token transfers are irreversible.

## Steps

Assume you want to transfer `0.0001 SYS` token to an account called `bob` from an account called `alice`, execute the following:

```sh
cleos transfer alice bob "0.0001 SYS" "Hodl!" -p alice@active
```

It should yield an output similar to the one below:

```console
executed transaction: 800835f28659d405748f4ac0ec9e327335eae579a0d8e8ef6330e78c9ee1b67c  128 bytes  1073 us
#   eosio.token <= eosio.token::transfer        {"from":"alice","to":"bob","quantity":"25.0000 SYS","memo":"m"}
#         alice <= eosio.token::transfer        {"from":"alice","to":"bob","quantity":"25.0000 SYS","memo":"m"}
#           bob <= eosio.token::transfer        {"from":"alice","to":"bob","quantity":"25.0000 SYS","memo":"m"}
```
