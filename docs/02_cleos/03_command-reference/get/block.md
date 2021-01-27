## Description
Retrieves a full block from the blockchain

## Positional Parameters
- `block` _TEXT_ - The number **or** ID of the block to retrieve
## Options
- `--header-state` - Get block header state from fork database instead
- `--info` - Get block info from the blockchain by block num only
## Example

### Get the full block:

```sh
cleos get block 1
```
or
```sh
cleos get block 0000000130d70e94e0022fd2fa035cabb9e542c34ea27f572ac90b5a7aa3d891
```

This will output a block object similar to the following

```json
{
  "timestamp": "2018-03-02T12:00:00.000",
  "producer": "",
  "confirmed": 1,
  "previous": "0000000000000000000000000000000000000000000000000000000000000000",
  "transaction_mroot": "0000000000000000000000000000000000000000000000000000000000000000",
  "action_mroot": "0000000000000000000000000000000000000000000000000000000000000000",
  "schedule_version": 0,
  "new_producers": null,
  "header_extensions": [],
  "producer_signature": "SIG_K1_111111111111111111111111111111111111111111111111111111111111111116uk5ne",
  "transactions": [],
  "block_extensions": [],
  "id": "0000000130d70e94e0022fd2fa035cabb9e542c34ea27f572ac90b5a7aa3d891",
  "block_num": 1,
  "ref_block_prefix": 3526296288
}
```

### Get the block info:

```sh
cleos get block --info 1
```

This will output a block info object similar to the following

```json
{
  "block_num": 1,
  "ref_block_num": 1,
  "id": "0000000130d70e94e0022fd2fa035cabb9e542c34ea27f572ac90b5a7aa3d891",
  "timestamp": "2018-03-02T12:00:00.000",
  "producer": "",
  "confirmed": 1,
  "previous": "0000000000000000000000000000000000000000000000000000000000000000",
  "transaction_mroot": "0000000000000000000000000000000000000000000000000000000000000000",
  "action_mroot": "0000000000000000000000000000000000000000000000000000000000000000",
  "schedule_version": 0,
  "producer_signature": "SIG_K1_111111111111111111111111111111111111111111111111111111111111111116uk5ne",
  "ref_block_prefix": 3526296288
}
```

## Remarks
The block info object has a fixed size and excludes the following fields: new_producers, header_extensions, transactions, block_extensions.
