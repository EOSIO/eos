# Protocol

The plugin's protocol is based on WebSocket. When a client connects, the plugin sends a string message containing the protocol ABI. All communication after this point is in a binary format defined by the ABI. The client may send one or more requests. The plugin responds with either a single response, or a stream of responses.

Note: there will be an incompatible break between the alpha and release versions of the plugin. The versioning mechanism intends to provide an upgrade path between the initial release version and future releases.

## ABI

The protocol ABI is in the EOSIO ABI 1.1 JSON format, with some extensions in the `tables` section. Both `abieos` and `eosjs` can encode and decode messages using this ABI. The major ABI sections the protocol uses are:

* `structs`: this defines structures for portions of the protocol and for content carried by the protocol.
* `variants`: this defines the message selection and major versioning mechanism for the protocol. e.g. the `table_delta` variant currently has 1 alternative: `table_delta_v0`. If a future version of this part of the protocol has an incompatible structure, it will likely be in a new struct named `table_delta_v1` added to the `table_delta` variant.
* `tables`: this maps table names to the variant types the tables contain. It also identifies which fields make up each table's primary key.

The ABI uses binary extensions for minor versioning. e.g. if the table deltas structure gains a new field, it will likely be a binary extension added to the end of `table_delta_v0`.

The abi lives in `plugins/state_history_plugin/state_history_plugin_abi.cpp` within the eos repo.

## Requests and Results

A client sends a `request` variant and the plugin sends the `result` variant.

## Requesting Status

If a client sends `get_status_request_v0`, the plugin will reply with a `get_status_result_v0` containing:
* Head block_num and block_id
* Last irreversible block_num and block_id
* The range of block numbers which exist in the transaction trace log (`trace_history.log`)
* The range of block numbers which exist in the chain state log (`chain_state_history.log`)

## Requesting Data

A client may send `get_blocks_request_v0` to request a data stream. The request contains:
* `start_block_num`: the first block number requested. The plugin may start before this if needed; see [Fork Detection](#fork-detection).
* `end_block_num`: 1 past the last block number requested. The plugin stops sending data when the next block >= end_block_num.
  Set this to `0xffff'ffff` to receive all data and to follow live data as it's available.
* `max_messages_in_flight`: the maximum number of unacknowledged messages the plugin will send.
   * C++ clients: set this to `0xffff'ffff` for maximum streaming performance. No acks are necessary.
   * Javascript clients: set this to a small number to prevent overrunning buffers. The client must send 
     `get_blocks_ack_request_v0` to acknowledge receiving the messages.
* `have_positions`: see [Fork Detection](#fork-detection).
* `irreversible_only`: set to true to only receive data from irreversible blocks.
* `fetch_block`: set to true to receive blocks. The plugin fetches these from the block log.
* `fetch_traces`: set to true to receive transaction traces. The plugin fetches these from `trace_history.log`.
* `fetch_deltas`: set to true to receive state deltas. The plugin fetches these from `chain_state_history.log`.

## Receiving Data

The plugin sends a `get_blocks_result_v0` for each block. This contains:
* `head`: current head block number and id. This reflects current nodeos state; it may not directly relate to the block this result is about.
* `last_irreversible`: current irreversible block number and id. This reflects current nodeos state; it may not directly relate to the block this result is about.
* `this_block`: Identifies the block number and id this result is about, if any. May be empty if this result would have been about a block which was trimmed (block no longer in block log or *_history.log).
* `prev_block`: Identifies the block before this one, if any. May be empty if this block is the first, or the previous block is no longer available.
* `block`: blob which contains serialized `signed_block`. Sent if `fetch_block` is true and block is available in block log.
* `traces`: blob which contains serialized and gzipped `transaction_trace[]`. Sent if `fetch_traces` is true and block is available in `trace_history.log`.
* `deltas`: blob which contains serialized and gzipped `table_delta[]`. Sent if `fetch_deltas` is true and block is available in `chain_state_history.log`. See [table_delta](#table_delta).

## table_delta

## Fork Detection
