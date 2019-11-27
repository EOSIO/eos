# history_plugin

[[warning - Deprecation Notice]]
| The `history_plugin` is deprecated and will no longer be maintained. Please use the [`state_history_plugin`](../state_history_plugin/index.md) instead.

## Description

The `history_plugin` provides a cache layer to obtain historical data about the blockchain objects. It depends on [`chain_plugin`](../chain_plugin/index.md) for the data.

## Usage

```sh
# config.ini
plugin = eosio::history_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::history_plugin [options]
```

## Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::history_plugin:
  -f [ --filter-on ] arg                Track actions which match 
                                        receiver:action:actor. Actor may be 
                                        blank to include all. Action and Actor 
                                        both blank allows all from Recieiver. 
                                        Receiver may not be blank.
  -F [ --filter-out ] arg               Do not track actions which match 
                                        receiver:action:actor. Action and Actor
                                        both blank excludes all from Reciever. 
                                        Actor blank excludes all from 
                                        reciever:action. Receiver may not be 
                                        blank.
```

## Dependencies

* [`chain_plugin`](../chain_plugin/index.md)
