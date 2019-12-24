
The plugin-specific options can be configured using either CLI options or a configuration file, `config.ini`. Nodapifiny-specific options can only be configured from the command line. All CLI options and `config.ini` options can be found by running `$ nodapifiny --help` as shown above.

Each `config.ini` option has a corresponding CLI option. However, not all CLI options are available in `config.ini`. For instance, most plugin-specific options that perform actions are not available in `config.ini`, such as `--delete-state-history` from `state_history_plugin`.

For example, the CLI option `--plugin apifiny::chain_api_plugin` can also be set by adding `plugin = apifiny::chain_api_plugin` in `config.ini`.

## `config.ini` location

The default `config.ini` can be found in the following folders:
- Mac OS: `~/Library/Application Support/apifiny/nodapifiny/config`
- Linux: `~/.local/share/apifiny/nodapifiny/config`

A custom `config.ini` file can be set by passing the `nodapifiny` option `--config path/to/config.ini`.

## Nodapifiny Example

The example below shows a typical usage of `nodapifiny` when starting a block producing node:

```sh
$ nodapifiny --replay-blockchain \
  -e -p apifiny \
  --plugin apifiny::producer_plugin  \
  --plugin apifiny::chain_api_plugin \
  --plugin apifiny::http_plugin      \
  >> nodapifiny.log 2>&1 &
```

```sh
$ nodapifiny \
  -e -p apifiny \
  --data-dir /users/mydir/apifiny/data     \
  --config-dir /users/mydir/apifiny/config \
  --plugin apifiny::producer_plugin      \
  --plugin apifiny::chain_plugin         \
  --plugin apifiny::http_plugin          \
  --plugin apifiny::state_history_plugin \
  --contracts-console   \
  --disable-replay-opts \
  --access-control-allow-origin='*' \
  --http-validate-host=false        \
  --verbose-http-errors             \
  --state-history-dir /shpdata \
  --trace-history              \
  --chain-state-history        \
  >> nodapifiny.log 2>&1 &
```

The above `nodapifiny` command starts a producing node by:

* enabling block production (`-e`)
* identifying itself as block producer "apifiny" (`-p`)
* setting the blockchain data directory (`--data-dir`)
* setting the `config.ini` directory (`--config-dir`)
* loading plugins `producer_plugin`, `chain_plugin`, `http_plugin`, `state_history_plugin` (`--plugin`)
* passing `chain_plugin` options (`--contracts-console`, `--disable-replay-opts`)
* passing `http-plugin` options (`--access-control-allow-origin`, `--http-validate-host`, `--verbose-http-errors`)
* passing `state_history` options (`--state-history-dir`, `--trace-history`, `--chain-state-history`)
* redirecting both `stdout` and `stderr` to the `nodapifiny.log` file
* returning to the shell by running in the background (&)
