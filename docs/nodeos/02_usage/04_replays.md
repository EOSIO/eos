# Nodeos Replays

Nodeos provides various options for replaying blockchain blocks. This can be useful if, for example, a node has downloaded a `blocks.log` file from the internet (as a faster alternative to synchronizing from the p2p network) and the node wants to use it to quickly catch up with the network, or if you want to know chain state at specified points in a blockchain's life.

Replaying data can be done in two ways:

- From a **`blocks.log` file**:  
The `blocks.log` file contains all irreversible transactions on the blockchain. All instances of `nodeos` write irreversible blocks to the `blocks.log` file, which is located at the `data/blocks` directory relative to the `nodeos` directory. Using a `blocks.log` file to replay will allow you to start a `nodeos` instance, which recreates the entire history of the blockchain locally, without adding unnecessary load to the network.

- From a **snapshot file**:  
Snapshot files can be created from a running `nodeos` instance. The snapshot contains the chain state for the block referenced when created. It is recommended to use snapshot files created from blocks that are irreversible. Using a snapshot file to replay allows you to quickly start a `nodeos` instance which has a full and correct chain state at a specified block number, but not a full history of transactions up to that block number. From that point on the `nodeos` instance will operate in the configured manner.

## Replay How-Tos

* [How To Generate a Blocks Log](../04_how-tos/01_replays/00_how-to-generate-a-blocks-log.md)
* [How To Replay from a Blocks Log](../04_how-tos/01_replays/01_how-to-replay-from-a-blocks.log.md)
* [How To Generate a Snapshot](../04_how-tos/01_replays/02_how-to-generate-a-snapshot.md)
* [How to Replay from a Snapshot](../04_how-tos/01_replays/03_how-to-replay-from-snapshot.md)

## Replay Snapshot-specific Options

Typing `$ nodeos --help` on the command line will show you all the options available for running `nodeos`. The snapshot and replay specific options are:

 - **--force-all-checks**  
The node operator may not trust the source of the `blocks.log` file and may want to run `nodeos` with `--replay-blockchain --force-all-checks` the first time to make sure the blocks are good. The `--force-all-checks` flag can be passed into `nodeos` to tell it to not skip any checks during replay.

 - **--disable-replay-opts**  
By default, during replay, `nodeos` does not create a stack of chain state deltas (this stack is used to enable rollback of state for reversible blocks.) This is a replay performance optimization. Using this option turns off this replay optimization and creates a stack of chain state deltas. If you are using the state history plugin you must use this option.

 - **--replay-blockchain**  
This option tells `nodeos` to replay from the `blocks.log` file located in the data/blocks directory. `nodeos` will clear the chain state and replay all blocks.

 - **--hard-replay-blockchain**  
This option tells `nodeos` to replay from the `blocks.log` file located in the data/blocks directory. `nodeos` makes a backup of the existing `blocks.log` file and will then clear the chain state and replay all blocks. This option assumes that the backup `blocks.log` file may contain corrupted blocks, so `nodeos` replays as many blocks as possible from the backup block log. When `nodeos` finds the first corrupted block while replying from `nodeos.log` it will synchronize the rest of the blockchain from the p2p network.

 - **--delete-all-blocks**  
This tells `nodeos` to clear the local chain state and local the `blocks.log` file, If you intend to then synchronize from the p2p network you would need to provide a correct `genesis.json` file. This option is not recommended.

 - **--truncate-at-block**  
Default argument (=0), only used if the given value is non-zero.
Using this option when replaying the blockchain will force the replay to stop at the specified block number. This option will only work if replaying with the `--hard-replay-blockchain` option, or, when not replaying, the `--fix-reversible-blocks` option. The local `nodeos` process will contain the chain state for that block. This option may be useful for checking blockchain state at specific points in time. It is intended for testing/validation and is not intended to be used when creating a local `nodeos` instance which is synchronized with the network.  
 
 - **--snapshot**  
Use this option to specify which snapshot file to use to recreate the chain state from a snapshot file. This option will not replay the `blocks.log` file. The `nodeos` instance will not know the full transaction history of the blockchain. 

 - **--snapshots-dir**  
You can use this to specify the location of the snapshot file directory  (absolute path or relative to application data dir.)

 - **--blocks-dir**  
You can use this to specify the location of the `blocks.log` file directory  (absolute path or relative to application data dir)
