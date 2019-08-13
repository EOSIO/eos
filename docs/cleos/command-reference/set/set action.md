---
title: "set action"
excerpt: "`./cleos set action`"
---
## Description
Sets or updates an action's state on the blockchain.
## Info
**Command**

```shell
$ ./cleos set action
```
**Output**

```shell
ERROR: RequiredError: Subcommand required
set or update blockchain action state
Usage: ./cleos set action [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit

Subcommands:
  permission                  set parmaters dealing with account permissions
```
**Command**

```shell
$ ./cleos set action permission
```
**Output**
## Positionals
`code` _Type: Text_ - The account that owns the code for the action
`type` _Type: Text_ the type of the action
`requirement` _Type: Text, Default: Null_ - The permission name require for executing the given action
## Options
`-h,--help` Print this help message and exit
`-a,--abi' _Type:Text_ - The ABI for the contract
`-x,--expiration _Type:Text_ - set the time in seconds before a transaction expires, defaults to 30s
`-f,--force-unique` - force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
`-s,--skip-sign` Specify if unlocked wallet keys should be used to sign transaction
`-d,--dont-broadcast` - Don't broadcast transaction to the network (just print to stdout)
`-p,--permission`  _Type:Text_ - An account and permission level to authorize, as in 'account@permission' (defaults to 'account@active')

```shell
ERROR: RequiredError: account
set parmaters dealing with account permissions
Usage: ./cleos set action permission [OPTIONS] account code type requirement

Positionals:
  account TEXT                The account to set/delete a permission authority for
  code TEXT                   The account that owns the code for the action
  type TEXT                   the type of the action
  requirement TEXT            [delete] NULL, [set/update] The permission name require for executing the given action

Options:
  -h,--help                   Print this help message and exit
  -x,--expiration             set the time in seconds before a transaction expires, defaults to 30s
  -f,--force-unique           force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
  -s,--skip-sign              Specify if unlocked wallet keys should be used to sign transaction
  -d,--dont-broadcast         don't broadcast transaction to the network (just print to stdout)
  -p,--permission TEXT ...    An account and permission level to authorize, as in 'account@permission' (defaults to 'account@active')
```
