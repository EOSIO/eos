[[warning | Deprecation Notice]]
| The `mongo_db_plugin` is deprecated and will no longer be maintained. Please refer to the [`state_history_plugin`](../state_history_plugin/index.md) and the [`history-tools`](../state_history_plugin/index.md#history-tools) for better options to archive blockchain data.

## Description

The optional `eosio::mongo_db_plugin` provides archiving of blockchain data into a MongoDB. It is recommended that the plugin be added to a non-producing node as it is designed to shut down on any failed insert into the MongoDB and it is resource intensive. For best results dedicate a `nodeos` instance to running this one plugin. The rationale behind this shutdown on error is so that any issues with connectivity or the mongo database can be fixed and `nodeos` can be restarted without having to resync or replay.

## Important Notes

* Documents stored in mongo by `mongo_db_plugin` which contain empty field/struct names will be stored with the field/struct name of `empty_field_name` / `empty_struct_name`.
* Action data is stored on chain as raw bytes. This plugin attempts to use associated ABI on accounts to deserialize the raw bytes into expanded `abi_def` form for storage into mongo. Note that invalid or missing ABI on a contract will result in the action data being stored as raw bytes. For example the EOSIO system contract does not provide ABI for the `onblock` action so it is stored as raw bytes.
* The `mongo_db_plugin` does slow down replay/resync as the conversion of block data to JSON and insertion into MongoDB is resource intensive. The plugin does use a worker thread for processing the block data, but this does not help much when replaying/resyncing.

## Recommendations

* It is recommended that a large `--abi-serializer-max-time-ms` value be passed into the `nodeos` running the `mongo_db_plugin` as the default ABI serializer time limit is not large enough to serialize large blocks.
* Read-only mode should be used to avoid speculative execution. See [Nodeos Read Modes](../../02_usage/05_nodeos-implementation.md#nodeos-read-modes). Forked data is still recorded (data that never becomes irreversible) but speculative transaction processing and signaling is avoided, minimizing the transaction_traces/action_traces stored.

## Options

These can be specified from both the command-line or the `config.ini` file:

```console
  -q [ --mongodb-queue-size ] arg (=256)
                                        The target queue size between nodeos
                                        and MongoDB plugin thread.
  --mongodb-abi-cache-size              The maximum size of the abi cache for
                                        serializing data.
  --mongodb-wipe                        Required with --replay-blockchain,
                                        --hard-replay-blockchain, or
                                        --delete-all-blocks to wipe mongo
                                        db.This option required to prevent
                                        accidental wipe of mongo db. 
                                        Defaults to false.
  --mongodb-block-start arg (=0)        If specified then only abi data pushed
                                        to mongodb until specified block is
                                        reached.
  -m [ --mongodb-uri ] arg              MongoDB URI connection string, see:
                                        https://docs.mongodb.com/master/referen
                                        ce/connection-string/. If not specified
                                        then plugin is disabled. Default
                                        database 'EOS' is used if not specified
                                        in URI. Example: mongodb://127.0.0.1:27
                                        017/EOS
  --mongodb-update-via-block-num arg (=0)
                                        Update blocks/block_state with latest
                                        via block number so that duplicates are
                                        overwritten.                         
  --mongodb-store-blocks                Enables storing blocks in mongodb.
                                        Defaults to true.
  --mongodb-store-block-states          Enables storing block state in mongodb.
                                        Defaults to true.
  --mongodb-store-transactions          Enables storing transactions in mongodb.
                                        Defaults to true.
  --mongodb-store-transaction-traces    Enables storing transaction traces in                                             mongodb.
                                        Defaults to true.
  --mongodb-store-action-traces         Enables storing action traces in mongodb.
                                        Defaults to true.
  --mongodb-filter-on                   Mongodb: Track actions which match
                                        receiver:action:actor. Actor may be blank
                                        to include all. Receiver and Action may
                                        not be blank. Default is * include
                                        everything.
  --mongodb-filter-out                  Mongodb: Do not track actions which match
                                        receiver:action:actor. Action and Actor
                                        both blank excludes all from reciever.                                           Actor blank excludes all from
                                        reciever:action. Receiver may not be
                                        blank.
```

## Notes

* `--mongodb-store-*` options all default to true.
* `--mongodb-filter-*` options currently only applies to the `action_traces` collection.

## Example Filters

```console
mongodb-filter-out = eosio:onblock:
mongodb-filter-out = gu2tembqgage::
mongodb-filter-out = blocktwitter:: 
```

[[warning | Warning]]
| When the `mongo_db_plugin` is turned on, the target mongodb instance may take a lot of storage space. With all collections enabled and no filters applied, the mongodb data folder can easily occupy hundreds of GBs of data. It is recommended that you tailor the options and utilize the filters as you need in order to maximize storage efficiency.

## Collections

* `accounts` - created on applied transaction. Always updated even if `mongodb-store-action-traces=false`.
  * Currently limited to just name and ABI if contract abi on account
  * Mostly for internal use as the stored ABI is used to convert action data into JSON for storage as associated actions on contract are processed.
  * Invalid ABI on account will prevent conversion of action data into JSON for storage resulting in just the action data being stored as hex. For example, the original eosio.system contract did not provide ABI for the `onblock` action and therefore all `onblock` action data is stored as hex until the time `onblock` ABI is added to the eosio.system contract.

* `action_traces` - created on applied transaction
  * `receipt` - action_trace action_receipt - see `eosio::chain::action_receipt`
  * `trx_id` - transaction id
  * `act` - action - see `eosio::chain::action`
  * `elapsed` - time in microseconds to execute action
  * `console` - console output of action. Always empty unless `contracts-console = true` option specified.

* `block_states` - created on accepted block
  * `block_num`
  * `block_id`
  * `block_header_state` - see `eosio::chain::block_header_state`
  * `validated`
  * `in_current_chain`

* `blocks` - created on accepted block
  * `block_num`
  * `block_id`
  * `block` - signed block - see `eosio::chain::signed_block`
  * `validated` - added on irreversible block
  * `in_current_chain` - added on irreversible block
  * `irreversible=true` - added on irreversible block

* `transaction_traces` - created on applied transaction
  * see `chain::eosio::transaction_trace`

* `transactions` - created on accepted transaction - does not include inline actions
  * see `eosio::chain::signed_transaction`. In addition to signed_transaction data the following are also stored.
  * `trx_id` - transaction id
  * `irreversible=true` - added on irreversible block
  * `block_id` - added on irreversble block
  * `block_num` - added on irreversible block
  * `signing_keys`
  * `accepted`
  * `implicit`
  * `scheduled`

* `account_controls` - created on applied transaction. Always updated even if `mongodb-store-action-traces=false`.
  * `controlled_account`
  * `controlling_permission`
  * `controlling_account`

The equivalent of `/v1/history/get_controlled_acounts` with mongo: `db.account_controls.find({"controlling_account":"hellozhangyi"}).pretty()`

* `pub_keys` - created on applied transaction. Always updated even if `mongodb-store-action-traces=false`.
  * `account`
  * `permission`
  * `public_key`

## Examples

The mongodb equivalent of `/v1/history/get_key_accounts` RPC API endpoint:

```console
db.pub_keys.find({"public_key":"EOS7EarnUhcyYqmdnPon8rm7mBCTnBoot6o7fE2WzjvEX2TdggbL3"}).pretty()
```

## Dependencies

* [`chain_plugin`](../chain_plugin/index.md)
* [`history_plugin`](../history_plugin/index.md)
