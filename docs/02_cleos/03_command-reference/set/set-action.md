## Description
Sets or updates an action's state on the blockchain.

**Command**

```sh
cleos set action
```
**Output**

```console
Usage: cleos set action [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit

Subcommands:
  permission                  set parmaters dealing with account permissions
```
**Command**

```sh
cleos set action permission
```

## Positionals

`account` TEXT The account to set/delete a permission authority for (required

`code` _Text_ The account that owns the code for the action

`type` _Type_ the type of the action

`requirement` _Type_ The permission name require for executing the given action

## Options
`-h,--help` Print this help message and exit

`-x,--expiration` _Type:Text_ - set the time in seconds before a transaction expires, defaults to 30s

`-f,--force-unique` - force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times

`-s,--skip-sign` Specify if unlocked wallet keys 
should be used to sign transaction

`-j,--json` print result as json

`-d,--dont-broadcast` - Don't broadcast transaction to the network (just print to stdout)

`--return-packed` used in conjunction with --dont-broadcast to get the packed transaction

`-r,--ref-block` _TEXT_         set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)

`-p,--permission`  _Type:Text_ - An account and permission level to authorize, as in 'account@permission' (defaults to 'account@active')

`--max-cpu-usage-ms` _UINT_ - Set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)

`--max-net-usage` _UINT_ - Set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)

`--delay-sec` _UINT_ - set the delay_sec seconds, defaults to 0s

## Usage

```sh
#Link a `voteproducer` action to the 'voting' permissions
cleos set action permission sandwichfarm eosio.system voteproducer voting -p sandwichfarm@voting

#Now can execute the transaction with the previously set permissions. 
cleos system voteproducer approve sandwichfarm someproducer -p sandwichfarm@voting
```

## See Also
- [Accounts and Permissions](https://developers.eos.io/welcome/v2.1/protocol/accounts_and_permissions) protocol document.
