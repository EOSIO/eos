# How to stake resource

## Goal

Stake resource for your account

## Before you begin

* Install the currently supported version of cleos

* Understand the following:
  * What is an account
  * What is network bandwidth
  * What is CPU bandwidth

## Steps

Stake 0.01 SYS network bandwidth for `alice`

```shell
cleos system delegatebw alice alice "0 SYS" "0.01 SYS"
```

Stake 0.01 SYS CPU bandwidth for `alice`:

```shell
cleos system delegatebw alice alice "0.01 SYS" "0 SYS"
```