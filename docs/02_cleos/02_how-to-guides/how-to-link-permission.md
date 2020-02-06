## Goal

Link a permission to an action of a contract

## Before you begin

* Install the currently supported version of `cleos`

* Understand the following:
  * What is an account
  * What is permission level
  * What is an action

## Steps

Link a permission level `permlvl` to the action `transfer` of contract `hodlcontract`

```sh
cleos set action permission alice hodlcontract transfer permlvl
```