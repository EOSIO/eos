## Goal

Push a transaction

## Before you begin

* Install the currently supported version of `cleos`

* Understand the following:
  * What is a [transaction](https://developers.eos.io/welcome/latest/glossary/index/#transaction).
  * How to generate a valid transaction JSON.
    * Consult [cleos push transaction reference](https://developers.eos.io/manuals/eos/latest/cleos/command-reference/push/push-transaction), and pay attention to option `-d` and `-j`.
    * Consult [push transaction operation](https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/push_transactions) for chain api plug-in, and pay attention to the payload definition.

## Steps

1. Create the JSON snippet which contains the transaction.

* Create a JSON snippet which contains a valid transaction and uses hex format for `data` field.

```JSON
{
  "expiration": "2019-08-01T07:15:49",
  "ref_block_num": 34881,
  "ref_block_prefix": 2972818865,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio.token",
      "name": "transfer",
      "authorization": [{
          "actor": "han",
          "permission": "active"
        }
      ],
      "data": "000000000000a6690000000000ea305501000000000000000453595300000000016d"
    }
  ],
  "transaction_extensions": [],
  "context_free_data": []
}
```

* Alternatively, you can also create a JSON snippet that uses clear text JSON for `data` field.

[[info]]
| Be aware that if a clear text `data` field is used, `cleos` needs to fetch the smart contract's ABI using `nodeos` API. This operation has an overall performance overhead for both `cleos` and `nodeos`. On the other hand if hex data is used in the `data` field then the ABI fetching is not executed and thus the total time to send and execute the transaction is faster.

```json
{
  "expiration": "2019-08-01T07:15:49",
  "ref_block_num": 34881,
  "ref_block_prefix": 2972818865,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio.token",
      "name": "transfer",
      "authorization": [{
          "actor": "han",
          "permission": "active"
        }
      ],
      "data": {
        "from": "han",
        "to": "eosio",
        "quantity": "0.0001 SYS",
        "memo": "m"
      }
    }
  ],
  "transaction_extensions": [],
  "context_free_data": []
}
```

2. Send the transaction packed in a JSON file or using a direct JSON text

* Send the transaction stored in `TRX_FILE.json` file:

```sh
cleos push transaction TRX_FILE.json
```

* Send the transaction using the JSON content directly:

```sh
cleos push transaction '{"expiration": "2019-08-01T07:15:49", "ref_block_num": 34881,"ref_block_prefix": 2972818865,"max_net_usage_words": 0,"max_cpu_usage_ms": 0,"delay_sec": 0,"context_free_actions": [],"actions": [{"account": "eosio.token","name": "transfer","authorization": [{"actor": "han","permission": "active"}],"data": {"from": "han","to": "eosio","quantity": "0.0001 SYS","memo": "m"}}],"transaction_extensions": [],"context_free_data": []}'
```
