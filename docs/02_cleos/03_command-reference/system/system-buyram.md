
## Command
cleos system buyram [OPTIONS] payer receiver amount

**Where**
* [OPTIONS] = See Options in Command Usage section below.
* payer = The account paying for RAM. 
* receiver = The account receiving bought RAM.
* amount = The amount of EOS to pay for RAM

**Note**: The arguments and options enclosed in square brackets are optional.

## Description
Use this command to buy RAM for a blockchain account on EOSIO.

## Command Usage
The following information shows the different positionals and options you can use with the `cleos system buyram` command:

### Positionals:
- `payer` _TEXT_ - The account paying for RAM
- `receiver` _TEXT_ - The account receiving bought RAM
- `amount` _TEXT_ - The amount of EOS to pay for RAM

### Options
- `-h,--help` - Print this help message and exit
- `-k,--kbytes` - Buyram in number of kibibytes (KiB)
- `-b,--bytes` - Buyram in number of bytes
- `-x,--expiration` _TEXT_ - Set the time in seconds before a transaction expires, defaults to 30s
- `-f,--force-unique` - Force the transaction to be unique. This will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
- `-s,--skip-sign` - Specify if unlocked wallet keys should be used to sign transaction
- `-j,--json` - Print result as json
- `--json-file` _TEXT_ - save result in json format into a file
- `-d,--dont-broadcast` - Don't broadcast transaction to the network (just print to stdout)
- `--return-packed` - Used in conjunction with --dont-broadcast to get the packed transaction
- `-r,--ref-block` _TEXT_ - Set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)
- ` --use-old-rpc` - Use old RPC push_transaction, rather than new RPC send_transaction
- `-p,--permission` _TEXT_ - An account and permission level to authorize, as in 'account@permission' (defaults to 'account@active')
- `--max-cpu-usage-ms` _UINT_ - Set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)
- `--max-net-usage` _UINT_ - Set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)
- `--delay-sec` _UINT_ - Set the delay_sec seconds, defaults to 0s

## Requirements
For the prerequisites to run this command see the Before you Begin section of [How to Buy Ram](../../02_how-to-guides/how-to-buy-ram.md)  

## Examples
* [How to Buy Ram](../../02_how-to-guides/how-to-buy-ram.md)
