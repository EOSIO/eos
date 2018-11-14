# statetrack_plugin

This plugin adds hooks on chainbase operations emplace, modify, remove, and undo,
and routes the objects and revision numbers to a ZMQ socket.

It can hook to the operations that happen on the state DBs of all contracts
deployed to the chain, and it can use filters to limit the operations
that should be sent to only the ones happening on the contracts, scopes, and tables
that the user is interested in tracking, thus limiting the performance impact to a minimum.

The plugin will also send operations happening on accounts and account permissions.
It will also send a stream of applied actions.

This is a very early stage of the implementation, and more testing is required.
This should not be considered production ready yet.

## Configuration

The following configuration statements in `config.ini` are recognized:

- `plugin = eosio::statetrack_plugin` -- Enables the state track plugin
- `st-zmq-sender-bind = ENDPOINT` -- Specifies the PUSH socket connection endpoint.
  Default value: tcp://127.0.0.1:3000.
- `st-filter-on = code:scope:table` -- Track DB operations which match code:scope:table.
- `st-filter-off = code:scope:table` -- Do not track DB operations which match code:scope:table.

## Important notes

- Accounts are sent as DB operations with `(code, scope, table)` being
  `(system, system, accounts)`.
  - Disable tracking of accounts using `st-filter-off = system:system:accounts`.
- Account permissions are sent as DB operations with `(code, scope, table)`
  being `(system, system, permissions)`.
  - Disable tracking of account permissions using `st-filter-off = system:system:permissions`.
- Actions are sent as DB operations with `(code, scope, table)` being
  `(system, system, actions)`.
  - Disable tracking of actions using `st-filter-off = system:system:actions`.
- Filters are not tested yet, but the implementation is taken from
  the history plugin, so it should work fine.
- We're adding separate filters for actions to make it possible to
  filter by specific actions of specific contracts.

## Open questions

- In Chainbase, we are emitting the operations inside an undo just before the
  operation is actually executed and checked for success
  ([See here](https://github.com/EOSIO/chainbase/compare/master...mmcs85:master#diff-298563b6c76ef92100c2ea27c06cb08bR355)).
  - This is because we are not sure how to get a reference to the item after
    it is given to the `modify` and `emplace` functions using `std::move`.
  - We don't like this inconsistency since it is emitting the event before being
    sure that the operation actually took place, but we think it might not
    be an issue since those operations are undoing previous ones and it
    would not be an expected scenario for them to fail unless there was a bug
    or a database inconsistency (maybe caused by bad locking mechanisms when
    using multiple threads?).

## Building

The plugin needs to be built using the ZeroMQ C++ headers.

### Adding ZMQ library on Mac

```
brew install zmq
brew tap jmuncaster/homebrew-header-only
brew install jmuncaster/header-only/cppzmq
```

### Adding ZMQ library on Ubuntu

```
apt-get install -y pkg-config libzmq5-dev
```
