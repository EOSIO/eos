## Command
cleos set action permission [OPTIONS] account code type requirement

**Where**
* [OPTIONS] = See Options in Command Usage section below.
* account = The account to set/delete a permission authority for.
* code = The account that owns the code for the action.
* type =  The name of the action, note permissions must be set per action.
* requirement = To remove a permission use NULL, To set or update the permission use the name of the permission required for executing the given action.

**Note**: The arguments and options enclosed in square brackets are optional.

## Description
Set parameters dealing with account permissions

## Command Usage
The following information shows the different positionals and options you can use with the `cleos set action permission` command:

### Positionals:
- `account` _TEXT_ REQUIRED The account to set/delete a permission authority 
- `code` _TEXT_ REQUIRED The account that owns the code for the action
- `type` _TEXT_ REQUIRED The type of the action
- `requirement` _TEXT_ REQUIRED [delete] NULL, [set/update] The permission name require for executing the given action

### Options:
- `-h,--help` Print this help message and exit
-  `-x`,`--expiration` Set the time in seconds before a transaction expires, defaults to 30s
-  `-f`,`--force-unique` Force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
-  `-s`,`--skip-sign` Specify if unlocked wallet keys should be used to sign transaction
-  `-j`,`--json` Print result as json
-  `--json-file` _TEXT_ Save result in json format into a file
-  `-d`,`--dont-broadcast` Don't broadcast transaction to the network (just print to stdout)
-  `--return-packed` Used in conjunction with --dont-broadcast to get the packed transaction
-  `-r`,`--ref-block` _TEXT_ Set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)
-  `--use-old-rpc` Use old RPC push_transaction, rather than new RPC send_transaction
-  `-p`,`--permission` _TEXT_ An account and permission level to authorize, as in 'account@permission' (defaults to 'account@active')
-  `--max-cpu-usage-ms` _UINT_ Set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)
-  `--max-net-usage` _UINT_ Set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)
- `--delay-sec` _UINT_ Set the delay_sec seconds, defaults to 0s

## Requirements
* Install the currently supported version of `cleos`.
[[info | Note]] 
| `Cleos` is bundled with the EOSIO software. [Installing EOSIO](../../../00_install/index.md) will also install the `cleos` and `keosd` comand line tools.  
* You have access to an EOSIO blockchain.
* You have an EOSIO account and access to the account's private key.

## Examples

1. Set permission on a _hi_ action deployed to the _scontract1_ account so that the _bob_ account's `active` permission and _customp1_ permission are authorized:

```shell
cleos set action permission bob scontract1 hi customp1 -p bob@active
```
**Where**
* `bob` = The name of the account to link the custom permission authority.
* `scontract1`= The name of the account which owns the smart contract.
* `hi` = The name of the action to link to a permission. 
* `customp1` = The permission used to authorize the transaction.
* `-p bob@active` = The permission used to authorize setting the permissions.

**Example Output**
```shell
executed transaction: 4eb4cf3aea232d46e0e949bc273c3f0575be5bdba7b61851ab51d927cf74a838  128 bytes  141 us
#         eosio <= eosio::linkauth              {"account":"bob","code":"scontract1","type":"hi","requirement":"customp1"}
```

2. Set permissions on a _bye_ action deployed to the _scontract1_ account so that the _bob_ account's `active` permission and _customp2_ permission are authorized:

```shell
cleos set action permission bob scontract1 bye customp2 -p bob@active
```

**Where**
* `bob` = The name of the account to link the custom permission authority.
* `scontract1`= The name of the account which owns the smart contract.
* `bye` = The name of the action to link to a permission. 
* `customp2` = The permission used to authorize the transaction.
* `-p bob@active` = The permission used to authorize setting the permissions.

**Example Output**
```shell
executed transaction: 4eb4cf3aea232d46e0e949bc273c3f0575be5bdba7b61851ab51d927cf74a838  128 bytes  141 us
#         eosio <= eosio::linkauth              {"account":"bob","code":"scontract1","type":"bye","requirement":"customp2"}
```

3. To remove the customp1 permission from the _hi_ action:

```shell
cleos set action permission bob scontract1 hi NULL -p bob@active
```

**Where**
* `bob` = The name of the account to link the custom permission authority.
* `scontract1`= The name of the account which owns the smart contract.
* `hi` = The name of the action to link to a permission. 
* `NULL` = Remove the permission.
* `-p bob@active` = The permission used to authorize the transaction.

**Example Output**
```shell
executed transaction: 50fe754760a1b8bd0e56f57570290a3f5daa509c090deb54c81a721ee7048201  120 bytes  242 us
#         eosio <= eosio::unlinkauth            {"account":"bob","code":"scontract1","type":"hi"}
```
## See Also
- [Accounts and Permissions](https://developers.eos.io/welcome/v2.1/protocol/accounts_and_permissions) protocol document.
- [Creating and Linking Custom Permissions](https://developers.eos.io/welcome/v2.1/smart-contract-guides/linking-custom-permission) tutorial.

