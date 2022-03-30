## Command

```sh
cleos get transaction [OPTIONS] id
```
**Where**:

* [`OPTIONS`] = See **Options** in **Command Usage** section below
* `id` = ID of the transaction to retrieve

## Description
Use this command to retrieve a transaction from the blockchain. 

## Command Usage

The following information shows the different positionals and options you can use with the `cleos get transaction` command:

```console
Usage: cleos get transaction [OPTIONS] id

Positionals:
  id TEXT                     ID of the transaction to retrieve (required)

Options:
  -b,--block-hint UINT        The block number this transaction may be in.
```

## Requirements

For prerequisites to run this command, see the **Before you Begin** section of the [_How to Get Transaction Information_](../02_how-to-guides/how-to-get-transaction-information.md) topic.


## Example

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

[[info | Important Note]]
| The above transaction id will not exist on your blockchain
