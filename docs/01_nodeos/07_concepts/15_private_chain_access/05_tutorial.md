---
content_title: Private Access Control Tutorial
link_text: Private Access Control Tutorial
---

## Overview

This tutorial demonstrates how to use the *SECURITY_GROUP* feature to launch a private EOSIO network with TLS connections, create a *security group* and add/remove participants for finer access control. The tutorial is divided into three parts: [Setting Up the Private Chain](#part-1-setting-up-the-private-chain), [Adding Participants](#part-2-adding-participants), and [Removing Participants](#part-3-removing-participants). The concepts will be illustrated from the context of a local multi-node testnet deployed locally.

## Before You Begin

* Get familiar with the [EOSIO Development Environment](../../01_nodeos/02_usage/03_development-environment/index.md).
* Make sure to [Set Up Your EOSIO Environment](https://developers.eos.io/welcome/latest/getting-started-guide/local-development-environment) for local development.
* Familiarize with the [Private Chain Access Control](index.md) feature.
* Become acquainted with the `--p2p-tls-***` parameters in `nodeos` to specify TLS related credentials, which is necessary to establish peer-to-peer TLS connections between the participants nodes.
* It is assumed that the potential participants to join the private EOSIO network have already established a *chain of trust*. Consequently, each of the participant nodes must produce the following information beforehand and store it in the local `certs` directory:
  - the Certificate Authority's certificate (`.pem`) file to verify the peers TLS connections.
  - the participant's own certificate (`.crt`) file to authenticate its running node(s).
  - the participant's private key (`.pem`) file used with its own certificate for server authorization during TLS connection handshakes.
* Download the test `eosio.secgrp` smart contract: [eosio.secgrp.abi](https://github.com/EOSIO/eos/blob/develop/unittests/test-contracts/security_group_test/eosio.secgrp.abi) and [eosio.secgrp.wasm](https://github.com/EOSIO/eos/blob/develop/unittests/test-contracts/security_group_test/eosio.secgrp.wasm) to the local `contracts` directory.

## Part 1: Setting Up the Private Chain

Perform the following steps to launch the private EOSIO network and enable the *SECURITY_GROUP• feature:

1. Start the participant nodes (note the three `--p2p-tls-***` arguments used):

Launch participant `node4` (bios node):
```sh
nodeos --max-transaction-time -1 --abi-serializer-max-time-ms 990000 --filter-on * --p2p-max-nodes-per-host 3 --contracts-console --plugin eosio::producer_api_plugin --http-max-response-time-ms 990000 --p2p-tls-own-certificate-file certs/node4.crt --p2p-tls-private-key-file certs/node4_key.pem --p2p-tls-security-group-ca-file certs/CA_cert.pem --config-dir config/node_bios --data-dir data/node_bios --genesis-json etc/eosio/node_bios/genesis.json --genesis-timestamp 2021-04-30T18:13:24.078
```

Launch participant `node1`:
```sh
nodeos --max-transaction-time -1 --abi-serializer-max-time-ms 990000 --filter-on * --p2p-max-nodes-per-host 3 --contracts-console --plugin eosio::producer_api_plugin --http-max-response-time-ms 990000 --p2p-tls-own-certificate-file certs/node1.crt --p2p-tls-private-key-file certs/node1_key.pem --p2p-tls-security-group-ca-file certs/CA_cert.pem --config-dir config/node_00 --data-dir data/node_00 --genesis-json etc/eosio/node_00/genesis.json --genesis-timestamp 2021-04-30T18:13:24.078
```

Launch participant `node2`:
```sh
nodeos --max-transaction-time -1 --abi-serializer-max-time-ms 990000 --filter-on * --p2p-max-nodes-per-host 3 --contracts-console --plugin eosio::producer_api_plugin --http-max-response-time-ms 990000 --p2p-tls-own-certificate-file certs/node2.crt --p2p-tls-private-key-file certs/node2_key.pem --p2p-tls-security-group-ca-file certs/CA_cert.pem --config-dir config/node_01 --data-dir data/node_01 --genesis-json etc/eosio/node_01/genesis.json --genesis-timestamp 2021-04-30T18:13:24.078
```

Launch participant `node3`:
```sh
nodeos --max-transaction-time -1 --abi-serializer-max-time-ms 990000 --filter-on * --p2p-max-nodes-per-host 3 --contracts-console --plugin eosio::producer_api_plugin --http-max-response-time-ms 990000 --p2p-tls-own-certificate-file certs/node3.crt --p2p-tls-private-key-file certs/node3_key.pem --p2p-tls-security-group-ca-file certs/CA_cert.pem --config-dir config/node_02 --data-dir data/node_02 --genesis-json etc/eosio/node_02/genesis.json --genesis-timestamp 2021-04-30T18:13:24.078
```

Also note that each participant's certificate file is named after its participant name. For instance, `node2.crt` stems from participant `node2`.

2. Activate required EOSIO protocol features:

First, check which protocol features are supported:
```sh
curl http://localhost:8788/v1/producer/get_supported_protocol_features -d '{"exclude_disabled": false, "exclude_unactivatable": false}' -X POST -H "Content-Type: application/json"
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
curl http://localhost:8788/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}' -X POST -H "Content-Type: application/json"
```
```json
{"result": "ok"}
```

Activate the supported `SECURITY_GROUP` feature:
```sh
curl http://localhost:8888/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ["ded2e25adcd78cbb94fa7f63a8f80a9af2b1a905e551a6e124e7d7829da1ea02"]}' -X POST -H "Content-Type: application/json"
```
```json
{"result": "ok"}
```

Note that if a protocol feature has already been activated, the following harmless response is returned:
```json
{"code": 500, "message": "Internal Service Error", "error": {"code": 3250000, "name": "protocol_feature_exception", "what": "Protocol feature exception", "details": [{"message": "protocol feature with digest '<hex-value>' has already been activated", "file": "controller.cpp", "line_number": 1650, "method": "check_protocol_features"}]}}
```

After the `SECURITY_GROUP` protocol feature has been activated, all nodes launched previously should begin exchanging data securely using TLS connections. If further control is desired on a per-participant basis, node operators can enable the data access control layer by [adding](#part-2-adding-participants) at least one participant to the *security group*.

## Part 2: Adding Participants

In this section your will use the test `eosio.secgrp` smart contract to add participants to the *security group*. This smart contract provides a simple wrapper interface over the [C++ security_group API](index.md#c++-security_group-api) intrinsics to add/remove participants and other housekeeping operations. First, you need to deploy the `eosio.secgrp` smart contract:

1. Deploy the `eosio.secgrp` smart contract using the `eosio` privileged account:

```sh
cmd: programs/cleos/cleos --url http://localhost:8888  --wallet-url http://localhost:9899 --no-auto-keosd -v set contract -j  eosio contracts/ eosio.secgrp.wasm eosio.secgrp.abi
```
```console
transaction id: f7c354b80421633086bfb2d5e1c6a69c97bcc12f10ac3b5e277dcd1f21ad5fe6, status: executed, (possible) block num: 125 
```

2. Add participant nodes ["node1","node2"] from the non-participant list to the *security group*:

First, add the nodes to the participants table inside the contract:
```sh
cleos --url http://localhost:8888 --wallet-url http://localhost:9899 --no-auto-keosd push action -j eosio add '[["node1","node2"]]' --permission eosio@active
```
```console
transaction id: d0a7099c95f870eaecc7d7acb4174ec69af7f9d5bf104bdf6801084357407a6d, status: executed, (possible) block num: 139 
```

Next, commit the changes by indirectly calling the `add_security_group_participant()` intrinsic:
```sh
cleos --url http://localhost:8888 --wallet-url http://localhost:9899 --no-auto-keosd push action -j eosio publish '[0]' --permission eosio@active
```
```console
transaction id: d0dba1f1ec4d214e9b2d738eaeee8ac4a406f8240733e2e1ca07ff05a8de3c7f, status: executed, (possible) block num: 145 
```

The `0` argument indicates that all `add` and `remove` actions that were sent to the contract should be executed. The test contract uses other numbers, just to prevent identical transactions.

## Part 3: Removing Participants

1. Remove participant node ["node4"] from the participant list in the *security group*:

First, remove the node from the participants table inside the contract:
```sh
cleos --url http://localhost:8888 --wallet-url http://localhost:9899 --no-auto-keosd push action -j eosio remove '[["node4"]]' --permission eosio@active
```
```console
515d17d819ab2278a62cbde8f5c073fe1277b398217e7f603aebe1c01b5a0981, status: executed, (possible) block num: 358 
```

Next, commit the changes by indirectly calling the `remove_security_group_participant()` intrinsic:
```sh
cleos --url http://localhost:8888 --wallet-url http://localhost:9899 --no-auto-keosd push action -j eosio publish '[0]' --permission eosio@active
```
```console
transaction id: 0f1803e44d920790886423ba3fcc18c80f37d64c08131d30850c605fb484153a, status: executed, (possible) block num: 365 
```

[[info | Note]]
| The test `eosio.secgrp` smart contract is used for didactic purposes. Developers are encouraged to crete their own smart contract wrappers the [C++ security_group API](index.md#c++-security_group-api) reference to best suite their application needs.

## Summary

This tutorial showcased how to use the *SECURITY_GROUP* feature to launch a private EOSIO network with TLS connections, create a *security group* and add/remove participants for finer access control. It also demonstrated how to use the `add`, `remove`, `publish` actions in the test `eosio.secgrp` smart contract.
