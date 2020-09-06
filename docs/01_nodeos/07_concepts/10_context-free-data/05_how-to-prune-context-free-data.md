---
content_title: How to prune context-free data
link_text: How to prune context-free data
---

## Summary

This how-to procedure showcases how to prune context-free data (CFD) from a transaction. The process involves launching the [`eosio-blocklog`](../../../10_utilities/05_eosio-blocklog.md) utility with the `--prune-transactions` option, the transaction ID(s) that contain(s) the context-free data, and additional options as specified below.

[[info | Data Pruning on Public Chains]]
| Pruning transaction data is not suitable for public EOSIO blockchains, unless previously agreed upon through EOSIO consensus by a supermajority of producers. Even if a producing node on a public EOSIO network prunes context-free data from a transaction, only their node would be affected. The integrity of the blockchain would not be compromised.

## Prerequisites

* The ID of a retired transaction with context-free data in a finalized block.
* Get familiar with the [context-free data](index.md) section of a transaction.
* Read the [`eosio-blocklog`](../../../10_utilities/05_eosio-blocklog.md) command-line utility reference.

## Procedure

Complete the following steps to prune the context-free data from the transaction:

1. Locate the transaction ID you want to prune the context-free data from, say `<trx_id>`. The transaction ID can also be found on the `id` field of the transaction.
2. Locate the block number that contains the transaction, say `<block_num>`. The block number can also be found on the `block_num` field of the transaction.
3. Find the blocks directory and state history directory (if applicable), say `<blocks_dir>` and `<state_hist_dir>`, respectively.
4. Launch the [`eosio-blocklog`](../../../10_utilities/05_eosio-blocklog.md) utility as follows:

   `eosio-blocklog [--blocks-dir <blocks_dir>] [--state-history-dir <state_hist_dir>] --prune-transactions --block-num <block_num> --transaction <trx_id> [--transaction <trx_id2> ...]`

   If successful:
   * The [`eosio-blocklog`](../../../10_utilities/05_eosio-blocklog.md) utility terminates silently with a zero error code (no error).
   * The following fields are updated within the pruned transaction from the block logs:
      * the `prunable_data["prunable_data"][0]` field is set from 0 to 1.
      * the `signatures` field is set to an empty array.
      * the `context_free_data` field is set to an empty array.
      * the `packed_context_free_data` field, if any, is removed.

   If unsuccessful:
   * The [`eosio-blocklog`](../../../10_utilities/05_eosio-blocklog.md) utility outputs an error to `stderr` and terminates with a non-zero error code (indicating an error).

## Notes

* You can pass multiple transactions to [`eosio-blocklog`](../../../10_utilities/05_eosio-blocklog.md) if they are within the same block.
* You can use [`eosio-blocklog`](../../../10_utilities/05_eosio-blocklog.md) to display the block that contains the pruned transactions.

## Example

Refer to the following transaction with context-free data:

