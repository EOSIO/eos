## Goal

Query infomation about a block.

## Before you begin

Make sure to meet the following requirements:

* Familiarize with the [`cleos get block`](../03_command-reference/get/block.md) command and its parameters.
* Install the currently supported version of `cleos`.

[[info | Note]]
| `cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install `cleos`.

* Understand what a [block](https://developers.eos.io/welcome/latest/glossary/index/#block) is and its role in the blockchain.
* Understand the [block lifecycle](https://developers.eos.io/welcome/latest/protocol-guides/consensus_protocol/#5-block-lifecycle) in the EOSIO consensus protocol.

## Steps

Perform the step below:

Retrieve full or partial information about a block:

```sh
cleos get block [--info] <block_number_or_id>
```

Where `block_number_or_id` is the specified block number or block ID and `--info` is an optional parameter to retrieve a partial subset of the block information.

Some examples are provided below:

* Query the testnet to retrieve full block information about block number `48351112` or block ID `02e1c7888a92206573ae38d00e09366c7ba7bc54cd8b7996506f7d2a619c43ba`:

**Example Output**

```sh
cleos -u https://api.testnet.eos.io get block 48351112
```
```json
{
  "timestamp": "2021-01-28T17:58:59.500",
  "producer": "inith",
  "confirmed": 0,
  "previous": "02e1c78787ff4d4ce6124831b936bb4ef6015e470868a535f1c6e04f3afed8a1",
  "transaction_mroot": "0000000000000000000000000000000000000000000000000000000000000000",
  "action_mroot": "1bf9d17b5a951cbb6d0a8324e4039744db4137df498abd53046ea26fa74d73c9",
  "schedule_version": 1,
  "new_producers": null,
  "producer_signature": "SIG_K1_JxFfxGA1wZx9LCVjbrBb5nxTuJai7RUSiwRXyY866fYvZZyRtdmQFn9KJCqVHFAiYEsJpDb6dhTmHNDwipJm4rDiyhEmGa",
  "transactions": [],
  "id": "02e1c7888a92206573ae38d00e09366c7ba7bc54cd8b7996506f7d2a619c43ba",
  "block_num": 48351112,
  "ref_block_prefix": 3493375603
}
```

* Query the testnet to retrieve full block information about block ID `02e1c7888a92206573ae38d00e09366c7ba7bc54cd8b7996506f7d2a619c43ba`:

**Example Output**

```sh
cleos -u https://api.testnet.eos.io get block 02e1c7888a92206573ae38d00e09366c7ba7bc54cd8b7996506f7d2a619c43ba
```
```json
{
  "timestamp": "2021-01-28T17:58:59.500",
  "producer": "inith",
  "confirmed": 0,
  "previous": "02e1c78787ff4d4ce6124831b936bb4ef6015e470868a535f1c6e04f3afed8a1",
  "transaction_mroot": "0000000000000000000000000000000000000000000000000000000000000000",
  "action_mroot": "1bf9d17b5a951cbb6d0a8324e4039744db4137df498abd53046ea26fa74d73c9",
  "schedule_version": 1,
  "new_producers": null,
  "producer_signature": "SIG_K1_JxFfxGA1wZx9LCVjbrBb5nxTuJai7RUSiwRXyY866fYvZZyRtdmQFn9KJCqVHFAiYEsJpDb6dhTmHNDwipJm4rDiyhEmGa",
  "transactions": [],
  "id": "02e1c7888a92206573ae38d00e09366c7ba7bc54cd8b7996506f7d2a619c43ba",
  "block_num": 48351112,
  "ref_block_prefix": 3493375603
}
```

* Query the local chain to retrieve partial block information about block number `1`:

**Example Output**

```sh
cleos get block --info 1
```
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

Note that the partial block information excludes the variable fields `new_producers`, `header_extensions`, `transactions`, or `block_extensions`.
