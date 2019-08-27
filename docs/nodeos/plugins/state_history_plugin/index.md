# state_history_plugin

## Description

The `state_history_plugin` is useful for capturing historical data about the blockchain state. The plugin receives blockchain data from other connected nodes and caches the data into files. The `state_history_plugin` listens on a socket for applications to connect and sends blockchain data back based on the plugin options specified when starting `nodeos`.

## Usage

The `state_history_plugin` options are specified on either `config.ini` or the command line:

```sh
# config.ini
plugin = eosio::state_history_plugin [options]

# nodeos startup params
$ nodeos ... --plugin eosio::state_history_plugin [options]
```

## Options

[TODO: choose style: markdown table or nodeos output]

options                        | config.ini or command-line | description
-------------------------------|----------------------------|------------
`--state-history-dir arg`      | both                       | optional; defaults to `"state-history"`
`--trace-history`              | both                       | enable to collect transaction and action traces
`--chain-state-history`        | both                       | enable to collect state deltas
`--state-history-endpoint arg` | both                       | optional; defaults to `127.0.0.1:8080`<br>the endpoint upon which to listen for incoming connections.<br>Caution: only expose this port to your internal network.
`--trace-history-debug-mode`   | both                       | enable debug mode for trace history
`--delete-state-history`       | command-line only          | clear state history files

```sh
# config options for eosio::state_history_plugin

  --state-history-dir arg       (default="state-history")
                                the location of the state-history 
                                directory (absolute path or relative to
                                application data dir)
  --trace-history               enable trace history
  --chain-state-history         enable chain state history
  --state-history-endpoint arg  (default=127.0.0.1:8080)
                                the endpoint upon which to listen for 
                                incoming connections. Caution: only 
                                expose this port to your internal 
                                network.
  --trace-history-debug-mode    enable debug mode for trace history

# command line options for eosio::state_history_plugin

  --delete-state-history        clear state history files

```

## Examples

* [`js-example`](examples/js-example.md)
* [`history-tools`](examples/history-tools/README.md)

## Dependencies

[`chain_plugin`](../chain_plugin/index.md)

### Load Dependency Examples

```sh
# config.ini
plugin = eosio::chain_plugin --disable-replay-opts

# nodeos startup params
$ nodeos ... --plugin eosio::chain_plugin --disable-replay-opts
```

## How-To Guides

* [How to fast start without history on existing chains](how-to-fast-start-without-old-history.md)
* [How to create a portable snapshot with full state history](how-to-create-snapshot-with-full-history.md)
* [How to restore a portable snapshot with full state history](how-to-restore-snapshot-with-full-history.md)
* [How to replay or resync with full history](how-to-replay-or-resync-wth-full-history.md)
