## Goal

Setup an account that require multiple signatures for signning a transaction

## Before you begin

* You have an account

* Ensure the reference system contracts from `eosio.contracts` repository is deployed and used to manage system resources

* You have sufficient token allocated to your account

* Install the currently supported version of cleos

* Unlock your wallet

## Steps

Buys RAM in value of 0.1 SYS tokens for account `alice`:

```shell
cleos system buyram alice alice "0.1 SYS" -p alice@active
```