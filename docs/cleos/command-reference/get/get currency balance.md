---
title: "get currency balance"
excerpt: "Retrieve the balance of an account for a given currency"
---
## Positional Parameters
`contract` _TEXT_ - The contract that operates the currency
`account` _TEXT_ - The account to query balances for
`symbol` _TEXT_ - The symbol for the currency if the contract operates multiple currencies
## Options
There are no options for this subcommand
## Example
Get balance of eosio from eosio.token contract for SYS symbol. 

```text
$ cleos get currency balance eosio.token eosio SYS
999999920.0000 SYS
```
