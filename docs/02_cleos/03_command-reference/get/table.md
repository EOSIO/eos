## Description

Retrieves the contents of a database table

## Positional Parameters
`contract` _TEXT_ - The contract who owns the table

`scope` _TEXT_ - The scope within the contract in which the table is found

`table` _TEXT_ - The name of the table as specified by the contract abi

## Options
`-b,--binary` _UINT_ - Return the value as BINARY rather than using abi to interpret as JSON

`-l,--limit` _UINT_ - The maximum number of rows to return

`-k,--key` _TEXT_ - The name of the key to index by as defined by the abi, defaults to primary key

`-L,--lower` _TEXT_ - JSON representation of lower bound value of key, defaults to first

`-U,--upper` _TEXT_ - JSON representation of upper bound value value of key, defaults to last

## Example
Get the data from the accounts table for the eosio.token contract, for user eosio,

```sh
cleos get table eosio.token eosio accounts
```
```json
{
  "rows": [{
      "balance": "999999920.0000 SYS"
    }
  ],
  "more": false
}
```
