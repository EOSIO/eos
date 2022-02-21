---
content_title: How to replay from a snapshot
---

Once you have obtained a copy of a valid snapshot file from which you wish to create a valid chain state, copy it to your data/snapshots directory, backing up (if you wish to keep them) and removing any existing contents of the data directory.

location          | name                       |  action
----------------- | -------------------------- | ------------
data/snapshots    | <head block id in hex>.bin | place the snapshot file you want to replay here
data/             | *                          | remove

You can use `snapshots-dir = "snapshots" ` in the configuration file or using the `--snapshots-dir` command line option, to specify the where to find the the snapshot to replay, use `--snapshot` to specify the name of the snapshot to replay.

```sh
nodeos --snapshot yoursnapshot.name \
  --plugin eosio::producer_plugin  \
  --plugin eosio::chain_api_plugin \
  --plugin eosio::http_plugin      \
  >> nodeos.log 2>&1 &
```

When replaying from a snapshot file it is recommended that all existing data is removed, however if a blocks.log file is provided it *must* at least contain blocks up to the snapshotted block and *may* contain additional blocks that will be applied as part of startup.  If a blocks.log file exists, but does not contain blocks up to and/or after the snapshotted block then replaying from a snapshot will create an exception. Any available reversible blocks will also be applied.

blocks.log               | snapshot                    | action
------------------------ | --------------------------- | ------
no blocks.log            | for irreversible block 2000 | ok
contains blocks 1 - 1999 | for irreversible block 2000 | exception
contains blocks 1 - 2001 | for irreversible block 2000 | ok - will recreate from snapshot and 'play' block 2001

When instantiating a node from a snapshot file, it is invalid to pass in the `--genesis-json` or `--genesis-timestamp` arguments to `nodeos` as that information is loaded from the snapshot file. If a `blocks.log` file exists, the genesis information it contains will be validated against the genesis data in the snapshot.  The replay will fail with an error if the genesis data is not consistent, i.e. it checks that the blocks.log file and the snapshot file are for the same blockchain.
