## Overview
This guide provides instructions on calling a smart contract read-only query.   

The example uses `cleos push transaction` to call a read-only query.  A readonly query can be called using either the `--read-only` option or the `-o` option.  

## Before you Begin
Make sure you meet the following requirements: 

* Install version 2.2, or greater, of `cleos`  
[[info | Note]]
| `Cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the `cleos` and `keosd` comand line tools. 
* You have loaded a smart contract, containing a read-only query to an EOSIO blockchain see 

## Command Reference
See the following reference guides for command line usage and related options:

* [cleos push transaction](../03_command-reference/push/push-transaction.md) command

## Examples

The following examples show you how to call read-only queries:

Example 1. Run the following command to call a read-only query _get_, with no parameters, on the _scholder_ account to which a smart contract containing a read-only query is deployed. The transaction is authorised by _bob@active_

```shell
cleos push transaction --read-only -j '{"actions":[{"account":"scholder","name":"get","authorization":[{"actor":"bob","permission":"active"}],"data":""}]}' -p bob@active
```

**Where**
- `--read-only` = Tells the `cleos push transaction` command that this is calling a read-only query.  
- `-j` = Tells the `cleos push transaction` to print the result as json. 
- `'{"actions":[{"account":"scholder","name":"get","authorization":[{"actor":"bob","permission":"active"}],"data":""}]}'` = The transaction to call.
- `-p bob@active` - The account permission used to call the action.

**Example Output**
```json
{
  "head_block_num": 89230,
  "head_block_id": "00015c8e67e66b8bb1daeeb6452fefea22509d0ef99381f34ed4b1486036bdbd",
  "last_irreversible_block_num": 89229,
  "last_irreversible_block_id": "00015c8d117aa070cea8bbe8bae16d27cfa321d54deaa89b2125b5d2f5a4922b",
  "code_hash": "89959d556cf0a0863064269f93dbc69c3b1e3b278385d495fc59f3509f77a6fb",
  "pending_transactions": [],
  "result": {
    "id": "5394a83aebf2c43dd007473bd917ef4d7461c73395b1b504df6b853787f4967f",
    "block_num": 89231,
    "block_time": "2021-06-03T09:46:39.500",
    "producer_block_id": null,
    "receipt": {
      "status": "executed",
      "cpu_usage_us": 126,
      "net_usage_words": 12
    },
    "elapsed": 3562,
    "net_usage": 96,
    "scheduled": false,
    "action_traces": [{
        "action_ordinal": 1,
        "creator_action_ordinal": 0,
        "closest_unnotified_ancestor_action_ordinal": 0,
        "receipt": {
          "receiver": "scholder",
          "act_digest": "ffe396998865e412f71cb314f5b4fec9a05cb7a043f2bcc38e3e5bff70f6cfe7",
          "global_sequence": 89247,
          "recv_sequence": 6,
          "auth_sequence": [[
              "bob",
              6
            ]
          ],
          "code_sequence": 2,
          "abi_sequence": 2
        },
        "receiver": "scholder",
        "act": {
          "account": "scholder",
          "name": "get",
          "authorization": [{
              "actor": "bob",
              "permission": "active"
            }
          ],
          "hex_data": ""
        },
        "context_free": false,
        "elapsed": 3480,
        "console": "",
        "trx_id": "5394a83aebf2c43dd007473bd917ef4d7461c73395b1b504df6b853787f4967f",
        "block_num": 89231,
        "block_time": "2021-06-03T09:46:39.500",
        "producer_block_id": null,
        "account_ram_deltas": [],
        "account_disk_deltas": [],
        "except": null,
        "error_code": null,
        "return_value_hex_data": "045068696c0100000033000000",
        "return_value_data": [{
				"name": "Alice",
				"gender": 0,
				"age": 19
        		},
				{
				"name": "Bob",
				"gender": 1,
				"age": 19
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

## Summary
In conclusion, by following these instructions you are able to call a smart contract read-only action.
