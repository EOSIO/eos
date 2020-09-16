---
content_title: How to prune context-free data
link_text: How to prune context-free data
---

## Summary

This how-to procedure showcases the steps to prune context-free data (CFD) from a transaction. The process involves launching the [`eosio-blocklog`](../../../10_utilities/eosio-blocklog.md) utility with the `--prune-transactions` option, the transaction ID(s) that contain(s) the context-free data, and additional options as specified below.

[[caution | Data Pruning on Public Chains]]
| Pruning transaction data is not suitable for public EOSIO blockchains, unless previously agreed upon through EOSIO consensus by a supermajority of producers. Even if a producing node on a public EOSIO network prunes context-free data from a transaction, only their node would be affected. The integrity of the blockchain would not be compromised.

## Prerequisites

The following items must be known in advance or completed before starting the procedure:

* The ID of a retired transaction with context-free data in a finalized block.
* Become familiar with the [context-free data](index.md) section of a transaction.
* Review the [`eosio-blocklog`](../../../10_utilities/eosio-blocklog.md) command-line utility reference.

## Procedure

Complete the following steps to prune the context-free data from the transaction:

1. Locate the transaction ID you want to prune the context-free data from, e.g. `<trx_id>`. The transaction ID can also be found on the `id` field of the transaction.
2. Locate the block number that contains the transaction, e.g. `<block_num>`. Make sure the block number matches the `block_num` field of the transaction.
3. Find the blocks directory and state history directory (if applicable), e.g. `<blocks_dir>` and `<state_hist_dir>`, respectively.
4. Launch the [`eosio-blocklog`](../../../10_utilities/eosio-blocklog.md) utility as follows:

   `eosio-blocklog [--blocks-dir <blocks_dir>] [--state-history-dir <state_hist_dir>] --prune-transactions --block-num <block_num> --transaction <trx_id> [--transaction <trx_id2> ...]`

   If the operation is *successful*:
   * The [`eosio-blocklog`](../../../10_utilities/eosio-blocklog.md) utility terminates silently with a zero error code (no error).
   * The following fields are updated within the pruned transaction from the block logs:
      * The `prunable_data["prunable_data"][0]` field is set from 0 to 1.
      * The `signatures` field is set to an empty array.
      * The `context_free_data` field is set to an empty array.
      * The `packed_context_free_data` field, if any, is removed.

   If the operaton is *unsuccessful*:
   * The [`eosio-blocklog`](../../../10_utilities/eosio-blocklog.md) utility outputs an error to `stderr` and terminates with a non-zero error code (indicating an error).

## Notes

Some additional considerations are in order:

* You can pass multiple transactions to [`eosio-blocklog`](../../../10_utilities/eosio-blocklog.md) if they are within the same block.
* You can use [`eosio-blocklog`](../../../10_utilities/eosio-blocklog.md) to display the block that contains the pruned transactions.

## Example

