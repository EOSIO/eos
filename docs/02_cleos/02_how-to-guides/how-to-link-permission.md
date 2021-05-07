## Overview
This guide provides instructions to link a permission to a smart contract action.   

The example uses `cleos` to link a custom permission _customp_ in the account __alice_ to a _hi_ action deployed to the _scontract_ account so that the _alice_ account's `active` permssion and _customp_ permission are authorized to call the _hi_ _action.  

## Before you Begin
Make sure you meet the following requirements: 

* Install the currently supported version of `cleos.`
[[info | Note]]
| `Cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the `cleos` and `keosd` comand line tools. 
* You have access to a blockchain and the `eosio.system` reference contract from [`eosio.contracts`](https://github.com/EOSIO/eosio.contracts) repository is deployed and used to manage system resources.
* You have an EOSIO account and access to the account's `active` private key.
* You have created a custom permssion.

## Command Reference
See the following reference guides for command line usage and related options:

* [cleos set action permssion](../03_command-reference/set/set-action-permssion.md) command

## link Procedure

The following step shows you how to link a permssion

1. Run the following command to link _alices_ account permssion _customp_ with the _hi_ action deployed to the _scontract_ account:

```shell
cleos set action permission alice scontract hi customp -p alice@active
```
**Where**
`alice` = The name of the account containing the permssion to link.
`scontract`= The name of the account which owns the smart contract.
`hi` = The name of the action to link to a permssion. 
`customp` = The permission used to authorize the transaction.
`-p alice@active` = The permission used to authorize setting the permssions.

**Example Output**
```shell
executed transaction: 4eb4cf3aea232d46e0e949bc273c3f0575be5bdba7b61851ab51d927cf74a838  128 bytes  141 us
#         eosio <= eosio::linkauth              {"account":"alice","code":"scontract","type":"hi","requirement":"customp"}
```
## Summary
In conclusion, by following these instructions you are able to link a permssion to a smart contract action.

