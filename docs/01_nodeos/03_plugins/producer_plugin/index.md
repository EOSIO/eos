# producer_plugin

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
  --max-block-cpu-usage-threshold-us    Threshold of CPU block production to 
                                        consider block full; when within threshold 
                                        of max-block-cpu-usage block can be 
                                        produced immediately. Default value 5000
  --max-block-net-usage-threshold-bytes Threshold of NET block production to 
                                        consider block full; when within threshold
                                        of max-block-net-usage block can be produced
                                        immediately. Default value 1024
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
                                        
                                           <provider-type> is KEY, or KEOSD
                                        
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
  --keosd-provider-timeout arg (=5)     Limits the maximum time (in 
                                        milliseconds) that is allowed for 
                                        sending blocks to a keosd provider for 
                                        signing
  --greylist-account arg                account that can not access to extended
                                        CPU/NET virtual resources
  --produce-time-offset-us arg (=0)     offset of non last block producing time
                                        in microseconds. Negative number 
                                        results in blocks to go out sooner, and
                                        positive number results in blocks to go
                                        out later
  --last-block-time-offset-us arg (=0)  offset of last block producing time in 
                                        microseconds. Negative number results 
                                        in blocks to go out sooner, and 
                                        positive number results in blocks to go
                                        out later
  --max-scheduled-transaction-time-per-block-ms arg (=100)
                                        Maximum wall-clock time, in 
                                        milliseconds, spent retiring scheduled 
                                        transactions in any block before 
                                        returning to normal transaction 
                                        processing.
  --incoming-defer-ratio arg (=1)       ratio between incoming transactions and 
                                        deferred transactions when both are 
                                        queued for execution                                        
                                                                            
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
