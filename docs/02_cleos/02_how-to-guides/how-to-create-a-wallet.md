## Goal

Create a kapifinyd wallet

## Before you begin

* Install the currently supported version of clapifiny

* Understand the following:
  * What is an account
  * What is a public and private key pair

## Steps

Create a wallet and save the password to a file:

```shell
clapifiny wallet create --file password.pwd
```

You should see something like below. Note here the wallet is named as `default`

```shell
Creating wallet: default
Save password to use in the future to unlock this wallet.
Without password imported keys will not be retrievable.
saving password to password.pwd
```

Alternatively, you can name a wallet with `-n` option:

```shell
clapifiny wallet create -n named_wallet -f passwd
```

You will see something like below:

```shell
Creating wallet: named_wallet
Save password to use in the future to unlock this wallet.
Without password imported keys will not be retrievable.
saving password to passwd
```
