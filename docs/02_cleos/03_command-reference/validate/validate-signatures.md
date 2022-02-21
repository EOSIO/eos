## Description
Validate signatures and recover public keys

[[info | JSON input]]
| This command involves specifying JSON input which depends on underlying class definitions. Therefore, such JSON input is subject to change in future versions of the EOSIO software.

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
cleos validate signatures --chain-id cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f '{ "expiration": "2020-04-23T04:47:23", "ref_block_num": 20, "ref_block_prefix": 3872940040, 
"max_net_usage_words": 0, "max_cpu_usage_ms": 0, "delay_sec": 0, "context_free_actions": [], "actions": [ { "account": "eosio", "name": "voteproducer", "authorization": [ { "actor": "initb", "permission": "active" } ], "data": "000000008093dd74000000000000000001000000008093dd74" } ], "transaction_extensions": [], "signatures": [ "SIG_K1_Jy81u5yWSE4vGET1cm9TChKrzhAz4QE2hB2pWnUsHQExGafqhVwXtg7a7mbLZwXcon8bVQJ3J5jtZuecJQADTiz2kwcm7c" ], "context_free_data": [] }'
```
or
```sh
cleos -u https://api.testnet.eos.io validate signatures '{ "expiration": "2020-04-23T04:47:23", "ref_block_num": 20, "ref_block_prefix": 3872940040, 
"max_net_usage_words": 0, "max_cpu_usage_ms": 0, "delay_sec": 0, "context_free_actions": [], "actions": [ { "account": "eosio", "name": "voteproducer", "authorization": [ { "actor": "initb", "permission": "active" } ], "data": "000000008093dd74000000000000000001000000008093dd74" } ], "transaction_extensions": [], "signatures": [ "SIG_K1_Jy81u5yWSE4vGET1cm9TChKrzhAz4QE2hB2pWnUsHQExGafqhVwXtg7a7mbLZwXcon8bVQJ3J5jtZuecJQADTiz2kwcm7c" ], "context_free_data": [] }'
```

## Output
```console
[
  "EOS7pCywBCz5zw2bc7teCVcT7MEWUr9s749qnYDNPEsBoH32vGqqN"
]
```

## See Also
- [How to submit a transaction](../../02_how-to-guides/how-to-submit-a-transaction.md)
