# Nodeos Options

`Nodeos` is a command line interface (CLI) application. As such, it can be started manually from the command line or through an automated script. The behavior of `nodeos` is determined mainly by which plugins are loaded and which plugin options are used. The `nodeos` application features two main option categories: *nodeos-specific* options and *plugin-specific* options.

## Nodeos-specific Options

Nodeos-specific options are used mainly for housekeeping purposes, such as setting the directory where the blockchain data resides, specifying the name of the `nodeos` configuraton file, setting the name and path of the logging configuration file, etc. A sample output from running  `$ nodeos --help` is displayed below, showing the nodeos-specific options. The plugin-specific options have been excluded for clarity:

```sh
Application Config Options:
  --plugin arg                          Plugin(s) to enable, may be specified 
                                        multiple times

Application Command Line Options:
  -h [ --help ]                         Print this help message and exit.
  -v [ --version ]                      Print version information.
  --print-default-config                Print default configuration template
  -d [ --data-dir ] arg                 Directory containing program runtime 
                                        data
  --config-dir arg                      Directory containing configuration 
                                        files such as config.ini
  -c [ --config ] arg (=config.ini)     Configuration file name relative to 
                                        config-dir
  -l [ --logconf ] arg (=logging.json)  Logging configuration file name/path 
                                        for library users
```

## Plugin-specific Options

Plugin-specific options control the behavior of nodeos plugins. Every plugin-specific option has a unique name, so it can be specified in any order within the command line or `config.ini` file. When specifying one or more plugin-specific option(s), the applicable plugin(s) must also be enabled using the `--plugin` option or else the corresponding option(s) will be ignored. A sample output from running `$ nodeos --help` is displayed below, showing an excerpt from the plugin-specific options:

```sh
Config Options for eosio::bnet_plugin:
  --bnet-endpoint arg (=0.0.0.0:4321)   the endpoint upon which to listen for 
                                        incoming connections
  ...

Config Options for eosio::chain_plugin:
  --blocks-dir arg (="blocks")          the location of the blocks directory 
                                        (absolute path or relative to 
                                        application data dir)
  --checkpoint arg                      Pairs of [BLOCK_NUM,BLOCK_ID] that 
                                        should be enforced as checkpoints.
  ...

Command Line Options for eosio::chain_plugin:
  --genesis-json arg                    File to read Genesis State from
  ...
  --snapshot arg                        File to read Snapshot State from

...

Config Options for eosio::http_plugin:
  --http-server-address arg (=127.0.0.1:8888)
                                        The local IP and port to listen for 
                                        incoming http connections; set blank to
                                        disable.
  ...

...

Config Options for eosio::net_plugin:
  --p2p-listen-endpoint arg (=0.0.0.0:9876)
                                        The actual host:port used to listen for
                                        incoming p2p connections.
  ...

Config Options for eosio::producer_plugin:

  -e [ --enable-stale-production ]      Enable block production, even if the 
                                        chain is stale.
  ...
  --snapshots-dir arg (="snapshots")    the location of the snapshots directory
                                        (absolute path or relative to 
                                        application data dir)

Config Options for eosio::state_history_plugin:
  ...
  --trace-history                       enable trace history
  --chain-state-history                 enable chain state history
  ...

Command Line Options for eosio::state_history_plugin:
  --delete-state-history                clear state history files

Config Options for eosio::txn_test_gen_plugin:
  --txn-reference-block-lag arg (=0)    Lag in number of blocks from the head 
                                        block when selecting the reference 
                                        block for transactions (-1 means Last 
                                        Irreversible Block)
```

For more information on each plugin-specific option, please visit the [Plugins](../03_plugins/index.md) section.
