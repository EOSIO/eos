---
content_title: Nodeos Troubleshooting
---

### "Database dirty flag set (likely due to unclean shutdown): replay required"

`nodeos` needs to be shut down cleanly. To ensure this is done, send a `SIGTERM`, `SIGQUIT` or `SIGINT` and wait for the process to shutdown. Failing to do this will result in this error. If you get this error, your only recourse is to replay by starting `nodeos` with `--replay-blockchain` 

### "Memory does not match data" Error at Restart

If you get an error such as `St9exception: content of memory does not match data expected by executable` when trying to start `nodeos`, try restarting `nodeos` with one of the following options (you can use `nodeos --help` to get a full listing of these).

```
Command Line Options for eosio::chain_plugin:
    --fix-reversible-blocks               recovers reversible block database if 
                                          that database is in a bad state
    --force-all-checks                    do not skip any checks that can be 
                                          skipped while replaying irreversible 
                                          blocks
    --replay-blockchain                   clear chain state database and replay 
                                          all blocks
    --hard-replay-blockchain              clear chain state database, recover as 
                                          many blocks as possible from the block 
                                          log, and then replay those blocks
    --delete-all-blocks                   clear chain state database and block 
                                          log
```

### "Could not grow database file to requested size." Error

Start `nodeos` with `--shared-memory-size-mb 1024`. A 1 GB shared memory file allows approximately half a million transactions.

### What version of EOSIO am I running/connecting to?

If defaults can be used, then `cleos get info` will output a block that contains a field called `server_version`.  If your `nodeos` is not using the defaults, then you need to know the URL of the `nodeos`. In that case, use the following with your `nodeos` URL:

```sh
cleos --url http://localhost:8888 get info
```

To focus only on the version line within the block:

```sh
cleos --url http://localhost:8888 get info | grep server_version
```

### Error 3070000: WASM Exception Error

If you try to deploy the `eosio.bios` contract or `eosio.system` contract in an attempt to boot an EOSIO-based blockchain and you get the following error or similar: `Publishing contract... Error 3070000: WASM Exception Error Details: env.set_proposed_producers_ex unresolveable`, it is because you have to activate the `PREACTIVATE_FEATURE` protocol first. More details about it and how to enable it can be found in the [Bios Boot Sequence Tutorial](https://developers.eos.io/welcome/latest/tutorials/bios-boot-sequence/#112-set-the-eosiosystem-contract). For more information, you may also visit the [Nodeos Upgrade Guides](https://developers.eos.io/manuals/eos/latest/nodeos/upgrade-guides/).
