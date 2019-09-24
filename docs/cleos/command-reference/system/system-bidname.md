## Description

Place a bid on "premium" usernames (account names less than 12 characters)

## Positionals
* `bidder` _TEXT_ - The bidding account (required)
* `newname` _TEXT_  - The bidding name (required)
* `bid` _TEXT_ - The amount of CORE SYMBOL to bid (required)
## Options
* `-h,--help` - Print this help message and exit
* `-x,--expiration` - set the time in seconds before a transaction expires, defaults to 30s
* `-f,--force-unique` - force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
* `-s,--skip-sign` - Specify if unlocked wallet keys should be used to sign transaction
* `-j,--json` - print result as json
* `-d,--dont-broadcast` - don't broadcast transaction to the network (just print to stdout)
* `-r,--ref-block` _TEXT_ - set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)
* `-p,--permission` _TEXT_ - An account and permission level to authorize, as in 'account@permission'
* `--max-cpu-usage-ms` _UINT_ - set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)
* `--max-net-usage` _UINT_ - set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)
- `--delay-sec` _UINT_            set the delay_sec seconds, defaults to 0s
- `-j,--json` print result as json

## Usage


```shell
cleos system bidname accountname1 bob "10 SYS"
```
