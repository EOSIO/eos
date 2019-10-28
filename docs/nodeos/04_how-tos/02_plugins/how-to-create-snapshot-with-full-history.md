# How to create a portable snapshot with full state history

## Goal

This procedure creates a database containing the chain state, with full history since genesis state.

## Before you begin
[TODO] (things to have knowledge/mastery of before carrying out the how-to)

## Steps

1. Enable the `producer_api_plugin` on a node with full state-history.

[[caution | Caution when using `producer_api_plugin`]]
| Either use a firewall to block access to `http-server-address`, or change it to `localhost:8888` to disable remote access.

2. Create a portable snapshot: `curl http://127.0.0.1:8888/v1/producer/create_snapshot | json_pp`
3. Wait for `nodeos` to process several blocks after the snapshot completed. The goal is for the state-history files to contain at least 1 more block than the portable snapshot has, and for the block log to contain the block after it has become irreversible.
   [[info | Note: if the block included in the portable snapshot is forked out, then the snapshot will be invalid. Repeat this process if this happens.]]
4. Stop `nodeos`
5. Make backups of:
   * The newly-created portable snapshot (`data/snapshots/snapshot-xxxxxxx.bin`)
   * The contents of `data/state-history`:
     * `chain_state_history.log`
     * `trace_history.log`
     * `chain_state_history.index`: optional. Restoring will take longer without this file.
     * `trace_history.index`: optional. Restoring will take longer without this file.
   * Optional: the contents of `data/blocks`, but excluding `data/blocks/reversible`.
