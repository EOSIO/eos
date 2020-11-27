# Summary
This how-to describes configuration of the Nodeos `backing store`. `Nodeos` can now use `chainbase` or `rocksdb` as a backing store for smart contract state.
   
# Prerequisites
Version 2.1 or above of the EOSIO development environment. 

# Parameter Definitions 
Specify which backing store to use with the `chain_plugin` `--backing-store` argument. This argument sets state storage to either `chainbase`, the default, or `rocksdb`.

```console
Config Options for eosio::chain_plugin:

  --backing-store arg (=chainbase)      The storage for state, chainbase or 
                                        rocksdb
  --persistent-storage-num-threads arg 	Number of rocksdb threads for flush and
                                        compaction.   
  --persistent-storage-max-num-files arg (=-1)
  										Max number of rocksdb files to keep 
                                        open. -1 = unlimited.
  --persistent-storage-write-buffer-size-mb arg (=128)
                                        Size of a single rocksdb memtable (in MiB).
  --persistent-storage-bytes-per-sync   Rocksdb write rate of flushes and compactions.
  --persistent-storage-mbytes-snapshot-batch
										Rocksdb batch size threshold before writing read in snapshot data to database.
``` 

# Procedure
To use `rocksdb` for state storage:

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --backing-store=’rocksdb’ --persistent-storage-num-threads=’2’ --persistent-storage-max-num-files=’2’ --persistent-storage-write-buffer-size-mb=’128’  --plugin eosio::http_plugin 
```

To use `chainbase` for state storage:

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --backing-store=’chainbase’ --plugin eosio::http_plugin 
```

or

```shell
nodeos -e -p eosio --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::http_plugin 
```

