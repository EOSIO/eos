
## Overview

The `blockvault_client_plugin` enables blockchain administrators to implement industry standard disaster recovery to maximize producer operational uptime. The plugin allows a block producer to cluster two or more nodes deployed as a single logical producer. If one of the nodes goes down, the other nodes in the cluster continue to operate, thereby meeting certain guarantees for the producer to continue to function with minimal service disruption.

## Goals

Block Vault is a clustered component within an EOSIO network architecture that enables replicated durable storage with strong consistency guarantees for the input required by a redundant cluster of nodes. In particular, Block Vault achieves the following guarantees for any cluster node running `nodeos` configured as a Block Vault client in producing mode:

* Guarantee against double-production of blocks
* Guarantee against finality violation
* Guarantee of liveness (ability to make progress as a blockchain)

To facilitate these guarantees, Block Vault allows `nodeos` to run in a redundant and/or highly available mode. Block Vault itself does not implement any coordination of nodes in a cluster. It merely guarantees that any such coordination, including faulty coordination leading to multiple active block constructing nodes, will be safe as defined by the above guarantees. For more information, read the [Block Vault Operation](#block-vault-operation) section below.

## Usage

```console
# config.ini
plugin = eosio::blockvault_client_plugin
[options]
```
```sh
# command-line
nodeos ... --plugin eosio::blockvault_client_plugin [options]
```

## Configuration Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::blockvault_client_plugin:

  --block-vault-backend arg             the uri for block vault backend. 
                                        Currently, only PostgreSQL is 
                                        supported, the format is 
                                        'postgresql://username:password@localho
                                        st/company'
```

## Plugin Dependencies

* [`producer_plugin`](../producer_plugin/index.md)

## Configuration Example

To use `blockvault_client_plugin`, the `nodeos` service must be configured as a producer with the `--block-vault-backend` option:

```sh
nodeos --plugin eosio::producer_plugin --producer-name myproducer --plugin eosio::blockvault_client_plugin --block-vault-backend postgresql://user:password@mycompany.com
```

For production deployments, it is recommend to use the `PGPASSWORD` environment variable to configure the password, instead of embedding the password in the URI.

```sh
export PGPASSWORD=password
nodeos --plugin eosio::producer_plugin --producer-name myproducer --plugin eosio::blockvault_client_plugin --block-vault-backend postgresql://user@mycompany.com
```

## Software Dependencies

To build `blockvault_client_plugin` you need `libpq` version 10 or above and `libpqxx` version 6 or above. These dependencies are typically installed (alongside other dependencies) when you either [Install EOSIO](../../../00_install/index.md) from prebuilt binaries or build from source. You may also opt to install these dependencies manually prior to installing or building EOSIO.

For MacOS, you can simply use homebrew to install these dependencies:

```sh
brew install libpq libpqxx
```

For Linux, the versions of `libpq` and `libpqxx` provided by the system package managers may not be current enough. We recommend to follow the `install libpq` and `install libpqxx` sections of the corresponding dockerfile in `.cicd/platform/pinned` for your platform to install these dependencies.

## Block Vault Storage

Currently, Block Vault uses `PostgreSQL` for its durable storage. Other distributed databases may be supported in the future.

### Running PostgreSQL for Testing

We recommend to use `docker`:

```sh
docker run  -p 5432:5432 -e POSTGRES_PASSWORD=password -d postgres
```

### Running PostgreSQL for Production 

We recommend to deploy `PostgreSQL` with HA (high availability) mode and synchronous replication strategy.

### Database Schema

`blockvault_client_plugin` creates two tables `BlockData` and `SnapshotData` if not already in the database. The tables are created with the following SQL commands when the plugin starts:

```sh
CREATE TABLE IF NOT EXISTS BlockData (watermark_bn bigint, watermark_ts bigint, lib bigint, block_num bigint, block_id bytea UNIQUE, previous_block_id bytea, block oid, block_size bigint);

CREATE TABLE IF NOT EXISTS SnapshotData (watermark_bn bigint, watermark_ts bigint, snapshot oid);
```

## Block Vault Operation

As new nodes join a cluster, the Block Vault will be their exclusive source of sync data, enabling it to guarantee a consistent view of the blockchain as a base. When a node participating in the cluster constructs a new block, it will submit it to the Block Vault for approval prior to broadcasting it to external peers via the P2P network.

Block Vault is exclusively responsible for providing guarantees against double-production of blocks and finality violations. It facilitates partial guarantee of liveness through node redundancy by ensuring that data needed to construct new blocks are durable and replicated. However, Block Vault cannot guarantee that the data it contains was not malformed. To that end, Block Vault depends on the proper operation of the clustered nodes.

## Block Vault Client API

Cluster nodes interact with the Block Vault through the following messages:

* [`async_propose_constructed_block()`](../../../classeosio_1_1blockvault_1_1block__vault__interface#function-async_propose_constructed_block)
* [`async_append_external_block()`](../../../classeosio_1_1blockvault_1_1block__vault__interface#function-async_append_external_block)
* [`propose_snapshot()`](../../../classeosio_1_1blockvault_1_1block__vault__interface#function-propose_snapshot)
* [`sync()`](../../../classeosio_1_1blockvault_1_1block__vault__interface#function-sync)

For more information visit the [block_vault_interface](../../../classeosio_1_1blockvault_1_1block__vault__interface) C++ reference.
