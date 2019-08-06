# How to transfer a eosio.token-based token

## Goal

Transfer token created by eosio.token contract

## Before you begin

* Install the currently supported version of cleos

* You are going to transfer a token created by eosio.token contract

* Understand the following:
  * What is a transaction
  * Token transfers are irrevertable 

## Steps

Assumed you would like to transfer `0.0001 SYS` token to an account called `bob` from an account called `alice`

```shell
cleos transfer alice bob "0.0001 SYS" "Hodl!" -p alice@active
```

