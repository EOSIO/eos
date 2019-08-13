## Goal

Create a keosd wallet

## Before you begin

* Install the currently supported version of cleos

* Understand the following:
  * What is an account
  * What is a public and private key pair

## Steps

Create a wallet and save the password to a file:
```shell
cleos wallet create --file password.pwd
```
You should see something like below:

```shell
Creating wallet: default
Save password to use in the future to unlock this wallet.
Without password imported keys will not be retrievable.
saving password to password.pwd
```