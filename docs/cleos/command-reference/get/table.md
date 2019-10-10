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

`-U,--upper` _TEXT_ - JSON representation of upper bound value  of key, defaults to last

`--index` _TEXT_ - Index number, 1 - primary (first), 2 - secondary index (in order defined by multi_index), 3 - third index, etc. Number or name of index can be specified, e.g. 'secondary' or '2'.

`--key-type` _TEXT_ - The key type of --index, primary only supports (i64), all others support (i64, i128, i256, float64, float128, ripemd160, sha256). Special type 'name' indicates an account name.

`--encode-type` _TEXT_ - The encoding type of key_type (i64 , i128 , float64, float128) only support decimal encoding e.g. 'dec'i256 - supports both 'dec' and 'hex', ripemd160 and sha256 is 'hex' only

`-r,--reverse` - Iterate in reverse order

`--show-payer` - show RAM payer

## Example

Gets the data from the accounts table for the eosio.token contract, for user eosio,

```shell
$ cleos get table eosio.token eosio accounts
{
  "rows": [{
      "balance": "999999920.0000 SYS"
    }
  ],
  "more": false
}

```
