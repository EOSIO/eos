# Local Single-Node Testnet

After successfully building the project, the `nodeos` binary should be present in the `build/programs/nodeos` folder.  `nodeos` can be run directly from the `build` folder using `programs/nodeos/nodeos`, or you can `cd programs/nodeos` to change into the folder and run the `nodeos` command from there.  Here, we run the command within the `programs/nodeos` folder.

You can start your own single-node blockchain with this single command:

```sh
$ cd build/programs/nodeos
$ ./nodeos -e -p eosio --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin
```

When running `nodeos` you should get log messages similar to below. It means the blocks are successfully produced.

```console
1575001ms thread-0   chain_controller.cpp:235      _push_block          ] initm #1 @2017-09-04T04:26:15  | 0 trx, 0 pending, exectime_ms=0
1575001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initm generated block #1 @ 2017-09-04T04:26:15 with 0 trxs  0 pending
1578001ms thread-0   chain_controller.cpp:235      _push_block          ] initc #2 @2017-09-04T04:26:18  | 0 trx, 0 pending, exectime_ms=0
1578001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initc generated block #2 @ 2017-09-04T04:26:18 with 0 trxs  0 pending
...
eosio generated block 046b9984... #101527 @ 2018-04-01T14:24:58.000 with 0 trxs
eosio generated block 5e527ee2... #101528 @ 2018-04-01T14:24:58.500 with 0 trxs
...
```
At this point, `nodeos` is running with a single producer, `eosio`.

The following diagram depicts the single host testnet that we just created.   `cleos` is used to manage the wallets, manage the accounts, and invoke actions on the blockchain.  By default, `keosd` is started by `cleos` to perform wallet management.

![Single host single node testnet](single-host-single-node-testnet.png)

## Advanced Steps
The more advanced user will likely have need to modify the configuration.  `nodeos` uses a custom configuration folder.  The location of this folder is determined by your system.

- Mac OS: `~/Library/Application\ Support/eosio/nodeos/config`
- Linux: `~/.local/share/eosio/nodeos/config`

The build seeds this folder with a default `genesis.json` file.  A configuration folder can be specified using the `--config-dir` command line argument to `nodeos`.  If you use this option, you will need to manually copy a `genesis.json` file to your config folder.
 
`nodeos` will need a properly configured `config.ini` file in order to do meaningful work.  On startup, `nodeos` looks in the config folder for `config.ini`.  If one is not found, a default `config.ini` file is created.  If you do not already have a `config.ini` file ready to use, run `nodeos` and then close it immediately with <kbd>Ctrl-C</kbd>.  A default configuration (`config.ini`) will have been created in the config folder.  Edit the `config.ini` file, adding/updating the following settings to the defaults already in place:

```console
# config.ini:

    # Enable production on a stale chain, since a single-node test chain is pretty much always stale
    enable-stale-production = true
    # Enable block production with the testnet producers
    producer-name = eosio
    # Load the block producer plugin, so you can produce blocks
    plugin = eosio::producer_plugin
    # As well as API and HTTP plugins
    plugin = eosio::chain_api_plugin
    plugin = eosio::http_plugin
    plugin = eosio::history_api_plugin
```

Now it should be possible to run `nodeos` and see it begin producing blocks.

```sh
$ ./programs/nodeos/nodeos
```

`nodeos` stores runtime data (e.g., shared memory and log content) in a custom data folder.  The location of this folder is determined by your system.

- Mac OS: `~/Library/Application\ Support/eosio/nodeos/data`
- Linux: `~/.local/share/eosio/nodeos/data`
 
A data folder can be specified using the `--data-dir` command line argument to `nodeos`.
