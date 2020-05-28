---
content_title: Nodeos
---

## Introduction

`nodeos` is the core service daemon that runs on every EOSIO node. It can be configured to process smart contracts, validate transactions, produce blocks containing valid transactions, and confirm blocks to record them on the blockchain.

## Installation

`nodeos` is distributed as part of the [EOSIO software suite](https://github.com/EOSIO/eos/blob/master/README.md). To install `nodeos`, visit the [EOSIO Software Installation](../00_install/index.md) section.

## Explore

Navigate the sections below to configure and use `nodeos`.

* [Usage](02_usage/index.md) - Configuring and using `nodeos`, node setups/environments.
* [Plugins](03_plugins/index.md) - Using plugins, plugin options, mandatory vs. optional.
* [Replays](04_replays/index.md) - Replaying the chain from a snapshot or a blocks.log file.
* [RPC APIs](05_rpc_apis/index.md) - Remote Procedure Call API reference for plugin HTTP endpoints.
* [Logging](06_logging/index.md) - Logging config/usage, loggers, appenders, logging levels.
* [Upgrade Guides](07_upgrade-guides/index.md) - EOSIO version/consensus upgrade guides.
* [Troubleshooting](08_troubleshooting/index.md) - Common `nodeos` troubleshooting questions.

[[info | Access Node]]
| A local or remote EOSIO access node running `nodeos` is required for a client application or smart contract to interact with the blockchain.
