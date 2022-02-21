## Description

Retrieves the contents of a database table

## Positional Parameters
`account` _TEXT_ - The account who owns the table where the smart contract was deployed

`scope` _TEXT_ - The scope within the contract in which the table is found

`table` _TEXT_ - The name of the table as specified by the contract abi

## Options
`-l,--limit` _UINT_ - The maximum number of rows to return

`-k,--key` _TEXT_ - (Deprecated) The name of the key to index by as defined by the abi, defaults to primary key

`-L,--lower` _TEXT_ - JSON representation of lower bound value of key, defaults to first

`-U,--upper` _TEXT_ - JSON representation of upper bound value value of key, defaults to last

`--index` _TEXT_ - Index number, `1`: primary (first), `2`: secondary index (in order defined by multi_index), `3`: third index, etc. Number or name of index can be specified, e.g. `secondary` or `2`

`--key-type` _TEXT_ - The key type of `--index`; primary only supports `i64`. All others support `i64`, `i128`, `i256`, `float64`, `float128`, `ripemd160`, `sha256`. Special type `name` indicates an account name

`--encode-type` _TEXT_ - The encoding type of `--key_type`; `dec` for decimal encoding of (`i[64|128|256]`, `float[64|128]`); `hex` for hexadecimal encoding of (`i256`, `ripemd160`, `sha256`)

`-b,--binary` _UINT_ - Return the value as BINARY rather than using abi to interpret as JSON

`-r,--reverse` - Iterate in reverse order

`--show-payer` - Show RAM payer

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
