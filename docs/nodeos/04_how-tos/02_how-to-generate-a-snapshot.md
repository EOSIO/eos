<!-- ## How to generate a snapshot -->

You can force a running nodeos node to create a snapshot by using the `create_snapshot` RPC API call which is supported by the `producer_api_plugin`. This will create a snapshot file in the data/snapshots directory. Snapshot files are written to files named with the pattern `*snapshot-\<head_block_id_in_hex\>.bin*`. By default, snapshots are written to the data/snapshots directory relative to your nodeos data directory. See the `-d [ --data-dir ]` option.

Assuming your nodeos is running locally the below command is asking the nodeos to create a snapshot

```sh
curl --request POST \
  --url http://127.0.0.1:8888/v1/producer/create_snapshot \
  --header 'content-type: application/x-www-form-urlencoded; charset=UTF-8'
```

[[info]]
| You can also download a `blocks.log` file from third party providers.
