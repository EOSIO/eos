---
content_title: Storage and Read Modes
---

The EOSIO platform stores blockchain information in various data structures at various stages of a transaction's lifecycle. Some of these are described below. The producing node is the `nodeos` instance run by the block producer who is currently creating blocks for the blockchain (which changes every 6 seconds, producing 12 blocks in sequence before switching to another producer.)

## Blockchain State and Storage

Every `nodeos` instance creates some internal files to housekeep the blockchain state. These files reside in the `~/eosio/nodeos/data` installation directory and their purpose is described below:

* The `blocks.log` is an append only log of blocks written to disk and contains all the irreversible blocks. These blocks contain final, confirmed transactions.
* `reversible_blocks` is a memory mapped file and contains blocks that have been written to the blockchain but have not yet become irreversible. These blocks contain valid pushed transactions that still await confirmation to become final via the consensus protocol. The head block is the last block written to the blockchain, stored in `reversible_blocks`.
* The `chain state` or `chain database` is currently stored and cached in a memory mapped file. It contains the blockchain state associated with each block, including account details, deferred transactions, and data stored using multi index tables in smart contracts. The last 65,536 block IDs are also cached to support Transaction as Proof of Stake (TaPOS). The transaction ID/expiration is also cached until the transaction expires.

* The `pending block` is an in memory block containing transactions as they are processed and pushed into the block; this will/may eventually become the head block. If the `nodeos` instance is the producing node, the pending block is distributed to other `nodeos` instances.
* Outside the chain state, block data is cached in RAM until it becomes final/irreversible; especifically the signed block itself. After the last irreversible block (LIB) catches up to the block, that block is then retrieved from the irreversible blocks log.

## EOSIO Interfaces

EOSIO provides a set of [services](../../) and [interfaces](https://developers.eos.io/manuals/eosio.cdt/latest/files) that enable contract developers to persist state across action, and consequently transaction, boundaries. Contracts may use these services and interfaces for various purposes. For example, `eosio.token` contract keeps balances for all users in the chain database. Each instance of `nodeos` keeps the database in memory, so contracts can read and write data with ease.

### Nodeos RPC API

The `nodeos` service provides query access to the chain database via the HTTP [RPC API](../05_rpc_apis/index.md).

## Nodeos Read Modes

The `nodeos` service can be run in different "read" modes. These modes control how the node operates and how it processes blocks and transactions:

- `speculative`: this includes the side effects of confirmed and unconfirmed transactions.
- `head`: this only includes the side effects of confirmed transactions, this mode processes unconfirmed transactions but does not include them.
- `read-only`: this mode is deprecated. Similar functionality can be achieved by combining options: `read-mode = head`, `p2p-accept-transactions = false`, `api-accept-transactions = false`. When these options are set, the local database will contain state changes made by transactions in the chain up to the head block. Also, transactions received via the P2P network are not relayed and transactions cannot be pushed via the chain API.
- `irreversible`: this mode also includes confirmed transactions only up to those included in the last irreversible block.

A transaction is considered confirmed when a `nodeos` instance has received, processed, and written it to a block on the blockchain, i.e. it is in the head block or an earlier block.

### Speculative Mode

Clients such as `cleos` and the RPC API, will see database state as of the current head block plus changes made by all transactions known to this node but potentially not included in the chain, unconfirmed transactions for example.

Speculative mode is low latency but fragile, there is no guarantee that the transactions reflected in the state will be included in the chain OR that they will reflected in the same order the state implies.  

This mode features the lowest latency, but is the least consistent. 

In speculative mode `nodeos` is able to execute transactions which have TaPoS (Transaction as Proof of Stake) pointing to any valid block in a fork considered to be the best fork by this node.

### Head Mode

Clients such as `cleos` and the RPC API will see database state as of the current head block of the chain.  Since current head block is not yet irreversible and short-lived forks are possible, state read in this mode may become inaccurate  if `nodeos` switches to a better fork.  Note that this is also true of speculative mode.  

This mode represents a good trade-off between highly consistent views of the data and latency.

In this mode `nodeos` is able to execute transactions which have TaPoS pointing to any valid block in a fork considered to be the best fork by this node.

### Read-Only Mode

[[caution | Deprecation Notice]]
| The explicit `read-mode = read-only` mode is deprecated. Similar functionality can now be achieved in `head` mode by combining options: `read-mode = head`, `p2p-accept-transactions = false`, `api-accept-transactions = false`.

Clients such as `cleos` and the RPC API will see database state as of the current head block of the chain. It **will not** include changes made by transactions known to this node but not included in the chain, such as unconfirmed transactions.

### Irreversible Mode

When `nodeos` is configured to be in irreversible read mode, it will still track the most up-to-date blocks in the fork database, but the state will lag behind the current best head block, sometimes referred to as the fork DB head, to always reflect the state of the last irreversible block. 

Clients such as `cleos` and the RPC API will see database state as of the current head block of the chain. It **will not** include changes made by transactions known to this node but not included in the chain, such as unconfirmed transactions.

## How To Specify the Read Mode

The mode in which `nodeos` is run can be specified using the `--read-mode` option from the `eosio::chain_plugin`.
