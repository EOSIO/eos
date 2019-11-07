# txn_test_gen_plugin

## Description

The `txn_test_gen_plugin` is used for transaction test purposes.

## Usage

```sh
# config.ini
plugin = eosio::txn_test_gen_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::txn_test_gen_plugin [options]
```

## Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::txn_test_gen_plugin:
  --txn-reference-block-lag arg (=0)    Lag in number of blocks from the head 
                                        block when selecting the reference 
                                        block for transactions (-1 means Last 
                                        Irreversible Block)
  --txn-test-gen-threads arg (=2)       Number of worker threads in 
                                        txn_test_gen thread pool
  --txn-test-gen-account-prefix arg (=txn.test.)
                                        Prefix to use for accounts generated 
                                        and used by this plugin
```

## Dependencies

None
