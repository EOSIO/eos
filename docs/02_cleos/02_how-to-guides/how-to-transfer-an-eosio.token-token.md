## Overview

This how-to guide provides instructions on how to transfer tokens created by `eosio.token` contract.

## Before you begin

* Install the currently supported version of `cleos`.

* `eosio.token` contract is deployed on the network you are connected to.

* Understand the following:
  * What a [transaction](https://developers.eos.io/welcome/latest/glossary/index/#transaction) is.
  * Token transfers are irreversible.

## Command Reference

See the following reference guides for command line usage and related options for the `cleos` command:

* The [cleos transfer](https://developers.eos.io/manuals/eos/latest/cleos/command-reference/transfer) reference.

## Procedure

The following steps show how to transfer `0.0001 SYS` tokens to an account called `bob` from an account called `alice`:

```sh
cleos transfer alice bob "0.0001 SYS" "Hodl!" -p alice@active
```

Where:

* `alice` = the account that transfers the tokens.
* `bob` = the account that receives the tokens.
* `0.0001 SYS` = the amount of `SYS` tokens sent.
* `Hodl!` = the message, or memo, that is accompanying the transaction.

Example output:

```console
executed transaction: 800835f28659d405748f4ac0ec9e327335eae579a0d8e8ef6330e78c9ee1b67c  128 bytes  1073 us
#   eosio.token <= eosio.token::transfer        {"from":"alice","to":"bob","quantity":"25.0000 SYS","memo":"m"}
#         alice <= eosio.token::transfer        {"from":"alice","to":"bob","quantity":"25.0000 SYS","memo":"m"}
#           bob <= eosio.token::transfer        {"from":"alice","to":"bob","quantity":"25.0000 SYS","memo":"m"}
```

## Summary

In conclusion, the above instructions show how to transfer tokens created by `eosio.token` contract from one account to another.
