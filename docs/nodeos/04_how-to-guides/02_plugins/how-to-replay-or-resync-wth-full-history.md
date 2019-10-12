# How to replay or resync with full history

## Goal

This procedure records the entire chain history.

## Before you begin

[TODO] (things to have knowledge/mastery of before carrying out the how-to)

## Steps

1. Get a block log and place it in `data/blocks`, or get a genesis file and use the `--genesis-json` option
2. Make sure `data/state` does not exist, or use the `--replay-blockchain` option
3. Start `nodeos` with the `--snapshot` option, and the options listed in the [`state_history_plugin`]
