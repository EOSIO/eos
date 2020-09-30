# Summary
This how-to describes configuration of the Nodeos `backing store`. Nodeos can now use chainbase or rocks-db as a backing store for smart contract state.
   
# Prerequisites
Version 2.1 or above of the EOSIO development environment. 

# Parameter Definitions 
Specify which backing store to use with the chain_plugin --backing-store argument. This argument sets state storage to either CHAINBASE, the default, or ROCKSDB.

Config Options for eosio::chain_plugin:

 --backing-store arg (=rocksdb)        The storage for state, CHAINBASE or 
                                        ROCKSDB
  --rocksdb-threads arg (=1)            Number of rocksdb threads for flush and
                                        compaction
  --rocksdb-files arg (=-1)             Max number of rocksdb files to keep 
                                        open. -1 = unlimited.
 
# Procedure
To use `RocksDB` for state storage:

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --backing-store=’ROCKSDB’ --rocksdb-threads=’4’ --rocksdb-files=’2’ --plugin eosio::http_plugin 
```

To use `chainbase` for state storage:

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --backing-store=’CHAINBASE’ --plugin eosio::http_plugin 
```

or

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::http_plugin 
```

# Next Steps



