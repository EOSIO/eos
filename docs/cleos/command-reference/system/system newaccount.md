---
title: "system newaccount"
excerpt: "Create an account, buy ram, stake for bandwidth for the account"
---
## Positional Arguments
- `creator` _TEXT_  - The name of the account creating the new account
- `name` _TEXT_  - The name of the new account
- `OwnerKey` _TEXT_  - The owner public key for the new account
- `ActiveKey` _TEXT_  - The active public key for the new account
## Options
- `-h,--help` Print this help message and exit
- `--stake-net` _TEXT_ - The amount of EOS delegated for net bandwidth
- `--stake-cpu` _TEXT_  - The amount of EOS delegated for CPU bandwidth
- `--buy-ram-bytes` _UINT_ - The amount of RAM bytes to purchase for the new account in kilobytes KiB, default is 8 KiB
- `--buy-ram-EOS` _TEXT_ - The amount of RAM bytes to purchase for the new account in EOS
- `--transfer` - Transfer voting power and right to unstake EOS to receiver
- `-x,--expiration` _TEXT_ - set the time in seconds before a transaction expires, defaults to 30s
- `-f,--force-unique` - force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
- `-s,--skip-sign` Specify if unlocked wallet keys should be used to sign transaction
- `-d,--dont-broadcast` - Don't broadcast transaction to the network (just print to stdout)
- `-r,--ref-block` _TEXT_         set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)
- `-p,--permission`  _TEXT_ - An account and permission level to authorize, as in 'account@permission' (defaults to 'account@active')
- `--max-cpu-usage-ms` _UINT_ - set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)
- `--max-net-usage` _UINT_ - set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)
## Examples
