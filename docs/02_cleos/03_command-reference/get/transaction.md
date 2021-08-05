## Command
```sh
cleos get transaction [OPTIONS] id
```
**Where**:
* [`OPTIONS`] = See **Options** in [**Command Usage**](#command-usage) section below
* `id` = ID of the transaction to retrieve

**Note:** The arguments and options enclosed in square brackets are optional.

## Description
Use this command to retrieve a transaction from the blockchain. 

## Command Usage
The following information shows the different positionals and options you can use with the `cleos get transaction` command:

### Positionals
* `id` _TEXT_ REQUIRED - ID of the transaction to retrieve

### Options
* `-h,--help` - Print this help message and exit
* `-b,--block-hint` _UINT_ - The block number this transaction may be in

## Requirements
For prerequisites to run this command, see the **Before you Begin** section of the [How to Get Transaction Information](../../02_how-to-guides/how-to-get-transaction-information.md) topic.

## Examples
The following examples demonstrate the `cleos get transaction` command:

**Example 1.** Retrieve transaction information for transaction ID `eb4b94b7...b703`:
```sh
cleos get transaction eb4b94b72718a369af09eb2e7885b3f494dd1d8a20278a6634611d5edd76b703
```
```json
{
  "transaction_id": "eb4b94b72718a369af09eb2e7885b3f494dd1d8a20278a6634611d5edd76b703",
  "processed": {
    "refBlockNum": 2206,
    "refBlockPrefix": 221394282,
    "expiration": "2017-09-05T08:03:58",
    "scope": [
      "inita",
      "tester"
    ],
    "signatures": [
      "1f22e64240e1e479eee6ccbbd79a29f1a6eb6020384b4cca1a958e7c708d3e562009ae6e60afac96f9a3b89d729a50cd5a7b5a7a647540ba1678831bf970e83312"
    ],
    "messages": [{
        "code": "eos",
        "type": "transfer",
        "authorization": [{
            "account": "inita",
            "permission": "active"
          }
        ],
        "data": {
          "from": "inita",
          "to": "tester",
          "amount": 1000,
          "memo": ""
        },
        "hex_data": "000000008040934b00000000c84267a1e80300000000000000"
      }
    ],
    "output": [{
        "notify": [{
            "name": "tester",
            "output": {
              "notify": [],
              "sync_transactions": [],
              "async_transactions": []
            }
          },{
            "name": "inita",
            "output": {
              "notify": [],
              "sync_transactions": [],
              "async_transactions": []
            }
          }
        ],
        "sync_transactions": [],
        "async_transactions": []
      }
    ]
  }
}
```

[[info | Note]]
| The above transaction ID will not exist on your actual EOSIO blockchain.
