## Overview
This guide provides instructions to unlink a linked permission to a smart contract action.   

The example uses `cleos` to unlink a custom permission _customp_ which is linked to the _hi_ action deployed to the _scontract_ account.   

## Before you Begin
Make sure you meet the following requirements: 

* Install the currently supported version of `cleos.`
[[info | Note]]
| `Cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the `cleos` and `keosd` comand line tools. 
* You have an EOSIO account and access to the account's `active` private key.
* You have created a custom permission and linked it. See [How to Link Permission](how-to-link-permission.md).

## Command Reference
See the following reference guides for command line usage and related options:

* [cleos set action permission](../03_command-reference/set/set-action-permission.md) command
## link Procedure

The following step shows you how to link a permission:

1. Run the following command to unlink _alices_ account permission _customp_ with the _hi_ action deployed to the _scontract_ account:

```shell
cleos set action permission alice scontract hi NULL -p alice@active
```

**Where**
* `alice` = The name of the account containing the permission to link.
* `scontract`= The name of the account which owns the smart contract.
* `hi` = The name of the action to link to a permission. 
* `NULL` = Sets the permission to NULL.
* `-p alice@active` = The permission used to authorize linking the _customp_ permission.

**Example Output**
```shell
executed transaction: 4eb4cf3aea232d46e0e949bc273c3f0575be5bdba7b61851ab51d927cf74a838  128 bytes  141 us
#         eosio <= eosio::unlinkauth              {"account":"alice","code":"scontract","type":"hi"}
```

## Summary
In conclusion, by following these instructions you are able to unlink a permission to a smart contract action.
