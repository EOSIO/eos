## Overview

This how-to guide provides instructions on how to submit, or push, a transaction using the `cleos` CLI tool.

## Before you begin

* Install the currently supported version of `cleos`

* Understand the following:
  * What a [transaction](https://developers.eos.io/welcome/latest/glossary/index/#transaction) is.
  * How to generate a valid transaction JSON.
    * Consult [cleos push transaction](https://developers.eos.io/manuals/eos/latest/cleos/command-reference/push/push-transaction) reference, and pay attention to option `-d` and `-j`.
    * Consult [push transaction](https://developers.eos.io/manuals/eos/latest/nodeos/plugins/chain_api_plugin/api-reference/index#operation/push_transaction) endpoint for chain api plug-in, and pay attention to the payload definition.

## Command Reference

See the following reference guides for command line usage and related options for the `cleos` command:

* The [cleos push transaction](https://developers.eos.io/manuals/eos/latest/cleos/command-reference/push/push-transaction) reference.

## Procedure

The following steps show how to:

1. [Create the transaction as JSON snippet.](#1-create-the-transaction-as-json-snippet)
2. [Send the transaction.](#2-send-the-transaction)

### 1. Create the transaction as JSON snippet

You can create the JSON snippet defining the transaction in two ways:

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

### 2. Send the transaction

You can send the transaction you created as JSON snippet in two ways:

* You can send the transaction stored in `TRX_FILE.json` file:

```sh
cleos push transaction TRX_FILE.json
```

* Alternatively, you can send the transaction using the JSON content directly:

```sh
cleos push transaction '{"expiration": "2019-08-01T07:15:49", "ref_block_num": 34881,"ref_block_prefix": 2972818865,"max_net_usage_words": 0,"max_cpu_usage_ms": 0,"delay_sec": 0,"context_free_actions": [],"actions": [{"account": "eosio.token","name": "transfer","authorization": [{"actor": "han","permission": "active"}],"data": {"from": "han","to": "eosio","quantity": "0.0001 SYS","memo": "m"}}],"transaction_extensions": [],"context_free_data": []}'
```

## Summary

In conclusion, the above instructions show how to submit, or push, a transaction using the `cleos` CLI tool.
