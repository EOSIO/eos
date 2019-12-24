# Nodapifiny

## Introduction

`nodapifiny` is the core service daemon that runs on every APIFINY node. It can be configured to process smart contracts, validate transactions, produce blocks with valid transactions, and confirm blocks to record them on the blockchain.

## Installation

`nodapifiny` is distributed as part of the [APIFINY software suite](https://github.com/APIFINY/apifiny/blob/master/README.md). To install `nodapifiny`, visit the [APIFINY Software Installation](../00_install/index.md) section.

## Explore

Please navigate the sections below to configure and use `nodapifiny`.

* [Usage](02_usage/index.md) - Configuring and using `nodapifiny`, node setups/environments.
* [Plugins](03_plugins/index.md) - Using plugins, plugin options, mandatory vs. optional.
* [Replays](04_replays/index.md) - Replaying the chain from a snapshot or a blocks.log file.
* [Logging](06_logging/index.md) - Logging config/usage, loggers, appenders, logging levels.
* [Upgrade Guides](07_upgrade-guides/index.md) - APIFINY version/consensus upgrade guides.
* [Troubleshooting](08_troubleshooting/index.md) - Common `nodapifiny` troubleshooting questions.

[[info | Access Node]]
| A local or remote APIFINY access node running `nodapifiny` is required for a client application or smart contract to interact with the blockchain.
