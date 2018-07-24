# zmq_plugin

This plugin is doing approximately the same as `history_plugin`, but
instead of storing history events in the shared memory database, it
pushes them outside of `nodeos` process via ZeroMQ PUSH socket.

If wildcard filter is enabled for `history_plugin`, all account history
is stored in the same shared memory segment as the blockchain state
database. This leads to its rapid growth, up to 2GB per day, and
increased risk of node crash because of exceeded memory or disk
space. The ZMQ plugin allows processing and storing the history events
elsewhere, such as RDBMS with truncation or archiving of old entries.

The PUSH type of socket is blocking, so if nobody is pulling from it,
`nodeos` will wait forever. This is done in order to prevent skipping
any action events in the blockchain. The receiver may also route the
events to non-blocking types of sockets, such as PUB socket, in order to
let other systems listen to events on the go.



## Socket message format

1. 32-bit signed integer in host native format (little-endian on most
   platforms): `msgtype=0`. Other values are reserved for future message
   types.

2. 32-bit signed integer in host native format:
   `msgopts=0`. Other values are reserved for future option codes.

3. JSON data.



## JSON data format

The JSON data is a map with the following entries:

1. `action_trace`: action and its corresponding inline actions, like in
   history_plugin RPC output.

2. `block_num`: block number.

3. `block_time`: block timestamp

4. `global_action_seq`: global running number of the action.

5. `currency_balances`: array of token balances for all accounts and all
   currency tokens involved in the action. Each entry is a map with
   `account_name`, `issuer`, and `balance` keys.

6. `resource_balances`: array of maps indicating current resources for
   every account involved in the action, with the keys and values as
   follows:

  * `account_name`: resource holder account;

  * `cpu_weight`, `net_weight`: staked CPU and network bandwidth in EOS,
    multiplied by 10000;

  * `ram_quota`: total RAM owned by the account, in bytes;

  * `ram_usage`: used RAM, in bytes.


Unlike `history_plugin`, this plugin does not deliver
`account_action_seq`, because that value is calculated internally by the
history plugin.



## Configuration

The following configuration statements in `config.ini` are recognized:

* `plugin = eosio::zmq_plugin` -- enables the ZMQ plugin

* `zmq-sender-bind = ENDPOINT` -- specifies the PUSH socket binding
  endpoint. Default value: `tcp://127.0.0.1:5556`.



## Compiling

```bash
apt-get install -y pkg-config libzmq5-dev
LOCAL_CMAKE_FLAGS='-DBUILD_ZMQ_PLUGIN=true' ./eosio_build.sh
```


## Blacklists

Action `onblock` in `eosio` account is ignored and is not producing a
ZMQ event. These actions are generated every 0.5s, and ignored in order
to save the CPU resource.

System accounts, such as `eosio` and `eosio.token` and few others are
not listed in `currency_balances` and `resource_balances`.



## Author

* cc32d9 <cc32d9@gmail.com>
* https://github.com/cc32d9
* https://medium.com/@cc32d9


