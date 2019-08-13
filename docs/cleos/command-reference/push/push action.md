---
title: "push action"
excerpt: "Push a transaction with a single action"
---
## Positionals
  `contract` _Type: Text_ - The account providing the contract to execute
  `action' _Type: Text_ - The action to execute on the contract
  `data` _Type: Text_ - The arguments to the contract

**Output**
## Options
 ` -h,--help` - Print this help message and exit
 `-x,--expiration` - set the time in seconds before a transaction expires, defaults to 30s
 `-f,--force-unique` - force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
` -s,--skip-sign` - Specify if unlocked wallet keys should be used to sign transaction
`-d,--dont-broadcast` - don't broadcast transaction to the network (just print to stdout)
`-p,--permission` _Type: Text_ - An account and permission level to authorize, as in 'account@permission'
## Example


```shell
$ ./cleos push action currency transfer '{"from":"USER_A","to":"USER_B","quantity":"20.0000 CUR","memo":""}' --permission USERA@active
```
