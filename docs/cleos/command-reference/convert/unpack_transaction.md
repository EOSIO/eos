## Description

From packed to plain signed json form

## Positionals

- `transaction` _TEXT_ - The packed transaction json (string containing packed_trx and optionally compression fields.)

## Options

- `-h,--help` - Print this help message and exit
- `--unpack-action-data` - Unpack all action data within transaction, needs interaction with nodeos

## Usage

```text
cleos convert unpack_transaction '{
  "signatures": [
    "SIG_K1_KmRbWahefwxs6uyCGNR6wNRjw7cntEeFQhNCbyg8S92Kbp7zdSSVGTD2QS7pNVWgcU126zpxaBp9CwUxFpRwSnfkjd46bS"
  ],
  "compression": "none",
  "packed_context_free_data": "",
  "packed_trx": "8468635b7f379feeb95500000000010000000000ea305500409e9a2264b89a010000000000ea305500000000a8ed3232660000000000ea305500a6823403ea30550100000001000240cc0bf90a5656c8bb81f0eb86f49f89613c5cd988c018715d4646c6bd0ad3d8010000000100000001000240cc0bf90a5656c8bb81f0eb86f49f89613c5cd988c018715d4646c6bd0ad3d80100000000"
}'
```

## Output


```text
{
  "expiration": "2018-08-02T20:24:36",
  "ref_block_num": 14207,
  "ref_block_prefix": 1438248607,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "eosio",
      "name": "newaccount",
      "authorization": [{
          "actor": "eosio",
          "permission": "active"
        }
      ],
      "data": "0000000000ea305500a6823403ea30550100000001000240cc0bf90a5656c8bb81f0eb86f49f89613c5cd988c018715d4646c6bd0ad3d8010000000100000001000240cc0bf90a5656c8bb81f0eb86f49f89613c5cd988c018715d4646c6bd0ad3d801000000"
    }
  ],
  "transaction_extensions": [],
  "signatures": [
    "SIG_K1_KmRbWahefwxs6uyCGNR6wNRjw7cntEeFQhNCbyg8S92Kbp7zdSSVGTD2QS7pNVWgcU126zpxaBp9CwUxFpRwSnfkjd46bS"
  ],
  "context_free_data": []
}

```
