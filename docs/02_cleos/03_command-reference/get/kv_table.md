## Description

Retrieves the contents of a database kv_table

## Positional Parameters
`account` _TEXT_ - The account who owns the kv_table where the smart contract was deployed

`table` _TEXT_ - The name of the kv_table as specified by the contract abi

`index_name` _TEXT_ - The name of the kv_table index as specified by the contract abi

## Options
`-l,--limit` _UINT_ - The maximum number of rows to return

`-i,--index` _TEXT_ - index value used for point query; encoded as `--encode-type`

`-L,--lower` _TEXT_ - JSON representation of lower bound index value of `--key` for ranged query (defaults to first). Query result includes rows specified with `--lower`

`-U,--upper` _TEXT_ - JSON representation of upper bound index value of `--key` for ranged query (defaults to last). Query result does NOT include rows specified with `--upper`

`--encode-type` _TEXT_ - The encoding type of `--index`, `--lower`, `--upper`; `bytes` for hexadecimal encoded bytes; `string` for string values; `dec` for decimal encoding of (`uint[64|32|16|8]`, `int[64|32|16|8]`, `float64`); `hex` for hexadecimal encoding of (`uint[64|32|16|8]`, `int[64|32|16|8]`, `sha256`, `ripemd160`

`-b,--binary` _UINT_ - Return the value as BINARY rather than using abi to interpret as JSON

`-r,--reverse` - Iterate in reverse order; results are returned in reverse order

`--show-payer` - Show RAM payer

## Remarks

  * When `--reverse` option is not set, `--upper` is optional; if `--upper` is not set, the result includes the end of the matching rows.
  * When `--reverse` option is set, `--lower` is optional; if `--lower` is not set, the result includes the start of the matching rows.
  * When the result returns `"more": true`, the remaining rows can be retrieved by setting `--encode_bytes` to `bytes` and `--index`, `--lower` or `--upper` (as applicable, depending on the `--reverse` option) to the value returned in `"next_key": "XYZ"`, where `XYZ` is the next index value in hex.
  * When `--index` is used as non-unique secondary index, the result can return multiple rows.

## Examples

Point query to return the row that matches eosio name key `boba` from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -i boba contr_acct kvtable primarykey -b
```
```json
{
    "rows": [
        "000000000000600e3d010000000000000004626f6261000000"
    ],
    "more": false,
    "next_key": ""
}
```

Point query to return the row that matches decimal key `1` from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo` (note that multiple rows could have resulted since `foo` is a non-unique secondary index):
```sh
cleos get kv_table --encode-type dec -i 1 contr_acct kvtable foo
```
```json
{
    "rows": [
        "000000000000600e3d010000000000000004626f6261"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows starting from eosio name key `bobd` up to `bobh` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -L bobd -U bobh contr_acct kvtable primarykey -b
```
```json
{
    "rows": [
        "000000000000900e3d040000000000000004626f6264000000",
        "000000000000a00e3d050000000000000004626f6265000000",
        "000000000000b00e3d060000000000000004626f6266000000",
        "000000000000c00e3d070000000000000004626f6267000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (in reverse order) starting from eosio name key `bobh` down to `bobd` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -L bobd -U bobh contr_acct kvtable primarykey -b -r
```
```json
{
    "rows": [
        "000000000000d00e3d080000000000000004626f6268000000",
        "000000000000c00e3d070000000000000004626f6267000000",
        "000000000000b00e3d060000000000000004626f6266000000",
        "000000000000a00e3d050000000000000004626f6265000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows starting from eosio name key `bobg` up to the last row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -L bobg contr_acct kvtable primarykey -b
```
```json
{
    "rows": [
        "000000000000c00e3d070000000000000004626f6267000000",
        "000000000000d00e3d080000000000000004626f6268000000",
        "000000000000e00e3d090000000000000004626f6269000000",
        "000000000000f00e3d0a0000000000000004626f626a000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (in reverse order) starting from the last row key down to eosio name key `bobg` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -L bobg contr_acct kvtable primarykey -b -r
```
```json
{
    "rows": [
        "000000000000f00e3d0a0000000000000004626f626a000000",
        "000000000000e00e3d090000000000000004626f6269000000",
        "000000000000d00e3d080000000000000004626f6268000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows starting from the first row key up to eosio name key `bobe` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -U bobe contr_acct kvtable primarykey -b
```
```json
{
    "rows": [
        "000000000000600e3d010000000000000004626f6261000000",
        "000000000000700e3d020000000000000004626f6262000000",
        "000000000000800e3d030000000000000004626f6263000000",
        "000000000000900e3d040000000000000004626f6264000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (in reverse order) starting from eosio name key `bobe` down to the first row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -U bobe contr_acct kvtable primarykey -b -r
```
```json
{
    "rows": [
        "000000000000a00e3d050000000000000004626f6265000000",
        "000000000000900e3d040000000000000004626f6264000000",
        "000000000000800e3d030000000000000004626f6263000000",
        "000000000000700e3d020000000000000004626f6262000000",
        "000000000000600e3d010000000000000004626f6261000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (in reverse order, limit results to 2 rows) starting from eosio name key `bobe` down to the first row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type name -U bobe contr_acct kvtable primarykey -b -r -l 2
```
```json
{
    "rows": [
        "000000000000a00e3d050000000000000004626f6265000000",
        "000000000000900e3d040000000000000004626f6264000000"
    ],
    "more": true,
    "next_key": "3D0E800000000000"
}
```

Continue previous range query to return all rows (in reverse order, limit results to 2 rows) starting from hex key `3D0E800000000000` (returned in `next_key` field from previous result) down to the first row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type bytes -U 3D0E800000000000 contr_acct kvtable primarykey -b -r -l 2
```
```json
{
    "rows": [
        "000000000000800e3d030000000000000004626f6263000000",
        "000000000000700e3d020000000000000004626f6262000000"
    ],
    "more": true,
    "next_key": "3D0E600000000000"
}
```

Continue previous range query to return all rows (in reverse order, limit results to 2 rows) starting from hex key `3D0E600000000000` (returned in `next_key` field from previous result) down to the first row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `primarykey`:
```sh
cleos get kv_table --encode-type bytes -U 3D0E600000000000 contr_acct kvtable primarykey -b -r -l 2
```
```json
{
    "rows": [
        "000000000000600e3d010000000000000004626f6261000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows starting from decimal key `0` up to `3` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo` (note that key `0` does not exist, so it starts from key `1`):
```sh
cleos get kv_table --encode-type dec -L 0 -U 3 contr_acct kvtable foo -b
```
```json
{
    "rows": [
        "000000000000600e3d010000000000000004626f6261",
        "000000000000700e3d020000000000000004626f6262"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows starting from decimal key `6` up to the last row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`:
```sh
cleos get kv_table --encode-type dec -L 6 contr_acct kvtable foo -b
```
```json
{
    "rows": [
        "000000000000b00e3d060000000000000004626f6266",
        "000000000000c00e3d070000000000000004626f6267",
        "000000000000d00e3d080000000000000004626f6268",
        "000000000000e00e3d090000000000000004626f6269",
        "000000000000f00e3d0a0000000000000004626f626a"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (limit results to 2 rows) starting from decimal key `6` up to the last row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`:
```sh
cleos get kv_table --encode-type dec -L 6 contr_acct kvtable foo -b -l 2
```
```json
{
    "rows": [
        "000000000000b00e3d060000000000000004626f6266",
        "000000000000c00e3d070000000000000004626f6267"
    ],
    "more": true,
    "next_key": "0000000000000008"
}
```

Continue previous range query to return all rows (limit results to 2 rows) starting from hex key `0000000000000008` (returned in `next_key` field from previous result, which is also hex for decimal key `8`) up to the last row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`:
```sh
cleos get kv_table --encode-type bytes -L 0000000000000008 contr_acct kvtable foo -b -l 2
```
```json
{
    "rows": [
        "000000000000d00e3d080000000000000004626f6268",
        "000000000000e00e3d090000000000000004626f6269"
    ],
    "more": true,
    "next_key": "000000000000000A"
}
```

Continue previous range query to return all rows (limit results to 2 rows) starting from hex key `000000000000000A` (returned in `next_key` field from previous result, which is also hex for decimal key `10`) up to the last row key from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`:
```sh
cleos get kv_table --encode-type bytes -L 000000000000000A contr_acct kvtable foo -b -l 2
```
```json
{
    "rows": [
        "000000000000f00e3d0a0000000000000004626f626a"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (in reverse order) starting from hex key `4` down to `2` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo` (note that hex keys are not correctly specified, or decimal type should be used instead):
```sh
cleos get kv_table --encode-type bytes -L 2 -U 4 contr_acct kvtable foo -b -r
```
```console
Error 3060003: Contract Table Query Exception
Most likely, the given table doesn't exist in the blockchain.
Error Details:
Invalid index type/encode_type/Index_value: uint64/bytes/{v}
```

Range query to return all rows (in reverse order) starting from decimal key `4` down to `2` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`:
```sh
cleos get kv_table --encode-type dec -L 2 -U 4 contr_acct kvtable foo -b -r
```
```json
{
    "rows": [
        "000000000000900e3d040000000000000004626f6264",
        "000000000000800e3d030000000000000004626f6263"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (in reverse order) starting from hex key `0000000000000004` down to `0000000000000002` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo`
```sh
cleos get kv_table --encode-type bytes -L 0000000000000002 -U 0000000000000004 contr_acct kvtable foo -b -r
```
```json
{
    "rows": [
        "000000000000900e3d040000000000000004626f6264",
        "000000000000800e3d030000000000000004626f6263"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows starting from string key `boba` up to `bobe` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `foo` (note that `--lower` and `--upper` values have correct `string` type, but the incorrect index `foo` was used):
```sh
cleos get kv_table --encode-type string -L boba -U bobe contr_acct kvtable foo -b
```
```json
{
    "rows": [],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows starting from string key `boba` up to `bobe` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `bar`:
```sh
cleos get kv_table --encode-type string -L boba -U bobe contr_acct kvtable bar -b
```
```json
{
    "rows": [
        "0186f263c540000000addd235fd05780003d0e60000000",
        "0186f263c540000000addd235fd05780003d0e70000000",
        "0186f263c540000000addd235fd05780003d0e80000000",
        "0186f263c540000000addd235fd05780003d0e90000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (in reverse order) starting from string key `bobe` down to `boba` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `bar`:
```sh
cleos get kv_table --encode-type string -L boba -U bobe contr_acct kvtable bar -b -r
```
```json
{
    "rows": [
        "0186f263c540000000addd235fd05780003d0ea0000000",
        "0186f263c540000000addd235fd05780003d0e90000000",
        "0186f263c540000000addd235fd05780003d0e80000000",
        "0186f263c540000000addd235fd05780003d0e70000000"
    ],
    "more": false,
    "next_key": ""
}
```

Range query to return all rows (in reverse order, limit results to 2 rows) starting from string key `bobe` down to `boba` (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `bar`:
```sh
cleos get kv_table --encode-type string -L boba -U bobe contr_acct kvtable bar -b -r -l 2
```
```json
{
    "rows": [
        "0186f263c540000000addd235fd05780003d0ea0000000",
        "0186f263c540000000addd235fd05780003d0e90000000"
    ],
    "more": true,
    "next_key": "626F62630000"
}
```

Continue previous range query to return all rows (in reverse order, limit results to 2 rows) starting from hex key `626F62630000` (returned in `next_key` field from previous result, which is also hex for string `bobc`) down to hex key `626F62610000` (hex for string `boba`) (exclusive) from kv_table named `kvtable` owned by `contr_acct` account using kv_table index `bar`:
```sh
cleos get kv_table --encode-type bytes -L 626F62610000 -U 626F62630000 contr_acct kvtable bar -b -r -l 2
```
```json
{
    "rows": [
        "0186f263c540000000addd235fd05780003d0e80000000",
        "0186f263c540000000addd235fd05780003d0e70000000"
    ],
    "more": false,
    "next_key": ""
}
```
