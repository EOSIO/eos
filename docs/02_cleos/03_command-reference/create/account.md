## Command 
```sh
cleos create account [OPTIONS] creator name OwnerKey [ActiveKey]
```
**Where:**
* [`OPTIONS`] = See **Options** in [**Command Usage**](#command-usage) section below
* `creator` = The name of the existing account creating the new account
* `name` = The name of the new account
* `OwnerKey` = The owner public key or permission level for the new account
* [`ActiveKey`] = The active public key or permission level for the new account

**Note:** The arguments and options enclosed in square brackets are optional.

## Description
Use this command to create a new account on an EOSIO blockchain. 

[[info | Note]]
| This command assumes system contract does not restrict RAM usage. 

## Command Usage
The following information shows the different positionals and options you can use with the `cleos create account` command:

### Positionals
* `creator` _TEXT_ REQUIRED - The name of the account creating the new account
* `name` _TEXT_ REQUIRED - The name of the new account
* `OwnerKey` _TEXT_ REQUIRED - The owner public key or permission level for the new account
* `ActiveKey` _TEXT_ - The active public key or permission level for the new account

### Options
* `-h,--help` - Print this help message and exit
* `-x,--expiration` - set the time in seconds before a transaction expires, defaults to 30s
* `-f,--force-unique` - force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
* `-s,--skip-sign` - Specify if unlocked wallet keys should be used to sign transaction
* `-j,--json` - print result as JSON
* `--json-file` _TEXT_ - save result in JSON format into a file
* `-d,--dont-broadcast` - don't broadcast transaction to the network (just print to `stdout`)
* `--return-packed` - used in conjunction with `--dont-broadcast` to get the packed transaction
* `-r,--ref-block` _TEXT_ - set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)
* `--use-old-rpc` - use old RPC push_transaction, rather than new RPC send_transaction
* `-p,--permission` _TEXT_ ... - An account and permission level to authorize, as in `account@permission` (defaults to `creator@active`)
* `--max-cpu-usage-ms` _UINT_ - set an upper limit on the milliseconds of CPU usage budget, for the execution of the transaction (defaults to 0 which means no limit)
* `--max-net-usage` UINT - set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)
* `--delay-sec` _UINT_ - set the `delay_sec` seconds, defaults to 0

## Requirements
For prerequisites to run this command, see the **Before you Begin** section of the [How to Create a Blockchain Account](../../02_how-to-guides/how-to-create-an-account.md) guide.

## Examples
The following examples demonstrate how to use the `cleos create account` command:

**Example 1.** Create a new account `tester` authorized by the creator account `inita`:
```sh
cleos create account inita tester EOS4toFS3YXEQCkuuw1aqDLrtHim86Gz9u3hBdcBw5KNPZcursVHq EOS7d9A3uLe6As66jzN8j44TXJUqJSK3bFjjEEqR4oTvNAB3iM9SA
```
```json
{
  "transaction_id": "6acd2ece68c4b86c1fa209c3989235063384020781f2c67bbb80bc8d540ca120",
  "processed": {
    "refBlockNum": "25217",
    "refBlockPrefix": "2095475630",
    "expiration": "2017-07-25T17:54:55",
    "scope": [
      "eos",
      "inita"
    ],
    "signatures": [],
    "messages": [{
        "code": "eos",
        "type": "newaccount",
        "authorization": [{
            "account": "inita",
            "permission": "active"
          }
        ],
        "data": "c9251a0000000000b44c5a2400000000010000000102bcca6347d828d4e1868b7dfa91692a16d5b20d0ee3d16a7ca2ddcc7f6dd03344010000010000000102bcca6347d828d4e1868b7dfa91692a16d5b20d0ee3d16a7ca2ddcc7f6dd03344010000010000000001c9251a000000000061d0640b000000000100010000000000000008454f5300000000"
      }
    ],
    "output": [{
        "notify": [],
        "sync_transactions": [],
        "async_transactions": []
      }
    ]
  }
}
```
