# chain_plugin

## Description

The `chain_plugin` is a core plugin required to process and aggregate chain data on an EOSIO node.

## Usage

```sh
# config.ini
plugin = eosio::chain_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::chain_plugin [operations] [options]
```

## Operations

These can only be specified from the `nodeos` command-line:

```console
Command Line Options for eosio::chain_plugin:

  --genesis-json arg                    File to read Genesis State from
  --genesis-timestamp arg               override the initial timestamp in the 
                                        Genesis State file
  --print-genesis-json                  extract genesis_state from blocks.log 
                                        as JSON, print to console, and exit
  --extract-genesis-json arg            extract genesis_state from blocks.log 
                                        as JSON, write into specified file, and
                                        exit
  --fix-reversible-blocks               recovers reversible block database if 
                                        that database is in a bad state
  --force-all-checks                    do not skip any checks that can be 
                                        skipped while replaying irreversible 
                                        blocks
  --disable-replay-opts                 disable optimizations that specifically
                                        target replay
  --replay-blockchain                   clear chain state database and replay 
                                        all blocks
  --hard-replay-blockchain              clear chain state database, recover as 
                                        many blocks as possible from the block 
                                        log, and then replay those blocks
  --delete-all-blocks                   clear chain state database and block 
                                        log
  --truncate-at-block arg (=0)          stop hard replay / block log recovery 
                                        at this block number (if set to 
                                        non-zero number)
  --import-reversible-blocks arg        replace reversible block database with 
                                        blocks imported from specified file and
                                        then exit
  --export-reversible-blocks arg        export reversible block database in 
                                        portable format into specified file and
                                        then exit
  --snapshot arg                        File to read Snapshot State from
```

## Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::chain_plugin:

  --blocks-dir arg (="blocks")          the location of the blocks directory 
                                        (absolute path or relative to 
                                        application data dir)
  --protocol-features-dir arg (="protocol_features")
                                        the location of the protocol_features 
                                        directory (absolute path or relative to
                                        application config dir)
  --checkpoint arg                      Pairs of [BLOCK_NUM,BLOCK_ID] that 
                                        should be enforced as checkpoints.
  --wasm-runtime wavm/wabt              Override default WASM runtime
  --abi-serializer-max-time-ms arg (=15000)
                                        Override default maximum ABI 
                                        serialization time allowed in ms
  --chain-state-db-size-mb arg (=1024)  Maximum size (in MiB) of the chain 
                                        state database
  --chain-state-db-guard-size-mb arg (=128)
                                        Safely shut down node when free space 
                                        remaining in the chain state database 
                                        drops below this size (in MiB).
  --reversible-blocks-db-size-mb arg (=340)
                                        Maximum size (in MiB) of the reversible
                                        blocks database
  --reversible-blocks-db-guard-size-mb arg (=2)
                                        Safely shut down node when free space 
                                        remaining in the reverseible blocks 
                                        database drops below this size (in 
                                        MiB).
  --signature-cpu-billable-pct arg (=50)
                                        Percentage of actual signature recovery
                                        cpu to bill. Whole number percentages, 
                                        e.g. 50 for 50%
  --chain-threads arg (=2)              Number of worker threads in controller 
                                        thread pool
  --contracts-console                   print contract's output to console
  --actor-whitelist arg                 Account added to actor whitelist (may 
                                        specify multiple times)
  --actor-blacklist arg                 Account added to actor blacklist (may 
                                        specify multiple times)
  --contract-whitelist arg              Contract account added to contract 
                                        whitelist (may specify multiple times)
  --contract-blacklist arg              Contract account added to contract 
                                        blacklist (may specify multiple times)
  --action-blacklist arg                Action (in the form code::action) added
                                        to action blacklist (may specify 
                                        multiple times)
  --key-blacklist arg                   Public key added to blacklist of keys 
                                        that should not be included in 
                                        authorities (may specify multiple 
                                        times)
  --sender-bypass-whiteblacklist arg    Deferred transactions sent by accounts 
                                        in this list do not have any of the 
                                        subjective whitelist/blacklist checks 
                                        applied to them (may specify multiple 
                                        times)
  --read-mode arg (=speculative)        Database read mode ("speculative", 
                                        "head", "read-only", "irreversible").
                                        In "speculative" mode database contains
                                        changes done up to the head block plus 
                                        changes made by transactions not yet 
                                        included to the blockchain.
                                        In "head" mode database contains 
                                        changes done up to the current head 
                                        block.
                                        In "read-only" mode database contains 
                                        changes done up to the current head 
                                        block and transactions cannot be pushed
                                        to the chain API.
                                        In "irreversible" mode database 
                                        contains changes done up to the last 
                                        irreversible block and transactions 
                                        cannot be pushed to the chain API.
                                        
  --validation-mode arg (=full)         Chain validation mode ("full" or 
                                        "light").
                                        In "full" mode all incoming blocks will
                                        be fully validated.
                                        In "light" mode all incoming blocks 
                                        headers will be fully validated; 
                                        transactions in those validated blocks 
                                        will be trusted 
                                        
  --disable-ram-billing-notify-checks   Disable the check which subjectively 
                                        fails a transaction if a contract bills
                                        more RAM to another account within the 
                                        context of a notification handler (i.e.
                                        when the receiver is not the code of 
                                        the action).
  --trusted-producer arg                Indicate a producer whose blocks 
                                        headers signed by it will be fully 
                                        validated, but transactions in those 
                                        validated blocks will be trusted.
  --database-map-mode arg (=mapped)     Database map mode ("mapped", "heap", or
                                        "locked").
                                        In "mapped" mode database is memory 
                                        mapped as a file.
                                        In "heap" mode database is preloaded in
                                        to swappable memory.
                                        In "locked" mode database is preloaded,
                                        locked in to memory, and optionally can
                                        use huge pages.
                                        
  --database-hugepage-path arg          Optional path for database hugepages 
                                        when in "locked" mode (may specify 
                                        multiple times)
```

## Dependencies

None
