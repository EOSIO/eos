## Goal

Stake resource for your account

## Before you begin

* Install the currently supported version of `cleos`

* Ensure the reference system contracts from `eosio.contracts` repository is deployed and used to manage system resources

* Understand the following:
  * What is an account
  * What is network bandwidth
  * What is CPU bandwidth

## Steps

Stake 0.01 SYS network bandwidth for `alice`

```sh
cleos system delegatebw alice alice "0 SYS" "0.01 SYS"
```

Stake 0.01 SYS CPU bandwidth for `alice`:

```sh
cleos system delegatebw alice alice "0.01 SYS" "0 SYS"
```