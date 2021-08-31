## Overview

This how-to guide provides instructions on how to create a new EOSIO blockchain account using the `cleos` CLI tool. You can use accounts to deploy smart contracts and perform other related blockchain operations. Create one or multiple accounts as part of your development environment setup.

The example in this how-to guide creates a new account named **bob**, authorized by the default system account **eosio**, using the `cleos` CLI tool. 

## Before you Begin

Make sure you meet the following requirements:

* Install the currently supported version of `cleos`.
[[info | Note]]
| The cleos tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the cleos tool. 
* Learn about [EOSIO Accounts and Permissions](https://developers.eos.io/welcome/v2.1/protocol/accounts_and_permissions)
* Learn about Asymmetric Cryptography - [public key](https://developers.eos.io/welcome/v2.1/glossary/index#public-key) and [private key](https://developers.eos.io/welcome/v2.1/glossary/index#private-key) pairs.
* Create public/private keypairs for the `owner` and `active` permissions of an account.

## Command Reference

See the following reference guide for `cleos` command line usage and related options:
* [`cleos create account`](../03_command-reference/create/account.md) command and its parameters

## Procedure

The following step shows how to create a new account **bob** authorized by the default system account **eosio**.

1. Run the following command to create the new account **bob**:

```sh
cleos create account eosio bob EOS87TQktA5RVse2EguhztfQVEh6XXxBmgkU8b4Y5YnGvtYAoLGNN
```
**Where**:
* `eosio` = the system account that authorizes the creation of a new account
* `bob` = the name of the new account conforming to [account naming conventions](https://developers.eos.io/welcome/v2.1/protocol-guides/accounts_and_permissions#2-accounts)
* `EOS87TQ...AoLGNN` = the owner public key or permission level for the new account (**required**)
[[info | Note]]
| To create a new account in the EOSIO blockchain, an existing account, also referred to as a creator account, is required to authorize the creation of a new account. For a newly created EOSIO blockchain, the default system account used to create a new account is **eosio**.

**Example Output**

```console
executed transaction: 4d65a274de9f809f9926b74c3c54aadc0947020bcfb6dd96043d1bcd9c46604c  200 bytes  166 us
#         eosio <= eosio::newaccount            {"creator":"eosio","name":"bob","owner":{"threshold":1,"keys":[{"key":"EOS87TQktA5RVse2EguhztfQVEh6X...
warning: transaction executed locally, but may not be confirmed by the network yet         ]
```

### Summary

By following these instructions, you are able to create a new EOSIO account in your blockchain environment.
