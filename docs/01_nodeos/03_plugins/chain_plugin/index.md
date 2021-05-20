## Description

The `chain_plugin` is a core plugin required to process and aggregate chain data on an EOSIO node.

## Usage

```console
# config.ini
plugin = eosio::chain_plugin
[options]
```
```sh
# command-line
nodeos ... --plugin eosio::chain_plugin [operations] [options]
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
  --print-build-info                    print build environment information to 
                                        console as JSON and exit
  --extract-build-info arg              extract build environment information 
                                        as JSON, write into specified file, and
                                        exit
  --fix-reversible-blocks               recovers reversible block database if 
                                        that database is in a bad state
  --force-all-checks                    do not skip any validation checks while
                                        replaying blocks (useful for replaying 
                                        blocks from untrusted source)
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
  --terminate-at-block arg (=0)         terminate after reaching this block 
                                        number (if set to a non-zero number)
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
  --blocks-log-stride arg (=4294967295) split the block log file when the head 
                                        block number is the multiple of the 
                                        stride
                                        When the stride is reached, the current
                                        block log and index will be renamed 
                                        '<blocks-retained-dir>/blocks-<start 
                                        num>-<end num>.log/index'
                                        and a new current block log and index 
                                        will be created with the most recent 
                                        block. All files following
                                        this format will be used to construct 
                                        an extended block log.
  --max-retained-block-files arg (=10)  the maximum number of blocks files to 
                                        retain so that the blocks in those 
                                        files can be queried.
                                        When the number is reached, the oldest 
                                        block file would be moved to archive 
                                        dir or deleted if the archive dir is 
                                        empty.
                                        The retained block log files should not
                                        be manipulated by users.
  --blocks-retained-dir arg (="")       the location of the blocks retained 
                                        directory (absolute path or relative to
                                        blocks dir).
                                        If the value is empty, it is set to the
                                        value of blocks dir.
  --blocks-archive-dir arg (="archive") the location of the blocks archive 
                                        directory (absolute path or relative to
                                        blocks dir).
                                        If the value is empty, blocks files 
                                        beyond the retained limit will be 
                                        deleted.
                                        All files in the archive directory are 
                                        completely under user's control, i.e. 
                                        they won't be accessed by nodeos 
                                        anymore.
  --fix-irreversible-blocks arg (=1)    When the existing block log is 
                                        inconsistent with the index, allows 
                                        fixing the block log and index files 
                                        automatically - that is, it will take 
                                        the highest indexed block if it is 
                                        valid; otherwise it will repair the 
                                        block log and reconstruct the index.
  --protocol-features-dir arg (="protocol_features")
                                        the location of the protocol_features 
                                        directory (absolute path or relative to
                                        application config dir)
  --checkpoint arg                      Pairs of [BLOCK_NUM,BLOCK_ID] that 
                                        should be enforced as checkpoints.
  --wasm-runtime runtime (=eos-vm-jit)  Override default WASM runtime ( 
                                        "eos-vm-jit", "eos-vm")
                                        "eos-vm-jit" : A WebAssembly runtime 
                                        that compiles WebAssembly code to 
                                        native x86 code prior to execution.
                                        "eos-vm" : A WebAssembly interpreter.
                                        
  --abi-serializer-max-time-ms arg (=15)
                                        Override default maximum ABI 
                                        serialization time allowed in ms
  --chain-state-db-size-mb arg (=1024)  Maximum size (in MiB) of the chain 
                                        state database
  --chain-state-db-guard-size-mb arg (=128)
                                        Safely shut down node when free space 
                                        remaining in the chain state database 
                                        drops below this size (in MiB).
  --backing-store arg (=chainbase)      The storage for state, chainbase or 
                                        rocksdb
  --persistent-storage-num-threads arg (=1)
                                        Number of rocksdb threads for flush and
                                        compaction
  --persistent-storage-max-num-files arg (=-1)
                                        Max number of rocksdb files to keep 
                                        open. -1 = unlimited.
  --persistent-storage-write-buffer-size-mb arg (=128)
                                        Size of a single rocksdb memtable (in 
                                        MiB)
  --persistent-storage-bytes-per-sync arg (=1048576)
                                        Rocksdb write rate of flushes and 
                                        compactions.
  --persistent-storage-mbytes-snapshot-batch arg (=50)
                                        Rocksdb batch size threshold before 
                                        writing read in snapshot data to 
                                        database.
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
  --deep-mind                           print deeper information about chain 
                                        operations
  --telemetry-url arg                   Send Zipkin spans to url. e.g. 
                                        http://127.0.0.1:9411/api/v2/spans
  --telemetry-service-name arg (=nodeos)
                                        Zipkin localEndpoint.serviceName sent 
                                        with each span
  --telemetry-timeout-us arg (=200000)  Timeout for sending Zipkin span.
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
                                        In "speculative" mode: database 
                                        contains state changes by transactions 
                                        in the blockchain up to the head block 
                                        as well as some transactions not yet 
                                        included in the blockchain.
                                        In "head" mode: database contains state
                                        changes by only transactions in the 
                                        blockchain up to the head block; 
                                        transactions received by the node are 
                                        relayed if valid.
                                        In "read-only" mode: (DEPRECATED: see 
                                        p2p-accept-transactions & 
                                        api-accept-transactions) database 
                                        contains state changes by only 
                                        transactions in the blockchain up to 
                                        the head block; transactions received 
                                        via the P2P network are not relayed and
                                        transactions cannot be pushed via the 
                                        chain API.
                                        In "irreversible" mode: database 
                                        contains state changes by only 
                                        transactions in the blockchain up to 
                                        the last irreversible block; 
                                        transactions received via the P2P 
                                        network are not relayed and 
                                        transactions cannot be pushed via the 
                                        chain API.
                                        
  --api-accept-transactions arg (=1)    Allow API transactions to be evaluated 
                                        and relayed if valid.
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
  --maximum-variable-signature-length arg (=16384)
                                        Subjectively limit the maximum length 
                                        of variable components in a variable 
                                        legnth signature to this size in bytes
  --trusted-producer arg                Indicate a producer whose blocks 
                                        headers signed by it will be fully 
                                        validated, but transactions in those 
                                        validated blocks will be trusted.
  --database-map-mode arg (=mapped)     Database map mode ("mapped", "heap", or
                                        "locked").
                                        In "mapped" mode database is memory 
                                        mapped as a file.
                                        In "heap" mode database is preloaded in
                                        to swappable memory and will use huge 
                                        pages if available.
                                        In "locked" mode database is preloaded,
                                        locked in to memory, and will use huge 
                                        pages if available.
                                        
  --enable-account-queries arg (=0)     enable queries to find accounts by 
                                        various metadata.
  --max-nonprivileged-inline-action-size arg (=4096)
                                        maximum allowed size (in bytes) of an 
                                        inline action for a nonprivileged 
                                        account
```

## Dependencies

None