In this example, we reproduce the steps listed in the [Procedure](#procedure) section above.

### Transaction sample (before)

Refer to the following transaction sample with context-free data:

```json
{
  "id": "1b9a9c53f9b692d3382bcc19c0c21eb22207e2f51a30fe88dabbb45376b6ff23",
  "trx": {
    "receipt": {
      "status": "executed",
      "cpu_usage_us": 155,
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
                  "SIG_K1_K3AJXEMFH99KScLFC1cnLA3WDnVK7WRsS8BtafHfP4VWmfQXXwX21KATVVtrCqopkcve6V8noc5bS4BJkwgSsonpfpWEJi"
                ],
                "packed_context_free_data": "0203a1b2c3031a2b3c"
              }
            ]
          },
          "packed_trx": "ec42545f7500ffe8aa290000000100305631191abda90000000000901d4d00000100305631191abda90000000000901d4d0100305631191abda900000000a8ed32320000"
        }
      ]
    },
    "trx": {
      "expiration": "2020-09-06T02:01:16",
      "ref_block_num": 117,
      "ref_block_prefix": 699066623,
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
        "SIG_K1_K3AJXEMFH99KScLFC1cnLA3WDnVK7WRsS8BtafHfP4VWmfQXXwX21KATVVtrCqopkcve6V8noc5bS4BJkwgSsonpfpWEJi"
      ],
      "context_free_data": [
        "a1b2c3",
        "1a2b3c"
      ]
    }
  },
  "block_time": "2020-09-06T02:00:47.000",
  "block_num": 119,
  "last_irreversible_block": 128,
  "traces": [
    {
      "action_ordinal": 1,
      "creator_action_ordinal": 0,
      "closest_unnotified_ancestor_action_ordinal": 0,
      "receipt": {
        "receiver": "payloadless",
        "act_digest": "4f09a630d4456585ee4ec5ef96c14151587367ad381f9da445b6b6239aae82cf",
        "global_sequence": 156,
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
      "elapsed": 206,
      "console": "Im a payloadless action",
      "trx_id": "1b9a9c53f9b692d3382bcc19c0c21eb22207e2f51a30fe88dabbb45376b6ff23",
      "block_num": 119,
      "block_time": "2020-09-06T02:00:47.000",
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
        "global_sequence": 157,
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
      "elapsed": 11,
      "console": "Im a payloadless action",
      "trx_id": "1b9a9c53f9b692d3382bcc19c0c21eb22207e2f51a30fe88dabbb45376b6ff23",
      "block_num": 119,
      "block_time": "2020-09-06T02:00:47.000",
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

### Steps

Using the above transaction to recreate the steps above:

1. Locate the transaction ID: `1b9a9c53f9b692d3382bcc19c0c21eb22207e2f51a30fe88dabbb45376b6ff23`.
2. Locate the block number: `119`.
3. Find the blocks directory and state history directory (if applicable), e.g. `<blocks_dir>` and `<state_hist_dir>`.
4. Launch the [`eosio-blocklog`](../../../10_utilities/eosio-blocklog.md) utility as follows:

   `eosio-blocklog --blocks-dir <blocks_dir> --state-history-dir <state_hist_dir> --prune-transactions --block-num 119 --transaction 1b9a9c53f9b692d3382bcc19c0c21eb22207e2f51a30fe88dabbb45376b6ff23`

If *successful*, the utility returns silently. If *unsuccessful*, it outputs an error to `stderr`.

### Transaction sample (after)

After retrieving the transaction a second time, the pruned transaction looks as follows:

```json
{
  "id": "1b9a9c53f9b692d3382bcc19c0c21eb22207e2f51a30fe88dabbb45376b6ff23",
  "trx": {
    "receipt": {
      "status": "executed",
      "cpu_usage_us": 155,
      "net_usage_words": 14,
      "trx": [
        1,
        {
          "compression": "none",
          "prunable_data": {
            "prunable_data": [
              1,
              {
                "digest": "6f29ea8ab323ffee90585238ff32300c4ee6aa563235ff05f3c1feb855f09189"
              }
            ]
          },
          "packed_trx": "ec42545f7500ffe8aa290000000100305631191abda90000000000901d4d00000100305631191abda90000000000901d4d0100305631191abda900000000a8ed32320000"
        }
      ]
    },
    "trx": {
      "expiration": "2020-09-06T02:01:16",
      "ref_block_num": 117,
      "ref_block_prefix": 699066623,
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
  "block_time": "2020-09-06T02:00:47.000",
  "block_num": 119,
  "last_irreversible_block": 131,
  "traces": [
    {
      "action_ordinal": 1,
      "creator_action_ordinal": 0,
      "closest_unnotified_ancestor_action_ordinal": 0,
      "receipt": {
        "receiver": "payloadless",
        "act_digest": "4f09a630d4456585ee4ec5ef96c14151587367ad381f9da445b6b6239aae82cf",
        "global_sequence": 156,
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
      "elapsed": 206,
      "console": "Im a payloadless action",
      "trx_id": "1b9a9c53f9b692d3382bcc19c0c21eb22207e2f51a30fe88dabbb45376b6ff23",
      "block_num": 119,
      "block_time": "2020-09-06T02:00:47.000",
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
        "global_sequence": 157,
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
      "elapsed": 11,
      "console": "Im a payloadless action",
      "trx_id": "1b9a9c53f9b692d3382bcc19c0c21eb22207e2f51a30fe88dabbb45376b6ff23",
      "block_num": 119,
      "block_time": "2020-09-06T02:00:47.000",
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

### Remarks

Notice the following modified fields within the pruned transaction:
   * The `prunable_data["prunable_data"][0]` field is 1.
   * The `signatures` field now contains an empty array.
   * The `context_free_data` field contains an empty array.
   * The `packed_context_free_data` field is removed.

## Next Steps

The following actions are available after you complete the procedure:

* Verify that the pruned transaction indeed contains pruned context-free data.
* Display the block that contains the pruned transaction from the block logs.
