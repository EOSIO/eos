## Description
Validate signatures and recover public keys

## Positional Parameters
- `transaction` _TEXT_ - The JSON string or filename defining the transaction to validate

## Options
- `-c,--chain-id` - The chain id that will be used in signature verification

## Example
```sh
cleos validate signatures '{ \
  "expiration": "2018-08-02T20:24:36", \
  "ref_block_num": 14207, \
  "ref_block_prefix": 1438248607, \
  "max_net_usage_words": 0, \
  "max_cpu_usage_ms": 0, \
  "delay_sec": 0, \
  "context_free_actions": [], \
  "actions": [{ \
      "account": "eosio", \
      "name": "newaccount", \
      "authorization": [{ \
          "actor": "eosio", \
          "permission": "active" \
        } \
      ], \
      "data": \
      "0000000000ea305500a6823403ea30550100000001000240cc0bf90a5656c8bb81f0eb86f49f89613c5cd988c018715d4646c6bd0ad3d8010000000100000001000240cc0bf90a5656c8bb81f0eb86f49f89613c5cd988c018715d4646c6bd0ad3d801000000" \
    } \
  ], \
  "transaction_extensions": [] \
}'
```
