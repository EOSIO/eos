## Overview

This how-to guide provides instructions on how to query infomation of an EOSIO account. The example in this how-to guide retrieves information of the `eosio` account. 

## Before you begin

* Install the currently supported version of `cleos`

[[info | Note]]
| The cleos tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the cleos tool. 

* Acquire functional understanding of [EOSIO Accounts and Permissions](https://developers.eos.io/welcome/v2.1/protocol/accounts_and_permissions)

## Command Reference

See the following reference guide for command line usage and related options for the cleos command:

* [`cleos get account`](../03_command-reference/get/account.md) command and its parameters

## Procedure

The following step shows how to query information of the `eosio` account:

1. Run the following command:

```sh
cleos get account eosio
```
**Where**:

* `eosio` = The name of the default system account in the EOSIO blockchain.

**Example Output**

```console
created: 2018-06-01T12:00:00.000
privileged: true
permissions:
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:       unlimited  used:     3.004 KiB

net bandwidth:
     used:               unlimited
     available:          unlimited
     limit:              unlimited

cpu bandwidth:
     used:               unlimited
     available:          unlimited
     limit:              unlimited
```

[[info | Account Fields]]
| Depending on the EOSIO network you are connected, you might see different fields associated with an account. That depends on which system contract has been deployed on the network.
