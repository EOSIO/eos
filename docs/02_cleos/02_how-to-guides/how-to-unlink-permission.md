## Goal

Unlink a linked permission level

## Before you begin

* Install the current supported version of cleos

* Understand the following:
  * What is an account
  * What is permission level
  * What is an action

## Steps

Remove a linked permission level from an action `transfer` of contract `hodlcontract`

```shell
cleos set action permission alice hodlcontract transfer NULL
```