## Goal

Stake resources, NET and/or CPU, for your account.

## Before you begin

* Install the currently supported version of `cleos`.

* Ensure the [reference system contracts](https://developers.eos.io/manuals/eosio.contracts/v1.9/build-and-deploy) are deployed and used to manage system resources.

* Understand the following:
  * What is an [account](https://developers.eos.io/welcome/latest/glossary/index/#account).
  * What is [NET bandwidth](https://developers.eos.io/manuals/eosio.contracts/latest/key-concepts/net).
  * What is [CPU bandwidth](https://developers.eos.io/manuals/eosio.contracts/latest/key-concepts/cpu).
  * The [`delegatebw` cleos sub-command](https://developers.eos.io/manuals/eos/latest/cleos/command-reference/system/system-delegatebw).

## Steps

* Stake `0.01 SYS` NET bandwidth for `alice` from `alice` account:

```sh
cleos system delegatebw alice alice "0.01 SYS" "0 SYS"
```

The output should be similar to the one below:

```console
executed transaction: 5487afafd67bf459a20fcc2dbc5d0c2f0d1f10e33123eaaa07088046fd18e3ae  192 bytes  503 us
#         eosio <= eosio::delegatebw            {"from":"alice","receiver":"alice","stake_net_quantity":"0.01 SYS","stake_cpu_quanti...
#   eosio.token <= eosio.token::transfer        {"from":"alice","to":"eosio.stake","quantity":"0.01 EOS","memo":"stake bandwidth"}
#  alice <= eosio.token::transfer        {"from":"alice","to":"eosio.stake","quantity":"0.01 SYS","memo":"stake bandwidth"}
#   eosio.stake <= eosio.token::transfer        {"from":"alice","to":"eosio.stake","quantity":"0.01 SYS","memo":"stake bandwidth"}
```

* Stake `0.01 SYS` CPU bandwidth for `alice` from `alice` account:

```sh
cleos system delegatebw alice alice "0 SYS" "0.01 SYS"
```

The output should be similar to the one below:

```console
executed transaction: 5487afafd67bf459a20fcc2dbc5d0c2f0d1f10e33123eaaa07088046fd18e3ae  192 bytes  503 us
#         eosio <= eosio::delegatebw            {"from":"alice","receiver":"alice","stake_net_quantity":"0.0000 SYS","stake_cpu_quanti...
#   eosio.token <= eosio.token::transfer        {"from":"alice","to":"eosio.stake","quantity":"0.01 EOS","memo":"stake bandwidth"}
#  alice <= eosio.token::transfer        {"from":"alice","to":"eosio.stake","quantity":"0.01 SYS","memo":"stake bandwidth"}
#   eosio.stake <= eosio.token::transfer        {"from":"alice","to":"eosio.stake","quantity":"0.01 SYS","memo":"stake bandwidth"}
```
