## Description

Retrieves the contents of a database kv_table

## Positional Parameters
`account` _TEXT_ - The account who owns the kv_table where the smart contract was deployed

`table` _TEXT_ - The name of the kv_table as specified by the contract abi

`index_name` _TEXT_ - The name of the kv_table index as specified by the contract abi

## Options
`-l,--limit` _UINT_ - The maximum number of rows to return

`-k,--key` _TEXT_ - index value used for point query encoded as `--encode-type`

`-L,--lower` _TEXT_ - JSON representation of lower bound index value of `--key` for ranged query (defaults to first). Query result includes rows specified with `--lower`

`-U,--upper` _TEXT_ - JSON representation of upper bound index value of `--key` for ranged query (defaults to last). Query result does NOT include rows specified with `--upper`

`--encode-type` _TEXT_ - The encoding type of `--key`, `--lower`, `--upper`; `bytes` for hexadecimal encoded bytes; `dec` for decimal encoding of (`uint[64|32|16|8]`, `int[64|32|16|8]`, `float64`); `hex` for hexadecimal encoding of (`uint[64|32|16|8]`, `int[64|32|16|8]`, `sha256`, `ripemd160`

`-b,--binary` _UINT_ - Return the value as BINARY rather than using abi to interpret as JSON

`-r,--reverse` - Iterate in reverse order; results are returned in reverse order

`--show-payer` - Show RAM payer

## Examples

Range query to return all rows that match eosio name keys between `boba` (inclusive) and `bobj` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -L boba -U bobj contr_acct kvtable primarykey
```

Point query to return the row that matches eosio name key `boba` from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -k boba contr_acct kvtable primarykey
```

Point query to return the row that matches hex key `1` from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`:
```sh
cleos get kv_table --encode-type hex -k 1 contr_acct kvtable foo
```

Point query to return the row that matches decimal key `1` from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`:
```sh
cleos get kv_table --encode-type dec -k 1 contr_acct kvtable foo
```

Range query to return all rows that match decimal keys between `2` (inclusive) and `5` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`:
```sh
cleos get kv_table --encode-type dec -L 2 -U 5 contr_acct kvtable foo
```

Range query to return all rows that match decimal keys between `2` (inclusive) and `5` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`; return the results in reverse order and in binary (not JSON):
```sh
cleos get kv_table -r -b --encode-type dec -L 2 -U 5 contr_acct kvtable foo
```
