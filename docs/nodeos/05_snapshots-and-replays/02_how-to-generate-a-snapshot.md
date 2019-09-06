## How To Generate a Snapshot

You can force a running nodeos node to create a snapshot by using the `create_snapshot` RPC API call which is supported by the `producer_api_plugin`. This will create a snapshot file in the data/snapshots directory. Snapshot files are written to files named with the pattern `*snapshot-\<head_block_id_in_hex\>.bin*`. By default, snapshots are written to the data/snapshots directory relative to your nodeos data directory. See the `-d [ --data-dir ]` option.

You can also download a snapshot file from block producers or other companies. You can search for "eos snapshot" on google to find some companies that provide this service.
