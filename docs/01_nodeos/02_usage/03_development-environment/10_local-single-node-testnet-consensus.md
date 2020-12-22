---
content_title: Local Single-Node Testnet With Consensus Protocol
link_text: Local Single-Node Testnet With Consensus Protocol
---

## Goal

This section describes how to set up a single-node blockchain configuration running on a single host with consensus protocol enabled.  This is referred to as a _**single host, single-node testnet with consensus**_.  We will set up one node on your local computer and have it produce blocks.  The following diagram depicts the desired single host testnet.

![Single host single node testnet](single-host-single-node-testnet.png)

`cleos` is used to manage the wallets, manage the accounts, and invoke actions on the blockchain.  `keosd` performs wallet management, including digital signing.  If not started explicitly, `keosd` is started by `cleos` by default.

## Before you begin

* [Install the EOSIO software](../../../00_install/index.md) before starting this section.
* It is assumed that `nodeos`, `cleos`, and `keosd` are accessible through the path. * Know how to pass [Nodeos options](../../02_usage/00_nodeos-options.md) to enable or disable functionality.

## Steps

Open one "terminal" window and perform the following steps:

1. [Add the development key to the wallet](#1-add-the-development-key-to-the-wallet)
2. [Start the Producer Node](#2-start-the-producer-node)
3. [Preactivate Protocol Features](#3-preactivate-protocol-features)
4. [Build System Smart Contracts](#4-build-system-smart-contracts)
5. [Install eosio.boot System Contract](#5-install-eosio.boot-system-contract)
6. [Activate The Remaining Protocol Features](#6-activate-the-remaining-protocol-features)
7. [Install eosio.bios System Contract](#7-install-eosio.bios-system-contract)

### 1. Add the development key to the wallet

Execute the following at the command prompt:

```sh
cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
```

### 2. Start the Producer Node

Start your own single-node blockchain with this single command:

```sh
nodeos -e -p eosio --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin
```

[[info | Nodeos Minimal Options]]
| A minimal `nodeos` instance setup for block production requires both `chain_api_plugin` and `history_api_plugin` with the `-e` option (enable stale production) and `-p eosio` option (producer name `eosio`). Alternatively, you can also setup and specify your own account as the producer name.

After running `nodeos`, you should get log messages similar as below. It means the blocks are successfully produced.

```console
1575001ms thread-0   chain_controller.cpp:235      _push_block          ] initm #1 @2017-09-04T04:26:15  | 0 trx, 0 pending, exectime_ms=0
1575001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initm generated block #1 @ 2017-09-04T04:26:15 with 0 trxs  0 pending
1578001ms thread-0   chain_controller.cpp:235      _push_block          ] initc #2 @2017-09-04T04:26:18  | 0 trx, 0 pending, exectime_ms=0
1578001ms thread-0   producer_plugin.cpp:207       block_production_loo ] initc generated block #2 @ 2017-09-04T04:26:18 with 0 trxs  0 pending
...
eosio generated block 046b9984... #101527 @ 2018-04-01T14:24:58.000 with 0 trxs
eosio generated block 5e527ee2... #101528 @ 2018-04-01T14:24:58.500 with 0 trxs
...
```

At this point, `nodeos` is running with a single producer, `eosio`.

### 3. Preactivate Protocol Features

All of the protocol upgrade features introduced in v1.8 and on subsequent versions require a special protocol feature (codenamed `PREACTIVATE_FEATURE`) to be activated and for an updated version of the system contract that makes use of the functionality introduced by that feature to be deployed.

To activate the special protocol `PREACTIVATE_FEATURE` run the following command:

```sh
curl --request POST \
    --url http://127.0.0.1:8888/v1/producer/schedule_protocol_feature_activations \
    -d '{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}'
```

### 4. Build System Smart Contracts

Two reference system smart contracts, the `eosio.boot` and `eosio.bios`, are located in the [`eos`](https://github.com/EOSIO/eos.git) repository. To build them execute the following at the command prompt:

```sh
cd ~
git clone https://github.com/EOSIO/eos.git
cd ./eos/contracts/contracts/
mkdir build
cd build
cmake ..
make
pwd
```

Note the path printed at the command prompt, we will refer to it later as `EOSIO_SYSTEM_CONTRACTS_DIRECTORY`.

### 5. Install eosio.boot System Contract

To install the `eosio.boot` system contract execute the following command line. Make sure you replace the `EOSIO_SYSTEM_CONTRACTS_DIRECTORY` with the directory path where you have built `eosio.boot`, that is the directory path where `eosio.boot.wasm` and `eosio.boot.abi` files are located.

```sh
cleos set contract eosio EOSIO_SYSTEM_CONTRACTS_DIRECTORY/eosio.boot/
```

As result you should see something similar to the following output:

```sh
Reading WAST/WASM from /users/documents/eos/contracts/contracts/eosio.boot/build/eosio.boot.wasm...
Using already assembled WASM...
Publishing contract...
executed transaction: 2150ed87e4564cd3fe98ccdea841dc9ff67351f9315b6384084e8572a35887cc  39968 bytes  4395 us
#         eosio <= eosio::setcode               {"account":"eosio","vmtype":0,"vmversion":0,"code":"0061736d0100000001be023060027f7e0060067f7e7e7f7f...
#         eosio <= eosio::setabi                {"account":"eosio","abi":{"types":[],"structs":[{"name":"buyrambytes","base":"","fields":[{"name":"p...
```

### 6. Activate The Remaining Protocol Features

After you deploy the eosio.boot contract, run the following commands to enable the rest of the features which are highly recommended to enable an EOSIO-based blockchain.

[[info | Optional Step]]
|These features are optional. You can choose to enable or continue without these features, however they are highly recommended for an EOSIO-based blockchain.

```sh
# KV_DATABASE
cleos push action eosio activate '["825ee6288fb1373eab1b5187ec2f04f6eacb39cb3a97f356a07c91622dd61d16"]' -p eosio

# ACTION_RETURN_VALUE
cleos push action eosio activate '["c3a6138c5061cf291310887c0b5c71fcaffeab90d5deb50d3b9e687cead45071"]' -p eosio

# CONFIGURABLE_WASM_LIMITS
cleos push action eosio activate '["bf61537fd21c61a60e542a5d66c3f6a78da0589336868307f94a82bccea84e88"]' -p eosio

# BLOCKCHAIN_PARAMETERS
cleos push action eosio activate '["5443fcf88330c586bc0e5f3dee10e7f63c76c00249c87fe4fbf7f38c082006b4"]' -p eosio

# GET_SENDER
cleos push action eosio activate '["f0af56d2c5a48d60a4a5b5c903edfb7db3a736a94ed589d0b797df33ff9d3e1d"]' -p eosio

# FORWARD_SETCODE
cleos push action eosio activate '["2652f5f96006294109b3dd0bbde63693f55324af452b799ee137a81a905eed25"]' -p eosio

# ONLY_BILL_FIRST_AUTHORIZER
cleos push action eosio activate '["8ba52fe7a3956c5cd3a656a3174b931d3bb2abb45578befc59f283ecd816a405"]' -p eosio

# RESTRICT_ACTION_TO_SELF
cleos push action eosio activate '["ad9e3d8f650687709fd68f4b90b41f7d825a365b02c23a636cef88ac2ac00c43"]' -p eosio

# DISALLOW_EMPTY_PRODUCER_SCHEDULE
cleos push action eosio activate '["68dcaa34c0517d19666e6b33add67351d8c5f69e999ca1e37931bc410a297428"]' -p eosio

 # FIX_LINKAUTH_RESTRICTION
cleos push action eosio activate '["e0fb64b1085cc5538970158d05a009c24e276fb94e1a0bf6a528b48fbc4ff526"]' -p eosio

 # REPLACE_DEFERRED
cleos push action eosio activate '["ef43112c6543b88db2283a2e077278c315ae2c84719a8b25f25cc88565fbea99"]' -p eosio

# NO_DUPLICATE_DEFERRED_ID
cleos push action eosio activate '["4a90c00d55454dc5b059055ca213579c6ea856967712a56017487886a4d4cc0f"]' -p eosio

# ONLY_LINK_TO_EXISTING_PERMISSION
cleos push action eosio activate '["1a99a59d87e06e09ec5b028a9cbb7749b4a5ad8819004365d02dc4379a8b7241"]' -p eosio

# RAM_RESTRICTIONS
cleos push action eosio activate '["4e7bf348da00a945489b2a681749eb56f5de00b900014e137ddae39f48f69d67"]' -p eosio

# WEBAUTHN_KEY
cleos push action eosio activate '["4fca8bd82bbd181e714e283f83e1b45d95ca5af40fb89ad3977b653c448f78c2"]' -p eosio

# WTMSIG_BLOCK_SIGNATURES
cleos push action eosio activate '["299dcb6af692324b899b39f16d5a530a33062804e41f09dc97e9f156b4476707"]' -p eosio
```

### 7. Install eosio.bios System Contract

To deploy the `eosio.bios` system contract execute the following at the command prompt. Make sure you replace the `EOSIO_SYSTEM_CONTRACTS_DIRECTORY` with the directory path where you built the `eosio.bios` system contract, the directory path where `eoios.bios.abi` and `eosio.bios.wasm` are located.

cleos set contract eosio EOSIO_SYSTEM_CONTRACTS_DIRECTORY/eosio.bios/

If you want instead of `eosio.bios`, a more elaborate system smart contract like `eosio.system`, you have to build it first and then execute the following at the command prompt. Make sure you replace the `EOSIO_SYSTEM_CONTRACTS_DIRECTORY` with the directory path where you have built the `eosio.system` system contract, the directory path where `eoios.system.abi` and `eosio.system.wasm` are located.

cleos set contract eosio EOSIO_SYSTEM_CONTRACTS_DIRECTORY/eosio.system/

[[info | What's next?]]
| We will explore how to setup and run a [single-host, multi-node testnet](20_local-multi-node-testnet.md).
