
## Description

The `producer_plugin` loads functionality required for a node to produce blocks.

[[info]]
| Additional configuration is required to produce blocks. Please read [Configuring Block Producing Node](../../02_usage/02_node-setups/00_producing-node.md).

## Usage

```console
# config.ini
plugin = eosio::producer_plugin [options]
```
```sh
# nodeos startup params
nodeos ... -- plugin eosio::producer_plugin [options]
```

## Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::producer_plugin:

  -e [ --enable-stale-production ]      Enable block production, even if the 
                                        chain is stale.
  -x [ --pause-on-startup ]             Start this node in a state where 
                                        production is paused
  --max-transaction-time arg (=30)      Limits the maximum time (in 
                                        milliseconds) that is allowed a pushed 
                                        transaction's code to execute before 
                                        being considered invalid
  --max-irreversible-block-age arg (=-1)
                                        Limits the maximum age (in seconds) of 
                                        the DPOS Irreversible Block for a chain
                                        this node will produce blocks on (use 
                                        negative value to indicate unlimited)
  -p [ --producer-name ] arg            ID of producer controlled by this node 
                                        (e.g. inita; may specify multiple 
                                        times)
  --private-key arg                     (DEPRECATED - Use signature-provider 
                                        instead) Tuple of [public key, WIF 
                                        private key] (may specify multiple 
                                        times)
  --signature-provider arg (=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV=KEY:5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3)
                                        Key=Value pairs in the form 
                                        <public-key>=<provider-spec>
                                        Where:
                                           <public-key>    is a string form of 
                                                           a vaild EOSIO public
                                                           key
                                        
                                           <provider-spec> is a string in the 
                                                           form <provider-type>
                                                           :<data>
                                        
                                           <provider-type> is KEY, KEOSD, or SE
                                        
                                           KEY:<data>      is a string form of 
                                                           a valid EOSIO 
                                                           private key which 
                                                           maps to the provided
                                                           public key
                                        
                                           KEOSD:<data>    is the URL where 
                                                           keosd is available 
                                                           and the approptiate 
                                                           wallet(s) are 
                                                           unlocked
                                        
                                           SE:             indicates the key 
                                                           resides in Secure 
                                                           Enclave
  --greylist-account arg                account that can not access to extended
                                        CPU/NET virtual resources
  --greylist-limit arg (=1000)          Limit (between 1 and 1000) on the 
                                        multiple that CPU/NET virtual resources
                                        can extend during low usage (only 
                                        enforced subjectively; use 1000 to not 
                                        enforce any limit)
  --produce-time-offset-us arg (=0)     Offset of non last block producing time
                                        in microseconds. Valid range 0 .. 
                                        -block_time_interval.
  --last-block-time-offset-us arg (=-200000)
                                        Offset of last block producing time in 
                                        microseconds. Valid range 0 .. 
                                        -block_time_interval.
  --cpu-effort-percent arg (=80)        Percentage of cpu block production time
                                        used to produce block. Whole number 
                                        percentages, e.g. 80 for 80%
  --last-block-cpu-effort-percent arg (=80)
                                        Percentage of cpu block production time
                                        used to produce last block. Whole 
                                        number percentages, e.g. 80 for 80%
  --max-block-cpu-usage-threshold-us arg (=5000)
                                        Threshold of CPU block production to 
                                        consider block full; when within 
                                        threshold of max-block-cpu-usage block 
                                        can be produced immediately
  --max-block-net-usage-threshold-bytes arg (=1024)
                                        Threshold of NET block production to 
                                        consider block full; when within 
                                        threshold of max-block-net-usage block 
                                        can be produced immediately
  --max-scheduled-transaction-time-per-block-ms arg (=100)
                                        Maximum wall-clock time, in 
                                        milliseconds, spent retiring scheduled 
                                        transactions in any block before 
                                        returning to normal transaction 
                                        processing.
  --subjective-cpu-leeway-us arg (=31000)
                                        Time in microseconds allowed for a 
                                        transaction that starts with 
                                        insufficient CPU quota to complete and 
                                        cover its CPU usage.
  --incoming-defer-ratio arg (=1)       ratio between incoming transactions and
                                        deferred transactions when both are 
                                        queued for execution
  --incoming-transaction-queue-size-mb arg (=1024)
                                        Maximum size (in MiB) of the incoming 
                                        transaction queue. Exceeding this value
                                        will subjectively drop transaction with
                                        resource exhaustion.
  --producer-threads arg (=2)           Number of worker threads in producer 
                                        thread pool
  --snapshots-dir arg (="snapshots")    the location of the snapshots directory
                                        (absolute path or relative to 
                                        application data dir)
```

## Dependencies

* [`chain_plugin`](../chain_plugin/index.md)

## The priority of transaction

You can give one of the transaction types priority over another when the producer plugin has a queue of transactions pending.

The option below sets the ratio between the incoming transaction and the deferred transaction:

```console
  --incoming-defer-ratio arg (=1)       
```

By default value of `1`, the `producer` plugin processes one incoming transaction per deferred transaction. When `arg` sets to `10`, the `producer` plugin processes 10 incoming transactions per deferred transaction. 

If the `arg` is set to a sufficiently large number, the plugin always processes the incoming transaction first until the queue of the incoming transactions is empty. Respectively, if the `arg` is 0, the `producer` plugin processes the deferred transactions queue first.


### Load Dependency Examples

```console
# config.ini
plugin = eosio::chain_plugin [operations] [options]
```
```sh
# command-line
nodeos ... --plugin eosio::chain_plugin [operations] [options]
```

For details about how blocks are produced please read the following [block producing explainer](10_block-producing-explained.md).
