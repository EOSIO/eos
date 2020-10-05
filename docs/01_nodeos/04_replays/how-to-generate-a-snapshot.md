---
content_title: How to generate a snapshot
---

You can force a running `nodeos` instance to create a snapshot by using the `create_snapshot` RPC API call supported by the `producer_api_plugin`. This will create a snapshot file in the `data/snapshots` directory. Snapshot files are written to disk with the name pattern `*snapshot-\<head_block_id_in_hex\>.bin*`.

[[info | Snapshots Location]]
| By default, snapshots are written to the `data/snapshots` directory relative to your `nodeos` data directory. See the `-d [ --data-dir ]` option.

If your `nodeos` instance is running locally, the below command will request `nodeos` to create a snapshot:

```sh
curl -X POST http://127.0.0.1:8888/v1/producer/create_snapshot
```

[[info | Getting other `blocks.log` files]]
| You can also download a `blocks.log` file from third party providers.
