<!-- # How to fast start without history on existing chains -->

## Goal

This procedure records the current chain state and future history, without previous historical data on the local chain.

## Before you begin

* Make sure [APIFINY is installed](../../../00_install/index.md).
* Learn about [Using Nodapifiny](../../02_usage/index.md).
* Get familiar with [state_history](../../03_plugins/state_history_plugin/index.md) plugin.

## Steps

1. Get the following:
   * A portable snapshot (`data/snapshots/snapshot-xxxxxxx.bin`)
   * Optional: a block log which includes the block the snapshot was taken at

2. Make sure `data/state` does not exist

3. Start `nodapifiny` with the `--snapshot` option, and the options listed in the [`state_history_plugin`](#index.md).

4. Look for `Placing initial state in block n` in the log, where n is the start block number.

5. Start a filler with `--fpg-create` (if PostgreSQL), `--fill-skip-to n`, and `--fill-trim`. Replace `n` with the value above.

6. Do not stop `nodapifiny` until it has received at least 1 block from the network, or it won't be able to restart.

## Remarks

If `nodapifiny` fails to receive blocks from the network, then try the above using `net_api_plugin`. Use `clapifiny net disconnect` and `clapifiny net connect` to reconnect nodes which timed out.

[[caution | Caution when using `net_api_plugin`]]
| Either use a firewall to block access to your `http-server-address`, or change it to `localhost:8888` to disable remote access.

[[info]]
| Whenever you run a filler after this point, use the `--fill-trim` option. Only use `--fpg-create` and `--fill-skip-to` the first time.

[[info]]
| On large chains, this procedure creates a delta record that is too large for javascript processes to handle. 64-bit C++ processes can handle the large record. `fill-pg` and `fill-lmdb` break up the large record into smaller records when filling databases.
