# Summary
This how-to describes configuration of the Nodeos `backing store`. `Nodeos` can now use `chainbase` or `rocksdb` as a backing store for smart contract state.
   
# Prerequisites
Version 2.1 or above of the EOSIO development environment. 

# Parameter Definitions 
Specify which backing store to use with the `chain_plugin` `--backing-store` argument. This argument sets state storage to either `chainbase`, the default, or `rocksdb`.

Config Options for eosio::chain_plugin:

  --backing-store arg (=chainbase)      The storage for state, chainbase or 
                                        rocksdb
  --rocksdb-threads arg (=1)            Number of rocksdb threads for flush and
                                        compaction
  --rocksdb-files arg (=-1)             Max number of rocksdb files to keep 
                                        open. -1 = unlimited.
  --rocksdb-write-buffer-size arg (=134217728)
                                        Size of a single rocksdb memtable
  --rocksdb-target-file-size-base arg (=52428800)
                                        Size of a level-1 file
  --rocksdb-max-bytes-for-level-base arg (=536870912)
                                        Maximum data size for level-1
 
# Procedure
To use `rocksdb` for state storage:

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --backing-store=’rocksdb’ --rocksdb-threads=’2’ --rocksdb-files=’2’ --plugin eosio::http_plugin 
```

To use `chainbase` for state storage:

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --backing-store=’chainbase’ --plugin eosio::http_plugin 
```

or

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::http_plugin 
```

# Next Steps