```json
{
 "id": "7add73de37bbd9072bf76c62aa8ec1dd1b17a0fa8e368162a4a950905cfbe1e6",
 "trx": {
  "receipt": {
   "status": "executed",
   "cpu_usage_us": 195,
   "net_usage_words": 14,
   "trx": [
    1,
    {
     "compression": "none",
     "prunable_data": {
      "prunable_data": [
       0,
       {
        "signatures": [
         "SIG_K1_Kcs5fDX5hpB1zpsEVudk81J91RN14xUgpVomqM9NkW8H58SLNx3qT9ZDnKK8PiUwwXpTEyGexmSTiF4K4n5HReH8jaRT2t"
        ],
        "packed_context_free_data": "0203a1b2c3031a2b3c"
       }
      ]
     },
     "packed_trx": "4475525f7200ccbae7b80000000100305631191abda90000000000901d4d00000100305631191abda90000000000901d4d0100305631191abda900000000a8ed32320000"
    }
   ]
  },
  "trx": {
   "expiration": "2020-09-04T17:11:32",
   "ref_block_num": 114,
   "ref_block_prefix": 3102194380,
   "max_net_usage_words": 0,
   "max_cpu_usage_ms": 0,
   "delay_sec": 0,
   "context_free_actions": [
    {
     "account": "payloadless",
     "name": "doit",
     "authorization": [],
     "data": ""
    }
   ],
   "actions": [
    {
     "account": "payloadless",
     "name": "doit",
     "authorization": [
      {
       "actor": "payloadless",
       "permission": "active"
      }
     ],
     "data": ""
    }
   ],
   "signatures": [
    "SIG_K1_Kcs5fDX5hpB1zpsEVudk81J91RN14xUgpVomqM9NkW8H58SLNx3qT9ZDnKK8PiUwwXpTEyGexmSTiF4K4n5HReH8jaRT2t"
   ],
   "context_free_data": [
    "a1b2c3",
    "1a2b3c"
   ]
  }
 },
 "block_time": "2020-09-04T17:11:02.500",
 "block_num": 116,
 "last_irreversible_block": 126,
 "traces": [
  {
   "action_ordinal": 1,
   "creator_action_ordinal": 0,
   "closest_unnotified_ancestor_action_ordinal": 0,
   "receipt": {
    "receiver": "payloadless",
    "act_digest": "4f09a630d4456585ee4ec5ef96c14151587367ad381f9da445b6b6239aae82cf",
    "global_sequence": 153,
    "recv_sequence": 2,
    "auth_sequence": [],
    "code_sequence": 1,
    "abi_sequence": 1
   },
   "receiver": "payloadless",
   "act": {
    "account": "payloadless",
    "name": "doit",
    "authorization": [],
    "data": ""
   },
   "context_free": true,
   "elapsed": 204,
   "console": "Im a payloadless action",
   "trx_id": "7add73de37bbd9072bf76c62aa8ec1dd1b17a0fa8e368162a4a950905cfbe1e6",
   "block_num": 116,
   "block_time": "2020-09-04T17:11:02.500",
   "producer_block_id": null,
   "account_ram_deltas": [],
   "account_disk_deltas": [],
   "except": null,
   "error_code": null,
   "return_value_hex_data": ""
  },
  {
   "action_ordinal": 2,
   "creator_action_ordinal": 0,
   "closest_unnotified_ancestor_action_ordinal": 0,
   "receipt": {
    "receiver": "payloadless",
    "act_digest": "b8871e8f3c79b02804a2ad28acb015f503e7f6e56f35565e5fa37b6767da1aa5",
    "global_sequence": 154,
    "recv_sequence": 3,
    "auth_sequence": [
     [
      "payloadless",
      3
     ]
    ],
    "code_sequence": 1,
    "abi_sequence": 1
   },
   "receiver": "payloadless",
   "act": {
    "account": "payloadless",
    "name": "doit",
    "authorization": [
     {
      "actor": "payloadless",
      "permission": "active"
     }
    ],
    "data": ""
   },
   "context_free": false,
   "elapsed": 13,
   "console": "Im a payloadless action",
   "trx_id": "7add73de37bbd9072bf76c62aa8ec1dd1b17a0fa8e368162a4a950905cfbe1e6",
   "block_num": 116,
   "block_time": "2020-09-04T17:11:02.500",
   "producer_block_id": null,
   "account_ram_deltas": [],
   "account_disk_deltas": [],
   "except": null,
   "error_code": null,
   "return_value_hex_data": ""
  }
 ]
}
```

Using the above transaction and recreating the steps listed above:

1. Locate the transaction ID: `7add73de37bbd9072bf76c62aa8ec1dd1b17a0fa8e368162a4a950905cfbe1e6`.
2. Locate the block number: `116`.
3. Find the blocks directory and state history directory (if applicable), say `<blocks_dir>` and `<state_hist_dir>`.
4. Launch the [`eosio-blocklog`](../../../10_utilities/05_eosio-blocklog.md) utility as follows:

   `eosio-blocklog --blocks-dir <blocks_dir> --state-history-dir <state_hist_dir> --prune-transactions --block-num 116 --transaction 7add73de37bbd9072bf76c62aa8ec1dd1b17a0fa8e368162a4a950905cfbe1e6`

The utility will return silently if successful, or output an error to `stdout` otherwise.

After retrieving the transaction a second time, the pruned transaction looks as follows:

