## Command
cleos push transaction [OPTIONS]

**Where**
* [OPTIONS] = See Options in Command Usage section below. You must choose one option of `--file` or `--to-console`

**Note**: The arguments and options enclosed in square brackets are optional.

## Description
Push an arbitrary JSON transaction

## Command Usage
The following information shows the different positionals and options you can use with the `cleos push transaction` command:

### Positionals:
- `transaction` (text) The JSON of the transaction to push, or the name of a JSON file containing the transaction

### Options
- `-h,--help` - Print this help message and exit
- `-x,--expiration` - Set the time in seconds before a transaction expires, defaults to 30s
- - `-f,--force-unique` - Force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
`-s,--skip-sign` - Specify if unlocked wallet keys should be used to sign transaction
- `-j,--json` - Print result as JSON
- `--json-file` _TEXT_ - Save result in JSON format into a file
- `-d,--dont-broadcast` - Don't broadcast transaction to the network (just print to stdout)
- `--return-packed` - Used in conjunction with --dont-broadcast to get the packed transaction
- `-r,--ref-block` _TEXT_ - Set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)
- `--use-old-rpc` - Use old RPC push_transaction, rather than new RPC send_transaction
- `--compression` _ENUM:value in {none->0,zlib->1} OR {0,1}_ - Compression for transaction 'none' or 'zlib'
- `-p,--permission` _TEXT_ - An account and permission level to authorize, as in 'account@permission'
- `--max-cpu-usage-ms` _UINT_ - Set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)
- `--max-net-usage` _UINT_ - Set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)
- `--delay-sec` _UINT_ - Set the delay_sec seconds, defaults to 0s
- `--sign-with` _TEXT_ - The public key or json array of public keys to sign with
- `-o,--read-only` - Specify a transaction is read-only
- `-t,--return-failure-trace` - Return partial traces on failed transactions, use it along with --read-only)


## Requirements
* Install version 2.2 or greater of `cleos.`
[[info | Note]]
| The cleos tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the cleos comand line tool and the keosd key store or wallet. 

## Examples

1. Basic usage
```shell
cleos push transaction {}
```

2. Call a read-only query
```shell
cleos push transaction --read-only -j '{"actions":[{"account":"scholder","name":"get","authorization":[{"actor":"bob","permission":"active"}],"data":""}]}' -p bob@active
```

**Where**
- `--read-only` = Tells the `cleos push transaction` command that this is calling a read-only query.  
- `-j` = Tells the `cleos push transaction` to print the result as json. 
- `'{"actions":[{"account":"scholder","name":"get","authorization":[{"actor":"bob","permission":"active"}],"data":""}]}'` = The transaction to call.
- `-p bob@active` - The account permission.

**Example Output**
This will vary depending on the results from the read-only query.

```json
{
  "head_block_num": 90989,
  "head_block_id": "0001636d2c9e67191f36b37d7e6f71380449517659713bcf5e042faa0033fe8e",
  "last_irreversible_block_num": 90988,
  "last_irreversible_block_id": "0001636ce850a330375b6e78626647caadf61038344f5314147926faef692c03",
  "code_hash": "3db3fe6580e4cf39816674418d57b20f51a2427cb447345657f02cf37fa0e8cf",
  "pending_transactions": [],
  "result": {
    "id": "7f414329200ca641675d326b01b7bc595b639e6d9012f1308296c6721fb76c75",
    "block_num": 90990,
    "block_time": "2021-06-03T10:01:19.000",
    "producer_block_id": null,
    "receipt": {
      "status": "executed",
      "cpu_usage_us": 150,
      "net_usage_words": 12
    },
    "elapsed": 2549,
    "net_usage": 96,
    "scheduled": false,
    "action_traces": [{
        "action_ordinal": 1,
        "creator_action_ordinal": 0,
        "closest_unnotified_ancestor_action_ordinal": 0,
        "receipt": {
          "receiver": "scholder",
          "act_digest": "c14ab63292ffd6d2ebcd615fe2ab3eb4be2a6ddca690ca47cc2584cb00af9c67",
          "global_sequence": 91008,
          "recv_sequence": 6,
          "auth_sequence": [[
              "bob",
              6
            ]
          ],
          "code_sequence": 3,
          "abi_sequence": 3
        },
        "receiver": "scholder",
        "act": {
          "account": "scholder",
          "name": "get3",
          "authorization": [{
              "actor": "bob",
              "permission": "active"
            }
          ],
          "hex_data": ""
        },
        "context_free": false,
        "elapsed": 2457,
        "console": "",
        "trx_id": "7f414329200ca641675d326b01b7bc595b639e6d9012f1308296c6721fb76c75",
        "block_num": 90990,
        "block_time": "2021-06-03T10:01:19.000",
        "producer_block_id": null,
        "account_ram_deltas": [],
        "account_disk_deltas": [],
        "except": null,
        "error_code": null,
        "return_value_hex_data": "07045068696c010000003300000005446176696401000000230000000943687269737469616e01000000320000000647656f726765010000001500000003537565000000001f00000004526974610000000015000000054e616e63790000000033000000",
        "return_value_data": [{
            "name": "Phil",
            "gender": 1,
            "age": 51
          },{
            "name": "Nancy",
            "gender": 0,
            "age": 35
          },{
            "name": "Christian",
            "gender": 1,
            "age": 50
          },{
            "name": "Rita",
            "gender": 0,
            "age": 21
          },{
            "name": "Sue",
            "gender": 0,
            "age": 31
          },{
            "name": "Bob",
            "gender": 1,
            "age": 21
          },{
            "name": "David",
            "gender": 1,
            "age": 51
          }
        ]
      }
    ],
    "account_ram_delta": null,
    "except": null,
    "error_code": null
  }
}
```
