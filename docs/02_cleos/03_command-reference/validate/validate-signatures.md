## Description
Validate signatures and recover public keys

## Usage
```sh
cleos validate signatures [OPTIONS] transaction
```

## Positional Arguments
- `transaction` _TEXT_ - The JSON string or filename defining the signed transaction to validate

## Options
- `-c,--chain-id` _TEXT_ - The chain id that will be used in signature verification

## Example
<!--
{
  "expiration": "2020-04-23T04:47:23",
  "ref_block_num": 20,
  "ref_block_prefix": 3872940040,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [
    {
      "account": "eosio",
      "name": "voteproducer",
      "authorization": [
        {
          "actor": "initb",
          "permission": "active"
        }
      ],
      "data": "000000008093dd74000000000000000001000000008093dd74"
    }
  ],
  "transaction_extensions": [],
  "signatures": [
    "SIG_K1_Jy81u5yWSE4vGET1cm9TChKrzhAz4QE2hB2pWnUsHQExGafqhVwXtg7a7mbLZwXcon8bVQJ3J5jtZuecJQADTiz2kwcm7c"
  ],
  "context_free_data": []
}
-->
```sh
cleos validate signatures transaction '{ "expiration": "2020-04-23T04:47:23", "ref_block_num": 20, "ref_block_prefix": 3872940040, 
"max_net_usage_words": 0, "max_cpu_usage_ms": 0, "delay_sec": 0, "context_free_actions": [], "actions": [ { "account": "eosio", "name": "voteproducer", "authorization": [ { "actor": "initb", "permission": "active" } ], "data": "000000008093dd74000000000000000001000000008093dd74" } ], "transaction_extensions": [], "signatures": [ "SIG_K1_Jy81u5yWSE4vGET1cm9TChKrzhAz4QE2hB2pWnUsHQExGafqhVwXtg7a7mbLZwXcon8bVQJ3J5jtZuecJQADTiz2kwcm7c" ], "context_free_data": [] }'
```

## See Also
- [How to submit a transaction](../../02_how-to-guides/how-to-submit-a-transaction.md)

## Output
```console
[
  "EOS7pCywBCz5zw2bc7teCVcT7MEWUr9s749qnYDNPEsBoH32vGqqN"
]
```