```json
{
 "id": "7add73de37bbd9072bf76c62aa8ec1dd1b17a0fa8e368162a4a950905cfbe1e6",
 "trx": {
  "receipt": {
   "status": "executed",
   "cpu_usage_us": 195,
   "net_usage_words": 14,
   "trx": [
    1,
    {
     "compression": "none",
     "prunable_data": {
      "prunable_data": [
       1,
       {
        "digest": "6b4f25b58b02e6469a77685248faee9ae0d2c7574a12e5fbaf5d00f1b5bfbd33"
       }
      ]
     },
     "packed_trx": "4475525f7200ccbae7b80000000100305631191abda90000000000901d4d00000100305631191abda90000000000901d4d0100305631191abda900000000a8ed32320000"
    }
   ]
  },
  "trx": {
   "expiration": "2020-09-04T17:11:32",
   "ref_block_num": 114,
   "ref_block_prefix": 3102194380,
   "max_net_usage_words": 0,
   "max_cpu_usage_ms": 0,
   "delay_sec": 0,
   "context_free_actions": [
    {
     "account": "payloadless",
     "name": "doit",
     "authorization": [],
     "data": ""
    }
   ],
   "actions": [
    {
     "account": "payloadless",
     "name": "doit",
     "authorization": [
      {
       "actor": "payloadless",
       "permission": "active"
      }
     ],
     "data": ""
    }
   ],
   "signatures": [],
   "context_free_data": []
  }
 },
 "block_time": "2020-09-04T17:11:02.500",
 "block_num": 116,
 "last_irreversible_block": 129,
 "traces": [
  {
   "action_ordinal": 1,
   "creator_action_ordinal": 0,
   "closest_unnotified_ancestor_action_ordinal": 0,
   "receipt": {
    "receiver": "payloadless",
    "act_digest": "4f09a630d4456585ee4ec5ef96c14151587367ad381f9da445b6b6239aae82cf",
    "global_sequence": 153,
    "recv_sequence": 2,
    "auth_sequence": [],
    "code_sequence": 1,
    "abi_sequence": 1
   },
   "receiver": "payloadless",
   "act": {
    "account": "payloadless",
    "name": "doit",
    "authorization": [],
    "data": ""
   },
   "context_free": true,
   "elapsed": 204,
   "console": "Im a payloadless action",
   "trx_id": "7add73de37bbd9072bf76c62aa8ec1dd1b17a0fa8e368162a4a950905cfbe1e6",
   "block_num": 116,
   "block_time": "2020-09-04T17:11:02.500",
   "producer_block_id": null,
   "account_ram_deltas": [],
   "account_disk_deltas": [],
   "except": null,
   "error_code": null,
   "return_value_hex_data": ""
  },
  {
   "action_ordinal": 2,
   "creator_action_ordinal": 0,
   "closest_unnotified_ancestor_action_ordinal": 0,
   "receipt": {
    "receiver": "payloadless",
    "act_digest": "b8871e8f3c79b02804a2ad28acb015f503e7f6e56f35565e5fa37b6767da1aa5",
    "global_sequence": 154,
    "recv_sequence": 3,
    "auth_sequence": [
     [
      "payloadless",
      3
     ]
    ],
    "code_sequence": 1,
    "abi_sequence": 1
   },
   "receiver": "payloadless",
   "act": {
    "account": "payloadless",
    "name": "doit",
    "authorization": [
     {
      "actor": "payloadless",
      "permission": "active"
     }
    ],
    "data": ""
   },
   "context_free": false,
   "elapsed": 13,
   "console": "Im a payloadless action",
   "trx_id": "7add73de37bbd9072bf76c62aa8ec1dd1b17a0fa8e368162a4a950905cfbe1e6",
   "block_num": 116,
   "block_time": "2020-09-04T17:11:02.500",
   "producer_block_id": null,
   "account_ram_deltas": [],
   "account_disk_deltas": [],
   "except": null,
   "error_code": null,
   "return_value_hex_data": ""
  }
 ]
}
```

## Next Steps

The following options are available when you complete the procedure:

* Verify that the pruned transaction indeed contains pruned context-free data.
* Display the block that contains the pruned transaction from the block logs.
