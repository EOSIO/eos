## Description
From json action data to packed form

## Positionals
- `account` _TEXT_ - The name of the account that hosts the contract
- `name` _TEXT_ - The name of the function that's called by this action
- `unpacked_action_data` _TEXT_ - The action data expressed as json
## Options
none
## Usage


```text
 cleos convert pack_action_data eosio unlinkauth '{"account":"test1", "code":"test2", "type":"eosioeosio"}'
```

## Output


```text
000000008090b1ca000000000091b1ca000075982aea3055
```
