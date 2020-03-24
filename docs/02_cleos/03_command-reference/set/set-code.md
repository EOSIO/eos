## Description
Sets or updates an account's code on the blockchain.

## Positionals

* `account` _TEXT_ - The account to set code for (required)
* `code-file` _TEXT_ - The fullpath containing the contract WAST or WASM (required)

## Options

`-h,--help` Print this help message and exit
`-a,--abi` _TEXT_ - The ABI for the contract

`-c,--clear` Remove contract on an account

`--suppress-duplicate-check`  Don't check for duplicate

`-x,--expiration` _TEXT_ - set the time in seconds before a transaction expires, defaults to 30s

`-f,--force-unique` - force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times

`-s,--skip-sign` Specify if unlocked wallet keys should be used to sign transaction

`-j,--json` print result as json

`-d,--dont-broadcast` - Don't broadcast transaction to the network (just print to stdout)

`--return-packed` used in conjunction with --dont-broadcast to get the packed transaction

`-r,--ref-block` _TEXT_         set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)

`-p,--permission`  _Type:Text_ - An account and permission level to authorize, as in 'account@permission' (defaults to 'account@active')

* `-r,--ref-block` _TEXT_         set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)
* `-p,--permission`  _TEXT_ - An account and permission level to authorize, as in 'account@permission' (defaults to 'account@active')
* `--max-cpu-usage-ms` _UINT_ - set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)
* `--max-net-usage` _UINT_ - set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)
* `--delay-sec` _UINT_ - set the delay_sec seconds, defaults to 0s

```sh
cleos set code someaccount1 ./path/to/wasm
```
