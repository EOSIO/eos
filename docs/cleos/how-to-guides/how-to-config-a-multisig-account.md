# How to configure a multi-sig account with cleos

## Goal

Setup an account that require multiple signatures for signning a transaction

## Before you begin

* You have an account

* You have some token allocated to your account

* Install the currently supported version of cleos

* Understand the following:
  * What is an account
  * What is a transaction



## Steps

```shell
cleos set account permission multisig active '{\"threshold\" : 1, \"accounts\" :[{\"permission\":{\"actor\":\"eosio\",\"permission\":\"active\"},\"weight\":1},{\"permission\":{\"actor\":\"customera\",\"permission\":\"active\"},\"weight\":1}]}' owner -p multisig@owner"
```