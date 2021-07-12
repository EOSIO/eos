# Overview

Block Vault is a component in an EOSIO network architecture which provides a replicated durable storage with strong consistency guarantees for all the input required by a redundant cluster of nodeos nodes to achieve the guarantees outlined in BlockVault: Failover for Nodeos  reproduced here:

* Guarantee against double-production of blocks
* Guarantee against finality violation
* Guarantee of liveness (ability to make progress as a blockchain)

By facilitating these guarantees, Block Vault allows nodeos to run in a redundant and/or highly available mode without an additional requirement of coordination between the nodes. Block Vault itself does not implement any coordination of nodeos nodes in a cluster. Thus, it merely guarantees that any such coordination, including faulty coordination leading to multiple active block constructing nodeos nodes will be safe as defined by the above guarantees.

Currently we use PostgreSQL as our durable storage for block vault, other distributed database may be supported in the future. 

## Compiling the blockvault_client_plugin

blockvault_client_plugin requires libpq to compile.

For Mac, you can simply use homebrew to install the required dependencies. 
```
brew install postgresql
```

For Linux, Ubuntu derived distributions will require installation of `libpq-dev` and `postgresql-server-dev-all` packages. Other distributions like CentOS will require only `postgresql-devel`.


## Running PostgreSQL for testing purpose

We recommend to use docker 

```
docker run  -p 5432:5432 -e POSTGRES_PASSWORD=password -d postgres
```

## Running PostgreSQL for Production 

We recommend to deploy PostgreSQL with HA (high availability) mode and synchronous replication strategy. PostgreSQL server should be 9.3 or later as previous versions do not supporting large objects >2GB which are used for snapshot storage in Block Vault. The version of libpq EOSIO is compiled with does not need to be the same as the server it is connecting too. Versions of libpq prior to 9.3 are able to store snapshots >2GB in a PostgreSQL 9.3+ server.

### Database schema

This plugin would create two tables `BlockData` and `SnapshotData` if it is not already in the database. The tables are created in the following SQL statement.

```
CREATE TABLE IF NOT EXISTS BlockData (watermark_bn bigint, watermark_ts bigint, lib bigint, block_num bigint, block_id bytea UNIQUE, previous_block_id bytea, block oid, block_size bigint);
CREATE TABLE IF NOT EXISTS SnapshotData (watermark_bn bigint, watermark_ts bigint, snapshot oid);
```

## Plugin configuration

To use this plugin, `nodeos` has to be configured as a producer with `--block-vault-backend` option.  For example

```
nodeos --plugin eosio::producer_plugin --producer-name myproducera --plugin eosio::blockvault_client_plugin --block-vault-backend postgresql://user:password@mycompany.com
```

For production environment, we recommend to use `PGPASSWORD` environment variable to configure the password instead of embedding the password in the URI.

```
export PGPASSWORD=password
nodeos --plugin eosio::producer_plugin --producer-name myproducera --plugin eosio::blockvault_client_plugin --block-vault-backend postgresql://user@mycompany.com
```
