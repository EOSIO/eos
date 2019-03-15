# Setup

## Configuration

| Option                                    | When to use |
|-------------------------------------------|-------------|
| `--plugin eosio::state_history_plugin`    | Always |
| `--state-history-endpoint`                | Optional; defaults to 127.0.0.1:8080 |
| `--trace-history`                         | Optional; collects transaction and action traces. If unsure, turn this on. |
| `--chain-state-history`                   | Optional; collects state deltas. If unsure, turn this on. |

Caution: either use a firewall to block access to the state-history endpoint, or leave it as `127.0.0.1:8080` to disable remote access.

## Fast start without old history

This procedure records the current chain state and future history.

* Get the following:
  * A portable snapshot (`data/snapshots/snapshot-xxxxxxx.bin`)
  * Optional: a block log which includes the block the snapshot was taken at
* Make sure `data/state` does not exist
* Start nodeos with the `--snapshot` option, and the options listed in "Configuration" above
* Do not stop nodeos until it has received at least 1 block from the network, or it won't be able to restart

If nodeos fails to receive blocks from the network, then try the above using `net_api_plugin`. Use `cleos net disconnect` and `cleos net connect` to reconnect nodes which timed out. Caution when using net_api_plugin: either use a firewall to block access to `http-server-address`, or change it to `localhost:8888` to disable remote access.

On large chains, this procedure creates a delta record that's too large for javascript processes to handle. 64-bit C++ processes can handle the large record. `fill-pg` and `fill-lmdb` break up the large record into smaller records when filling databases.

## Replay or resync with full history

This procedure records the entire chain history.

* Get a block log and place it in `data/blocks`, or get a genesis file and use the `--genesis-json` option
* Make sure `data/state` does not exist, or use the `--replay-blockchain` option
* Start nodeos with the `--snapshot` option, and the options listed in "Configuration" above

## Creating a portable snapshot with full history

* Enable the `producer_api_plugin` on a node with full history. Caution when using producer_api_plugin: either use a firewall to block access to `http-server-address`, or change it to `localhost:8888` to disable remote access.
* Create a portable snapshot: `curl http://127.0.0.1:8888/v1/producer/create_snapshot | json_pp`
* Wait for nodeos to process several blocks after the snapshot completed. The goal is for the state-history files to contain at least 1 more block than the portable snapshot has, and for the block log to contain the block after it has become irreversible.
* Note: if the block included in the portable snapshot is forked out, then the snapshot will be invalid. Repeat this process if this happens.
* Stop nodeos
* Make backups of:
  * The newly-created portable snapshot (`data/snapshots/snapshot-xxxxxxx.bin`)
  * The contents of `data/state-history`:
    * `chain_state_history.log`
    * `trace_history.log`
    * `chain_state_history.index`: optional. Restoring will take longer without this file.
    * `trace_history.index`: optional. Restoring will take longer without this file.
  * Optional: the contents of `data/blocks`, but excluding `data/blocks/reversible`.

## Restoring a portable snapshot with full history

* Get the following:
  * A portable snapshot (`data/snapshots/snapshot-xxxxxxx.bin`)
  * The contents of `data/state-history`
  * Optional: a block log which includes the block the snapshot was taken at. Do not include `data/blocks/reversible`.
* Make sure `data/state` does not exist
* Start nodeos with the `--snapshot` option, and the options listed in "Configuration" above
* Do not stop nodeos until it has received at least 1 block from the network, or it won't be able to restart.

If nodeos fails to receive blocks from the network, then try the above using `net_api_plugin`. Use `cleos net disconnect` and `cleos net connect` to reconnect nodes which timed out. Caution when using net_api_plugin: either use a firewall to block access to `http-server-address`, or change it to `localhost:8888` to disable remote access.
