# Nodeos Configuration

`Nodeos` is a command line interface (CLI) application. As such, it can be started manually from the command line or through an automated script. The behavior of `nodeos` is determined mainly by which plugins are loaded and which plugin options are used. The `nodeos` application features two main option categories: *nodeos-specific* options and *plugin-specific* options.

## Nodeos-specific Options

Nodeos-specific options are used mainly for housekeeping purposes, such as setting the directory where the blockchain data resides, specifying the name of the `nodeos` configuraton file, setting the name and path of the logging configuration file, etc. A sample output from running  `$ nodeos --help` is displayed below, showing the nodeos-specific options. The actual output also includes plugin-specific options, which have been excluded for clarity:

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

For details on options for each plugin please visit the [Plugins](../04_plugins/index.md) section:

## Nodeos Configuration

The plugin-specific options can be configured using either CLI options or a configuration file, `config.ini`. Nodeos-specific options can only be configured from the command line. All CLI options and `config.ini` options can be found by running `$ nodeos --help` as shown above.

Each `config.ini` option has a corresponding CLI option. However, not all CLI options are available in `config.ini`. For instance, most plugin-specific options that perform actions are not available in `config.ini`, such as `--delete-state-history` from `state_history_plugin`.

For example, the CLI option `--plugin eosio::chain_api_plugin` can also be set by adding `plugin = eosio::chain_api_plugin` in `config.ini`.

## `config.ini` location

The default `config.ini` can be found in the following folders:
- Mac OS: `~/Library/Application Support/eosio/nodeos/config`
- Linux: `~/.local/share/eosio/nodeos/config`

A custom `config.ini` file can be set by passing the `nodeos` option `--config path/to/config.ini`.

## Nodeos Example

The example below shows a typical usage of `nodeos` when starting a block producing node:

```sh
$ nodeos --replay-blockchain \
  -e -p eosio \
  --plugin eosio::producer_plugin  \
  --plugin eosio::chain_api_plugin \
  --plugin eosio::http_plugin      \
  >> nodeos.log 2>&1 &
```

```sh
$ nodeos \
  -e -p eosio \
  --data-dir /users/mydir/eosio/data     \
  --config-dir /users/mydir/eosio/config \
  --plugin eosio::producer_plugin      \
  --plugin eosio::chain_plugin         \
  --plugin eosio::http_plugin          \
  --plugin eosio::state_history_plugin \
  --contracts-console   \
  --disable-replay-opts \
  --access-control-allow-origin='*' \
  --http-validate-host=false        \
  --verbose-http-errors             \
  --state-history-dir /shpdata \
  --trace-history              \
  --chain-state-history        \
  >> nodeos.log 2>&1 &
```

The above `nodeos` command starts a producing node by:

* enabling block production (`-e`)
* identifying itself as block producer "eosio" (`-p`)
* setting the blockchain data directory (`--data-dir`)
* setting the `config.ini` directory (`--config-dir`)
* loading plugins `producer_plugin`, `chain_plugin`, `http_plugin`, `state_history_plugin` (`--plugin`)
* passing `chain_plugin` options (`--contracts-console`, `--disable-replay-opts`)
* passing `http-plugin` options (`--access-control-allow-origin`, `--http-validate-host`, `--verbose-http-errors`)
* passing `state_history` options (`--state-history-dir`, `--trace-history`, `--chain-state-history`)
* redirecting both `stdout` and `stderr` to the `nodeos.log` file
* returning to the shell by running in the background (&)

## What's Next?

* [Nodeos Common Configurations](00_common-configurations/index.md)
* [Nodeos Test Environment](01_test-environment/index.md)
