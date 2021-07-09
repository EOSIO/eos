---
content_title: Private Access Control Tutorial
link_text: Private Access Control Tutorial
---

## Overview

This tutorial demonstrates how to use the *SECURITY_GROUP* feature to launch a private EOSIO network with TLS connections, create a *security group* and add/remove participants for data access control. The tutorial is divided into four parts: [Setting Up the Development Environment](#part-1-setting-up-the-development-environment), [Setting Up the Private Chain](#part-2-setting-up-the-private-chain), [Adding Participants](#part-3-adding-participants), and [Removing Participants](#part-4-removing-participants). The concepts will be presented from the context of a local multi-node testnet deployed locally.

## Before You Begin

* Get familiar with the [EOSIO Development Environment](../../02_usage/03_development-environment/index.md).
* Familiarize with the [Private Chain Access Control](index.md) feature.
* Become acquainted with the `p2p-tls-***` parameters in `nodeos` to specify TLS related credentials, which is necessary to establish peer-to-peer TLS connections between the participants nodes.
* It is assumed that the potential participants to join the private EOSIO network have already established a *chain of trust*. Consequently, each of the participant nodes must produce the following information beforehand and store it in the local `certs` directory:
  - The Certificate Authority (CA) certificates (`.pem`) files to verify the peers TLS connections. Note that these are the only CA certificates used in verification. Your local certificates will not be used.
  - The participant's own certificate (`.crt`) file to authenticate its running node(s). It should be signed by the trusted CA. It is used by other nodes in the private network.
  - The participant's private key (`.pem`) file used with its own certificate for server authorization during TLS connection.
* Download the test `eosio.secgrp` smart contract: [eosio.secgrp.abi](https://github.com/EOSIO/eos/blob/develop/unittests/test-contracts/security_group_test/eosio.secgrp.abi) and [eosio.secgrp.wasm](https://github.com/EOSIO/eos/blob/develop/unittests/test-contracts/security_group_test/eosio.secgrp.wasm) to the local `contracts` directory.
* Create an optional local folder, like `secgrp.tutorial`, and change to that directory.

## Part 1: Setting Up the Development Environment

You may skip this part if you have already created the development environment for this tutorial, including a default wallet with default development key and `genesis.json` file.

1. Start the `keosd` service with a large timeout (press `Enter` a second time to show the shell, if needed):
```sh
keosd --unlock-timeout 999999999 &
```
```console
info  2021-07-08T16:32:49.257 thread-0  wallet_plugin.cpp:38          plugin_initialize    ] initializing wallet plugin
info  2021-07-08T16:32:49.261 thread-0  wallet_api_plugin.cpp:84      plugin_startup       ] starting wallet_api_plugin
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/create
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/create_key
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/get_public_keys
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/import_key
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/list_keys
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/list_wallets
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/lock
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/lock_all
info  2021-07-08T16:32:49.261 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/open
info  2021-07-08T16:32:49.262 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/remove_key
info  2021-07-08T16:32:49.262 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/set_timeout
info  2021-07-08T16:32:49.262 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/sign_digest
info  2021-07-08T16:32:49.262 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/sign_transaction
info  2021-07-08T16:32:49.262 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/wallet/unlock
info  2021-07-08T16:32:49.263 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/node/get_supported_apis
```

Note that the large `unlock-timeout` value prevents unlocking the wallet manually every 15 minutes during the tutorial. For production deployments, you may want to use the default timeout and only unlock the wallet when needed.

2. Create a default wallet (if not created yet):
```sh
cleos wallet create --to-console
```
```console
Creating wallet: default
Save password to use in the future to unlock this wallet.
Without password imported keys will not be retrievable.
"PW5Jmpquu11inPLE....................MxjVyrJH3"
```

Make sure to store your wallet password securely as you might need it later.

3. Import the default development key to your wallet (if not imported already):
```sh
cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
```
```console
imported private key for: EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
```

Note that the default development key is used to sign transactions on behalf of the `eosio` system account. For production deployments, you should create your own keys and import them into the appropriate wallet. Do NOT use the default development key for production deployments.

4. Create the default `genesis.json` file:
```sh
cat > genesis.json
```
Copy/paste the following JSON contents to the terminal, then press `Enter` and `Ctrl-D`to save:
```json
{
  "initial_timestamp": "2018-06-01T12:00:00.000",
  "initial_key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
  "initial_configuration": {
    "max_block_net_usage": 1048576,
    "target_block_net_usage_pct": 1000,
    "max_transaction_net_usage": 524288,
    "base_per_transaction_net_usage": 12,
    "net_usage_leeway": 500,
    "context_free_discount_net_usage_num": 20,
    "context_free_discount_net_usage_den": 100,
    "max_block_cpu_usage": 200000,
    "target_block_cpu_usage_pct": 1000,
    "max_transaction_cpu_usage": 150000,
    "min_transaction_cpu_usage": 100,
    "max_transaction_lifetime": 3600,
    "deferred_trx_expiration_window": 600,
    "max_transaction_delay": 3888000,
    "max_inline_action_size": 524288,
    "max_inline_action_depth": 4,
    "max_authority_depth": 6
  }
}
```

## Part 2: Setting Up the Private Chain

Perform the following steps to launch the private EOSIO network and enable the *SECURITY_GROUP* feature:

1. Start the participant nodes (note the three `--p2p-tls-***` arguments used):

Launch participant `node4` (bios node):
```sh
rm -rf node_bios
mkdir node_bios
nodeos  -e  -p eosio  --http-server-address 127.0.0.1:8788  --p2p-listen-endpoint 127.0.0.1:9776  --http-validate-host false  --max-transaction-time -1  --contracts-console  --genesis-json genesis.json  --blocks-dir node_bios/blocks  --config-dir node_bios/config --data-dir node_bios/data  --p2p-tls-own-certificate-file certs/node4.crt  --p2p-tls-private-key-file certs/node4_key.pem  --p2p-tls-security-group-ca-file certs/CA_cert.pem  --plugin eosio::producer_plugin  --plugin eosio::net_plugin  --plugin eosio::chain_api_plugin  --plugin eosio::history_api_plugin  2>>node_bios/stderr &
```

Make sure that `node4` (bios node) is running:
```sh
cat node_bios/stderr
```
```console
info  2021-07-09T12:53:50.873 thread-0  chain_plugin.cpp:715          plugin_initialize    ] initializing chain plugin
...
info  2021-07-09T12:53:50.877 thread-0  chain_plugin.cpp:522          operator()           ] Support for builtin protocol feature 'SECURITY_GROUP' (with digest of 'ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02') is enabled with preactivation required
info  2021-07-09T12:53:50.877 thread-0  chain_plugin.cpp:632          operator()           ] Saved default specification for builtin protocol feature 'SECURITY_GROUP' (with digest of 'ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02') to: ./node_bios/config/protocol_features/BUILTIN-SECURITY_GROUP.json
...
info  2021-07-09T12:53:50.882 thread-0  chain_plugin.cpp:535          operator()           ] Support for builtin protocol feature 'PREACTIVATE_FEATURE' (with digest of '0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd') is enabled without activation restrictions
info  2021-07-09T12:53:50.882 thread-0  chain_plugin.cpp:632          operator()           ] Saved default specification for builtin protocol feature 'PREACTIVATE_FEATURE' (with digest of '0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd') to: ./node_bios/config/protocol_features/BUILTIN-PREACTIVATE_FEATURE.json
info  2021-07-09T12:53:50.892 thread-0  chain_plugin.cpp:1075         plugin_initialize    ] Using genesis state provided in './genesis.json'
info  2021-07-09T12:53:50.892 thread-0  chain_plugin.cpp:1106         plugin_initialize    ] Starting fresh blockchain state using provided genesis state.
info  2021-07-09T12:53:51.353 thread-0  platform_timer_accurac:62     compute_and_print_ti ] Checktime timer accuracy: min:3us max:105us mean:26us stddev:20us
info  2021-07-09T12:53:51.362 thread-0  producer_plugin.cpp:936       plugin_initialize    ] Subjective CPU billing disabled
info  2021-07-09T12:53:51.365 thread-0  http_plugin.cpp:787           plugin_initialize    ] configured http to listen on 127.0.0.1:8788
info  2021-07-09T12:53:51.366 thread-0  resource_monitor_plugi:67     plugin_initialize    ] Monitoring interval set to 2
info  2021-07-09T12:53:51.366 thread-0  resource_monitor_plugi:73     plugin_initialize    ] Space usage threshold set to 90
info  2021-07-09T12:53:51.366 thread-0  resource_monitor_plugi:82     plugin_initialize    ] Shutdown flag when threshold exceeded set to true
info  2021-07-09T12:53:51.366 thread-0  resource_monitor_plugi:89     plugin_initialize    ] Warning interval set to 30
info  2021-07-09T12:53:51.366 thread-0  main.cpp:138                  main                 ] nodeos version v2.1.0-rc1 v2.1.0-rc1-36e3c009879190dd1ff6113b6cbbe734da2d4564
info  2021-07-09T12:53:51.367 thread-0  main.cpp:139                  main                 ] nodeos using configuration file ./node_bios/config/config.ini
info  2021-07-09T12:53:51.367 thread-0  main.cpp:140                  main                 ] nodeos data directory is ./node_bios/data
warn  2021-07-09T12:53:51.367 thread-0  controller.cpp:538            startup              ] No existing chain state or fork database. Initializing fresh blockchain state and resetting fork database.
warn  2021-07-09T12:53:51.368 thread-0  controller.cpp:381            initialize_blockchai ] Initializing new blockchain with genesis state
info  2021-07-09T12:53:51.380 thread-0  combined_database.cpp:248     check_backing_store_ ] using chainbase for backing store
info  2021-07-09T12:53:51.380 thread-0  controller.cpp:456            replay               ] no irreversible blocks need to be replayed
info  2021-07-09T12:53:51.380 thread-0  controller.cpp:469            replay               ] 0 reversible blocks replayed
info  2021-07-09T12:53:51.380 thread-0  controller.cpp:479            replay               ] replayed 0 blocks in 0 seconds, 0.00000000003073364 ms/block
info  2021-07-09T12:53:51.381 thread-0  chain_plugin.cpp:1328         plugin_startup       ] starting chain in read/write mode
info  2021-07-09T12:53:51.381 thread-0  chain_plugin.cpp:1333         plugin_startup       ] Blockchain started; head block is #1, genesis timestamp is 2018-06-01T12:00:00.000
info  2021-07-09T12:53:51.381 thread-0  producer_plugin.cpp:1025      plugin_startup       ] producer plugin:  plugin_startup() begin
info  2021-07-09T12:53:51.383 thread-0  producer_plugin.cpp:1050      plugin_startup       ] Launching block production for 1 producers at 2021-07-09T12:53:51.383.
info  2021-07-09T12:53:51.390 thread-0  producer_plugin.cpp:1061      plugin_startup       ] producer plugin:  plugin_startup() end
info  2021-07-09T12:53:51.390 thread-0  net_plugin.cpp:4138           plugin_startup       ] my node_id is acd57a947afba259a34c331454f17c2d559fafe0d6490a391f9380f6830612bd
info  2021-07-09T12:53:51.391 thread-0  net_plugin.cpp:4193           plugin_startup       ] starting listener, max clients is 25
info  2021-07-09T12:53:51.392 thread-0  chain_api_plugin.cpp:98       plugin_startup       ] starting chain_api_plugin
info  2021-07-09T12:53:51.392 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/chain/get_info
...
info  2021-07-09T12:53:51.395 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/chain/send_transaction
info  2021-07-09T12:53:51.395 thread-0  history_api_plugin.cpp:34     plugin_startup       ] starting history_api_plugin
info  2021-07-09T12:53:51.395 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/history/get_actions
...
info  2021-07-09T12:53:51.395 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/history/get_transaction
info  2021-07-09T12:53:51.395 thread-0  resource_monitor_plugi:94     plugin_startup       ] Creating and starting monitor thread
info  2021-07-09T12:53:51.395 thread-0  file_space_handler.hpp:112    add_file_system      ] ./node_bios/data/node_bios/blocks's file system monitored. shutdown_available: 49996317080, capacity: 499963170816, threshold: 90
info  2021-07-09T12:53:51.396 thread-0  http_plugin.cpp:864           operator()           ] start listening for http requests
info  2021-07-09T12:53:51.397 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/node/get_supported_apis
info  2021-07-09T12:53:51.401 thread-0  producer_plugin.cpp:2420      produce_block        ] Produced block 4ba1feaf45880b6d... #2 @ 2021-07-09T12:53:51.500 signed by eosio [trxs: 0, lib: 1, confirmed: 0]
info  2021-07-09T12:53:51.906 thread-0  producer_plugin.cpp:2420      produce_block        ] Produced block 4d3eb587ff14fe75... #3 @ 2021-07-09T12:53:52.000 signed by eosio [trxs: 0, lib: 2, confirmed: 0]
info  2021-07-09T12:53:52.401 thread-0  producer_plugin.cpp:2420      produce_block        ] Produced block 90e82a32b7add06a... #4 @ 2021-07-09T12:53:52.500 signed by eosio [trxs: 0, lib: 3, confirmed: 0]
...
```

Launch participant `node1` and attempt secure connection with `node4` (bios node):
```sh
rm -rf node_01
mkdir node_01
nodeos  -p producer.one  --http-server-address 127.0.0.1:8888  --p2p-listen-endpoint 127.0.0.1:9876  --http-validate-host false  --max-transaction-time -1  --contracts-console  --genesis-json genesis.json  --blocks-dir node_01/blocks  --config-dir node_01/config --data-dir node_01/data  --p2p-tls-own-certificate-file certs/node1.crt  --p2p-tls-private-key-file certs/node1_key.pem  --p2p-tls-security-group-ca-file certs/CA_cert.pem  --plugin eosio::producer_plugin  --plugin eosio::net_plugin  --plugin eosio::chain_api_plugin  --plugin eosio::history_api_plugin  --p2p-peer-address localhost:9776  2>>node_01/stderr &
```

Make sure that `node1` is running and connected securely to `node4` (bios node):
```sh
cat node_01/stderr
```
```console
info  2021-07-09T19:46:38.634 thread-0  chain_plugin.cpp:715          plugin_initialize    ] initializing chain plugin
...
info  2021-07-09T19:46:38.634 thread-0  chain_plugin.cpp:522          operator()           ] Support for builtin protocol feature 'SECURITY_GROUP' (with digest of 'ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02') is enabled with preactivation required
info  2021-07-09T19:46:38.635 thread-0  chain_plugin.cpp:632          operator()           ] Saved default specification for builtin protocol feature 'SECURITY_GROUP' (with digest of 'ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02') to: ./node_01/config/protocol_features/BUILTIN-SECURITY_GROUP.json
...
info  2021-07-09T19:46:38.640 thread-0  chain_plugin.cpp:535          operator()           ] Support for builtin protocol feature 'PREACTIVATE_FEATURE' (with digest of '0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd') is enabled without activation restrictions
info  2021-07-09T19:46:38.641 thread-0  chain_plugin.cpp:632          operator()           ] Saved default specification for builtin protocol feature 'PREACTIVATE_FEATURE' (with digest of '0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd') to: ./node_01/config/protocol_features/BUILTIN-PREACTIVATE_FEATURE.json
info  2021-07-09T19:46:38.642 thread-0  chain_plugin.cpp:1075         plugin_initialize    ] Using genesis state provided in './genesis.json'
info  2021-07-09T19:46:38.642 thread-0  chain_plugin.cpp:1106         plugin_initialize    ] Starting fresh blockchain state using provided genesis state.
...
info  2021-07-09T19:46:39.097 thread-0  http_plugin.cpp:787           plugin_initialize    ] configured http to listen on 127.0.0.1:8888
...
info  2021-07-09T19:46:39.097 thread-0  main.cpp:139                  main                 ] nodeos using configuration file ./node_01/config/config.ini
info  2021-07-09T19:46:39.097 thread-0  main.cpp:140                  main                 ] nodeos data directory is ./node_01/data
warn  2021-07-09T19:46:39.097 thread-0  controller.cpp:538            startup              ] No existing chain state or fork database. Initializing fresh blockchain state and resetting fork database.
warn  2021-07-09T19:46:39.097 thread-0  controller.cpp:381            initialize_blockchai ] Initializing new blockchain with genesis state
info  2021-07-09T19:46:39.102 thread-0  combined_database.cpp:248     check_backing_store_ ] using chainbase for backing store
info  2021-07-09T19:46:39.102 thread-0  controller.cpp:456            replay               ] no irreversible blocks need to be replayed
...
info  2021-07-09T19:46:39.103 thread-0  chain_plugin.cpp:1333         plugin_startup       ] Blockchain started; head block is #1, genesis timestamp is 2018-06-01T12:00:00.000
info  2021-07-09T19:46:39.103 thread-0  producer_plugin.cpp:1025      plugin_startup       ] producer plugin:  plugin_startup() begin
info  2021-07-09T19:46:39.103 thread-0  producer_plugin.cpp:1050      plugin_startup       ] Launching block production for 1 producers at 2021-07-09T19:46:39.103.
info  2021-07-09T19:46:39.103 thread-0  producer_plugin.cpp:1061      plugin_startup       ] producer plugin:  plugin_startup() end
info  2021-07-09T19:46:39.103 thread-0  net_plugin.cpp:4138           plugin_startup       ] my node_id is 7adb7266daa1f18d3d56cf120a93e64bd20d652920b21f97c732c09ae1d483ec
info  2021-07-09T19:46:39.103 thread-0  net_plugin.cpp:4193           plugin_startup       ] starting listener, max clients is 25
info  2021-07-09T19:46:39.104 thread-0  net_plugin.cpp:1059           connection           ] created connection to localhost:9776
info  2021-07-09T19:46:39.104 thread-0  chain_api_plugin.cpp:98       plugin_startup       ] starting chain_api_plugin
info  2021-07-09T19:46:39.104 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/chain/get_info
...
info  2021-07-09T19:46:39.106 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/chain/send_transaction
info  2021-07-09T19:46:39.106 thread-0  history_api_plugin.cpp:34     plugin_startup       ] starting history_api_plugin
info  2021-07-09T19:46:39.106 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/history/get_actions
...
info  2021-07-09T19:46:39.106 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/history/get_transaction
info  2021-07-09T19:46:39.106 thread-0  resource_monitor_plugi:94     plugin_startup       ] Creating and starting monitor thread
info  2021-07-09T19:46:39.106 thread-0  file_space_handler.hpp:112    add_file_system      ] ./node_01/data/node_01/blocks's file system monitored. shutdown_available: 49996317080, capacity: 499963170816, threshold: 90
info  2021-07-09T19:46:39.107 thread-0  http_plugin.cpp:864           operator()           ] start listening for http requests
info  2021-07-09T19:46:39.107 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/node/get_supported_apis
info  2021-07-09T19:46:39.116 thread-1  net_plugin.cpp:1332           operator()           ] Sending handshake generation 1 to localhost:9776, lib 1, head 1, id 7c8ae6fcc236a125
info  2021-07-09T19:46:39.128 thread-2  net_plugin.cpp:1826           set_state            ] old state in sync becoming lib catchup
info  2021-07-09T19:46:39.129 thread-2  net_plugin.cpp:1993           start_sync           ] Catching up with chain, our last req is 0, theirs is 222 peer localhost:9776
info  2021-07-09T19:46:39.129 thread-2  net_plugin.cpp:1939           operator()           ] requesting range 2 to 101, from localhost:9776
info  2021-07-09T19:46:39.129 thread-2  net_plugin.cpp:2053           recv_handshake       ] handshake from 127.0.0.1:9776 - fe12cb5, lib 222, head 224, head id cbad44e284c5824d.. sync 1
info  2021-07-09T19:46:39.129 thread-2  net_plugin.cpp:1332           operator()           ] Sending handshake generation 2 to 127.0.0.1:9776 - fe12cb5, lib 1, head 1, id 7c8ae6fcc236a125
info  2021-07-09T19:46:39.130 thread-2  net_plugin.cpp:1993           start_sync           ] Catching up with chain, our last req is 101, theirs is 222 peer 127.0.0.1:9776 - fe12cb5
info  2021-07-09T19:46:39.130 thread-2  net_plugin.cpp:1860           request_next_chunk   ] ignoring request, head is 1 last req = 101 source is 127.0.0.1:9776 - fe12cb5
info  2021-07-09T19:46:39.136 thread-0  producer_plugin.cpp:428       on_incoming_block    ] Received block 34e653647c331b7f... #2 @ 2021-07-09T19:44:48.000 signed by eosio [trxs: 0, lib: 1, conf: 0, latency: 111136 ms]
info  2021-07-09T19:46:39.136 thread-0  producer_plugin.cpp:428       on_incoming_block    ] Received block e5c3605390c5c643... #3 @ 2021-07-09T19:44:48.500 signed by eosio [trxs: 0, lib: 2, conf: 0, latency: 110636 ms]
...
```

Note that if a TLS connection cannot be established with another node, the `nodeos` instance will log an error message similar to the following:
```console
error 2021-07-09T19:48:56.126 thread-1  net_plugin.cpp:1142           operator()           ] ssl handshake error: Connection reset by peer
...
info  2021-07-09T19:48:56.126 thread-1  net_plugin.cpp:1205           _close               ] closing 'localhost:9776', localhost:9776
```

Launch participant `node2` and attempt secure connection with `node4` (bios node) and `node1`:
```sh
rm -rf node_02
mkdir node_02
nodeos  -p producer.two  --http-server-address 127.0.0.1:8889  --p2p-listen-endpoint 127.0.0.1:9877  --http-validate-host false  --max-transaction-time -1  --contracts-console  --genesis-json genesis.json  --blocks-dir node_02/blocks  --config-dir node_02/config --data-dir node_02/data  --p2p-tls-own-certificate-file certs/node2.crt  --p2p-tls-private-key-file certs/node2_key.pem  --p2p-tls-security-group-ca-file certs/CA_cert.pem  --plugin eosio::producer_plugin  --plugin eosio::net_plugin  --plugin eosio::chain_api_plugin  --plugin eosio::history_api_plugin  --p2p-peer-address localhost:9776  --p2p-peer-address localhost:9876  2>>node_02/stderr &
```

Make sure that `node2` is running and connected securely to `node4` (bios node) and `node1`:
```sh
cat node_02/stderr
```
```console
info  2021-07-09T19:48:55.644 thread-0  chain_plugin.cpp:715          plugin_initialize    ] initializing chain plugin
...
info  2021-07-09T19:48:55.644 thread-0  chain_plugin.cpp:522          operator()           ] Support for builtin protocol feature 'SECURITY_GROUP' (with digest of 'ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02') is enabled with preactivation required
info  2021-07-09T19:48:55.645 thread-0  chain_plugin.cpp:632          operator()           ] Saved default specification for builtin protocol feature 'SECURITY_GROUP' (with digest of 'ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02') to: ./node_02/config/protocol_features/BUILTIN-SECURITY_GROUP.json
...
info  2021-07-09T19:48:55.650 thread-0  chain_plugin.cpp:535          operator()           ] Support for builtin protocol feature 'PREACTIVATE_FEATURE' (with digest of '0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd') is enabled without activation restrictions
info  2021-07-09T19:48:55.650 thread-0  chain_plugin.cpp:632          operator()           ] Saved default specification for builtin protocol feature 'PREACTIVATE_FEATURE' (with digest of '0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd') to: ./node_02/config/protocol_features/BUILTIN-PREACTIVATE_FEATURE.json
info  2021-07-09T19:48:55.651 thread-0  chain_plugin.cpp:1075         plugin_initialize    ] Using genesis state provided in './genesis.json'
info  2021-07-09T19:48:55.651 thread-0  chain_plugin.cpp:1106         plugin_initialize    ] Starting fresh blockchain state using provided genesis state.
...
info  2021-07-09T19:48:56.115 thread-0  http_plugin.cpp:787           plugin_initialize    ] configured http to listen on 127.0.0.1:8889
...
info  2021-07-09T19:48:56.116 thread-0  main.cpp:139                  main                 ] nodeos using configuration file ./node_02/config/config.ini
info  2021-07-09T19:48:56.116 thread-0  main.cpp:140                  main                 ] nodeos data directory is ./node_02/data
warn  2021-07-09T19:48:56.116 thread-0  controller.cpp:538            startup              ] No existing chain state or fork database. Initializing fresh blockchain state and resetting fork database.
warn  2021-07-09T19:48:56.116 thread-0  controller.cpp:381            initialize_blockchai ] Initializing new blockchain with genesis state
info  2021-07-09T19:48:56.122 thread-0  combined_database.cpp:248     check_backing_store_ ] using chainbase for backing store
info  2021-07-09T19:48:56.122 thread-0  controller.cpp:456            replay               ] no irreversible blocks need to be replayed
...
info  2021-07-09T19:48:56.122 thread-0  chain_plugin.cpp:1333         plugin_startup       ] Blockchain started; head block is #1, genesis timestamp is 2018-06-01T12:00:00.000
info  2021-07-09T19:48:56.122 thread-0  producer_plugin.cpp:1025      plugin_startup       ] producer plugin:  plugin_startup() begin
info  2021-07-09T19:48:56.122 thread-0  producer_plugin.cpp:1050      plugin_startup       ] Launching block production for 1 producers at 2021-07-09T19:48:56.122.
info  2021-07-09T19:48:56.122 thread-0  producer_plugin.cpp:1061      plugin_startup       ] producer plugin:  plugin_startup() end
info  2021-07-09T19:48:56.122 thread-0  net_plugin.cpp:4138           plugin_startup       ] my node_id is 65eebc0bcd0aa3604f5a527da746176682d6553453a2718e49ee260e760c179c
info  2021-07-09T19:48:56.123 thread-0  net_plugin.cpp:4193           plugin_startup       ] starting listener, max clients is 25
info  2021-07-09T19:48:56.124 thread-0  net_plugin.cpp:1059           connection           ] created connection to localhost:9776
...
info  2021-07-09T19:48:56.124 thread-0  chain_api_plugin.cpp:98       plugin_startup       ] starting chain_api_plugin
info  2021-07-09T19:48:56.125 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/chain/get_info
...
info  2021-07-09T19:48:56.127 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/chain/send_transaction
info  2021-07-09T19:48:56.127 thread-0  history_api_plugin.cpp:34     plugin_startup       ] starting history_api_plugin
info  2021-07-09T19:48:56.127 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/history/get_actions
...
info  2021-07-09T19:48:56.127 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/history/get_transaction
info  2021-07-09T19:48:56.127 thread-0  resource_monitor_plugi:94     plugin_startup       ] Creating and starting monitor thread
info  2021-07-09T19:48:56.127 thread-0  file_space_handler.hpp:112    add_file_system      ] ./node_02/data/node_02/blocks's file system monitored. shutdown_available: 49996317080, capacity: 499963170816, threshold: 90
info  2021-07-09T19:48:56.128 thread-0  http_plugin.cpp:864           operator()           ] start listening for http requests
info  2021-07-09T19:48:56.128 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/node/get_supported_apis
info  2021-07-09T19:48:56.130 thread-2  net_plugin.cpp:1332           operator()           ] Sending handshake generation 1 to localhost:9876, lib 1, head 1, id 7c8ae6fcc236a125
info  2021-07-09T19:48:56.131 thread-2  net_plugin.cpp:1826           set_state            ] old state in sync becoming lib catchup
info  2021-07-09T19:48:56.131 thread-2  net_plugin.cpp:1993           start_sync           ] Catching up with chain, our last req is 0, theirs is 497 peer localhost:9876
info  2021-07-09T19:48:56.131 thread-2  net_plugin.cpp:1939           operator()           ] requesting range 2 to 101, from localhost:9876
info  2021-07-09T19:48:56.131 thread-2  net_plugin.cpp:2053           recv_handshake       ] handshake from 127.0.0.1:9876 - 7adb726, lib 497, head 498, head id 2bff313101f8347d.. sync 1
info  2021-07-09T19:48:56.131 thread-2  net_plugin.cpp:1332           operator()           ] Sending handshake generation 2 to 127.0.0.1:9876 - 7adb726, lib 1, head 1, id 7c8ae6fcc236a125
info  2021-07-09T19:48:56.132 thread-1  net_plugin.cpp:1993           start_sync           ] Catching up with chain, our last req is 101, theirs is 497 peer 127.0.0.1:9876 - 7adb726
info  2021-07-09T19:48:56.132 thread-1  net_plugin.cpp:1860           request_next_chunk   ] ignoring request, head is 1 last req = 101 source is 127.0.0.1:9876 - 7adb726
info  2021-07-09T19:48:56.133 thread-0  producer_plugin.cpp:428       on_incoming_block    ] Received block 34e653647c331b7f... #2 @ 2021-07-09T19:44:48.000 signed by eosio [trxs: 0, lib: 1, conf: 0, latency: 248133 ms]
info  2021-07-09T19:48:56.133 thread-0  producer_plugin.cpp:428       on_incoming_block    ] Received block e5c3605390c5c643... #3 @ 2021-07-09T19:44:48.500 signed by eosio [trxs: 0, lib: 2, conf: 0, latency: 247633 ms]
...
```

Launch participant `node3` and attempt secure connection with `node4` (bios node), `node1`, and `node2`:
```sh
rm -rf node_03
mkdir node_03
nodeos  -p produc.three  --http-server-address 127.0.0.1:8890  --p2p-listen-endpoint 127.0.0.1:9878  --http-validate-host false  --max-transaction-time -1  --contracts-console  --genesis-json genesis.json  --blocks-dir node_03/blocks  --config-dir node_03/config --data-dir node_03/data  --p2p-tls-own-certificate-file certs/node3.crt  --p2p-tls-private-key-file certs/node3_key.pem  --p2p-tls-security-group-ca-file certs/CA_cert.pem  --plugin eosio::producer_plugin  --plugin eosio::net_plugin  --plugin eosio::chain_api_plugin  --plugin eosio::history_api_plugin  --p2p-peer-address localhost:9776  --p2p-peer-address localhost:9876  --p2p-peer-address localhost:9877  2>>node_03/stderr &
```

Make sure that `node3` is running and connected securely to `node4` (bios node), `node1`, and `node2`:
```sh
cat node_03/stderr
```
```console
info  2021-07-09T19:55:00.705 thread-0  chain_plugin.cpp:715          plugin_initialize    ] initializing chain plugin
...
info  2021-07-09T19:55:00.706 thread-0  chain_plugin.cpp:522          operator()           ] Support for builtin protocol feature 'SECURITY_GROUP' (with digest of 'ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02') is enabled with preactivation required
info  2021-07-09T19:55:00.707 thread-0  chain_plugin.cpp:632          operator()           ] Saved default specification for builtin protocol feature 'SECURITY_GROUP' (with digest of 'ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02') to: ./node_03/config/protocol_features/BUILTIN-SECURITY_GROUP.json
...
info  2021-07-09T19:55:00.714 thread-0  chain_plugin.cpp:535          operator()           ] Support for builtin protocol feature 'PREACTIVATE_FEATURE' (with digest of '0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd') is enabled without activation restrictions
info  2021-07-09T19:55:00.714 thread-0  chain_plugin.cpp:632          operator()           ] Saved default specification for builtin protocol feature 'PREACTIVATE_FEATURE' (with digest of '0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd') to: ./node_03/config/protocol_features/BUILTIN-PREACTIVATE_FEATURE.json
info  2021-07-09T19:55:00.715 thread-0  chain_plugin.cpp:1075         plugin_initialize    ] Using genesis state provided in './genesis.json'
info  2021-07-09T19:55:00.715 thread-0  chain_plugin.cpp:1106         plugin_initialize    ] Starting fresh blockchain state using provided genesis state.
...
info  2021-07-09T19:55:01.199 thread-0  http_plugin.cpp:787           plugin_initialize    ] configured http to listen on 127.0.0.1:8890
...
info  2021-07-09T19:55:01.200 thread-0  main.cpp:139                  main                 ] nodeos using configuration file ./node_03/config/config.ini
info  2021-07-09T19:55:01.200 thread-0  main.cpp:140                  main                 ] nodeos data directory is ./node_03/data
warn  2021-07-09T19:55:01.200 thread-0  controller.cpp:538            startup              ] No existing chain state or fork database. Initializing fresh blockchain state and resetting fork database.
warn  2021-07-09T19:55:01.200 thread-0  controller.cpp:381            initialize_blockchai ] Initializing new blockchain with genesis state
info  2021-07-09T19:55:01.209 thread-0  combined_database.cpp:248     check_backing_store_ ] using chainbase for backing store
info  2021-07-09T19:55:01.209 thread-0  controller.cpp:456            replay               ] no irreversible blocks need to be replayed
...
info  2021-07-09T19:55:01.209 thread-0  chain_plugin.cpp:1333         plugin_startup       ] Blockchain started; head block is #1, genesis timestamp is 2018-06-01T12:00:00.000
info  2021-07-09T19:55:01.210 thread-0  producer_plugin.cpp:1025      plugin_startup       ] producer plugin:  plugin_startup() begin
info  2021-07-09T19:55:01.210 thread-0  producer_plugin.cpp:1050      plugin_startup       ] Launching block production for 1 producers at 2021-07-09T19:55:01.210.
info  2021-07-09T19:55:01.210 thread-0  producer_plugin.cpp:1061      plugin_startup       ] producer plugin:  plugin_startup() end
info  2021-07-09T19:55:01.210 thread-0  net_plugin.cpp:4138           plugin_startup       ] my node_id is eeed0ab5f44859e68723529e280c017f90bc82e1582750346ea628834a3ce073
info  2021-07-09T19:55:01.210 thread-0  net_plugin.cpp:4193           plugin_startup       ] starting listener, max clients is 25
info  2021-07-09T19:55:01.211 thread-0  net_plugin.cpp:1059           connection           ] created connection to localhost:9776
...
info  2021-07-09T19:55:01.212 thread-0  chain_api_plugin.cpp:98       plugin_startup       ] starting chain_api_plugin
info  2021-07-09T19:55:01.212 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/chain/get_info
...
info  2021-07-09T19:55:01.214 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/chain/send_transaction
info  2021-07-09T19:55:01.214 thread-0  history_api_plugin.cpp:34     plugin_startup       ] starting history_api_plugin
info  2021-07-09T19:55:01.214 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/history/get_actions
...
info  2021-07-09T19:55:01.215 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/history/get_transaction
info  2021-07-09T19:55:01.215 thread-0  resource_monitor_plugi:94     plugin_startup       ] Creating and starting monitor thread
info  2021-07-09T19:55:01.215 thread-0  file_space_handler.hpp:112    add_file_system      ] ./node_03/data/node_03/blocks's file system monitored. shutdown_available: 49996317080, capacity: 499963170816, threshold: 90
info  2021-07-09T19:55:01.216 thread-0  http_plugin.cpp:864           operator()           ] start listening for http requests
info  2021-07-09T19:55:01.216 thread-0  http_plugin.cpp:967           add_handler          ] add api url: /v1/node/get_supported_apis
info  2021-07-09T19:55:01.218 thread-1  net_plugin.cpp:1332           operator()           ] Sending handshake generation 1 to localhost:9877, lib 1, head 1, id 7c8ae6fcc236a125
info  2021-07-09T19:55:01.219 thread-1  net_plugin.cpp:1826           set_state            ] old state in sync becoming lib catchup
info  2021-07-09T19:55:01.219 thread-1  net_plugin.cpp:1993           start_sync           ] Catching up with chain, our last req is 0, theirs is 1227 peer localhost:9877
info  2021-07-09T19:55:01.219 thread-1  net_plugin.cpp:1939           operator()           ] requesting range 2 to 101, from localhost:9877
info  2021-07-09T19:55:01.219 thread-1  net_plugin.cpp:2053           recv_handshake       ] handshake from 127.0.0.1:9877 - 65eebc0, lib 1227, head 1228, head id 3caa814475b32e5c.. sync 1
info  2021-07-09T19:55:01.219 thread-1  net_plugin.cpp:1332           operator()           ] Sending handshake generation 2 to 127.0.0.1:9877 - 65eebc0, lib 1, head 1, id 7c8ae6fcc236a125
info  2021-07-09T19:55:01.220 thread-2  net_plugin.cpp:1993           start_sync           ] Catching up with chain, our last req is 101, theirs is 1227 peer 127.0.0.1:9877 - 65eebc0
info  2021-07-09T19:55:01.220 thread-2  net_plugin.cpp:1860           request_next_chunk   ] ignoring request, head is 1 last req = 101 source is 127.0.0.1:9877 - 65eebc0
info  2021-07-09T19:55:01.423 thread-0  producer_plugin.cpp:428       on_incoming_block    ] Received block 37d5b6886019a9f8... #2 @ 2021-07-09T19:50:01.500 signed by eosio [trxs: 0, lib: 1, conf: 0, latency: 299923 ms]
info  2021-07-09T19:55:01.424 thread-0  producer_plugin.cpp:428       on_incoming_block    ] Received block efa1ea3184baf246... #3 @ 2021-07-09T19:50:02.000 signed by eosio [trxs: 0, lib: 2, conf: 0, latency: 299424 ms]
...
```

At this point, all nodes `node4` (bios node), `node1`, `node2`, and `node3` should be communicating securely using TLS. This is evident from the logs which show successful handshakes and nodes 1 to 3 receiving the first blocks from bios node 4. Note, however, that only the bios node is producing so far.

[[info | Note]]
| To create a fully operational private network with multiple producing nodes, you need to synchronize the generation of the certificate files among all producer participants according to a mutually established "chain of trust", then pass the certificate files at the time each node is launched using the `p2p-tls-***` parameters and perform a similar procedure to the [bios-boot-tutorial.py](https://github.com/EOSIO/eos/blob/develop/tutorials/bios-boot-tutorial/bios-boot-tutorial.py) script from the [Bios Boot Sequence](https://developers.eos.io/welcome/latest/tutorials/bios-boot-sequence) tutorial.

2. Activate required EOSIO protocol features:

First, check which protocol features are supported:
```sh
curl -X POST http://localhost:8788/v1/producer/get_supported_protocol_features -d '{"exclude_disabled": false, "exclude_unactivatable": false}'
```
```json
[{"feature_digest": "0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd", "subjective_restrictions": {"enabled": true, "preactivation_required": false, "earliest_allowed_activation_time": "1970-01-01T00:00:00.000"}, "description_digest": "64fe7df32e9b86be2b296b3f81dfd527f84e82b98e363bc97e40bc7a83733310", "dependencies": [], "protocol_feature_type": "builtin", "specification": [{"name": "builtin_feature_codename", "value": "PREACTIVATE_FEATURE"}]},
...
{"feature_digest": "ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02", "subjective_restrictions": {"enabled": true, "preactivation_required": true, "earliest_allowed_activation_time": "1970-01-01T00:00:00.000"}, "description_digest": "72ec6337e369cbb33ef7716d3267db9d5678fe54555c25ca4c9f5b9dfb7739f3", "dependencies": [], "protocol_feature_type": "builtin", "specification": [{"name": "builtin_feature_codename", "value": "SECURITY_GROUP"}]},
...
{"feature_digest": "f0af56d2c5a48d60a4a5b5c903edfb7db3a736a94ed589d0b797df33ff9d3e1d", "subjective_restrictions": {"enabled": true, "preactivation_required": true, "earliest_allowed_activation_time": "1970-01-01T00:00:00.000"}, "description_digest": "1eab748b95a2e6f4d7cb42065bdee5566af8efddf01a55a0a8d831b823f8828a", "dependencies": [], "protocol_feature_type": "builtin", "specification": [{"name": "builtin_feature_codename", "value": "GET_SENDER"}]}]
```

Activate the `PREACTIVATE_FEATURE` feature:
```sh
curl -X POST http://localhost:8788/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}'
```
```json
{"result": "ok"}
```

Activate the supported `SECURITY_GROUP` feature:
```sh
curl -X POST http://localhost:8888/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ["ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02"]}'
```
```json
{"result": "ok"}
```

Note that if a protocol feature has already been activated, the following response is returned, which can be ignored:
```json
{"code": 500, "message": "Internal Service Error", "error": {"code": 3250000, "name": "protocol_feature_exception", "what": "Protocol feature exception", "details": [{"message": "protocol feature with digest '<hex-value>' has already been activated", "file": "controller.cpp", "line_number": 1650, "method": "check_protocol_features"}]}}
```

Nodes begin to communicate securely only if and when their corresponding peer-to-peer TLS connections are successful, even without the `SECURITY_GROUP` protocol feature activated. However, the protocol feature activation requires the TLS flags and enforces this policy on restart or loading from a snapshot. Just after activation, nodes work literally the same way as with TLS but without the feature activation. The `SECURITY_GROUP` protocol feature, once activated, simply adds participants management. That way you can restrict individual participants access even though they have valid certificates. After adding a first participant to the *security group* the "all valid certificates are welcome" policy stops working and only those within the group will continue to receive blocks. Nodes can be added or removed from a *security group* at any time and any unlimited number of times.

## Part 3: Adding Participants

In this section you will use the test `eosio.secgrp` smart contract to add participants to the *security group*. This smart contract provides a simple wrapper interface over the [C++ security_group API](index.md#c++-security_group-api) host functions to add/remove participants and perform other housekeeping operations. First, you need to deploy the `eosio.secgrp` smart contract:

1. Deploy the `eosio.secgrp` smart contract using the `eosio` privileged account:

```sh
cleos set contract eosio contracts/ eosio.secgrp.wasm eosio.secgrp.abi
```
```console
transaction id: f7c354b80421633086bfb2d5e1c6a69c97bcc12f10ac3b5e277dcd1f21ad5fe6, status: executed, (possible) block num: 125 
```

2. Add participant nodes ["node1","node2"] from the non-participant list to the *security group*:

First, add the nodes to the participants table inside the contract:
```sh
cleos push action eosio add '[["node1","node2"]]'
```
```console
transaction id: d0a7099c95f870eaecc7d7acb4174ec69af7f9d5bf104bdf6801084357407a6d, status: executed, (possible) block num: 139 
```

Next, publish the changes by indirectly calling the `add_security_group_participant()` host function:
```sh
cleos push action eosio publish '[0]'
```
```console
transaction id: d0dba1f1ec4d214e9b2d738eaeee8ac4a406f8240733e2e1ca07ff05a8de3c7f, status: executed, (possible) block num: 145 
```

The integer on the list argument of the `publish` action represents the number of participants to publish. The `0` integer means everyone.

## Part 4: Removing Participants

1. Remove participant node ["node4"] from the participant list in the *security group*:

First, remove the node from the participants table inside the contract:
```sh
cleos push action eosio remove '[["node4"]]'
```
```console
515d17d819ab2278a62cbde8f5c073fe1277b398217e7f603aebe1c01b5a0981, status: executed, (possible) block num: 358 
```

Next, publish the changes by indirectly calling the `remove_security_group_participant()` host function:
```sh
cleos push action eosio publish '[0]'
```
```console
transaction id: 0f1803e44d920790886423ba3fcc18c80f37d64c08131d30850c605fb484153a, status: executed, (possible) block num: 365 
```

[[info | Note]]
| The test `eosio.secgrp` smart contract is used for didactic purposes. Developers are encouraged to crete their own smart contract wrappers the [C++ security_group API](index.md#c++-security_group-api) reference to best suite their application needs.

## Summary

This tutorial showcased how to use the *SECURITY_GROUP* feature to launch a private EOSIO network with TLS connections, create a *security group* and add/remove participants for finer access control. It also demonstrated how to use the `add`, `remove`, `publish` actions in the test `eosio.secgrp` smart contract.
