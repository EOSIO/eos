## Command
```sh
cleos net peers [OPTIONS]
```

**Where:**
* [OPTIONS] = See **Options** in the [**Command Usage**](command-usage) section below.

**Note:** The arguments and options enclosed in square brackets are optional.

## Description
Returns a list with the status of all peer connections. This command allows a node operator to check the status of a node's peer connections.

## Command Usage
The following information shows the different positionals and options you can use with the `cleos net peers` command:

### Positionals
* `host` _TEXT_ REQUIRED - The hostname:port to disconnect from

### Options
* `-h,--help` - Print this help message and exit

## Requirements
Make sure you meet the following requirements:

* Install the currently supported version of `cleos`.
[[info | Note]]
| `cleos` is bundled with the EOSIO software. [Installing EOSIO](../../../00_install/index.md) will also install the `cleos` and `keosd` command line tools.
* You have access to a producing node instance with the [`net_api_plugin`](../../../01_nodeos/03_plugins/net_api_plugin/index.md) loaded.

## Examples
The following examples demonstrate how to use the `cleos net peers` command:

* List the status of all peer connections for a local node listening at http address `http://127.0.0.1:8001`:

```sh
cleos -u http://127.0.0.1:8001 net peers
```
**Output:**
```json
[{
    "peer": "",
    "connecting": false,
    "syncing": false,
    "last_handshake": {
      "network_version": 1210,
      "chain_id": "60fb0eb4742886af8a0e147f4af6fd363e8e8d8f18bdf73a10ee0134fec1c551",
      "node_id": "58ed23173cd4ed89ae90c2e65f5c29bb51e233e78d730d72220b6d84543bfc08",
      "key": "EOS1111111111111111111111111111111114T1Anm",
      "time": "1621013128445696000",
      "token": "0000000000000000000000000000000000000000000000000000000000000000",
      "sig": "SIG_K1_111111111111111111111111111111111111111111111111111111111111111116uk5ne",
      "p2p_address": "127.0.0.1:9005 - 58ed231",
      "last_irreversible_block_num": 127,
      "last_irreversible_block_id": "0000007f97323fae098048ae41f22a99238afc5db56cad17f50304919d21e1c2",
      "head_num": 128,
      "head_id": "0000008072eb0fc043770e44a5db5dedfbd86db9aa5c41b28618f1b9343c2d22",
      "os": "osx",
      "agent": "EOS Test Agent",
      "generation": 4
    }
  },{
    "peer": "",
    "connecting": false,
    "syncing": false,
    "last_handshake": {
      "network_version": 1210,
      "chain_id": "60fb0eb4742886af8a0e147f4af6fd363e8e8d8f18bdf73a10ee0134fec1c551",
      "node_id": "2101bd8d770e8eabb3e69cb981db4350b494a04cd5b7a7cea0cd3070aa722306",
      "key": "EOS1111111111111111111111111111111114T1Anm",
      "time": "1621013129884873000",
      "token": "0000000000000000000000000000000000000000000000000000000000000000",
      "sig": "SIG_K1_111111111111111111111111111111111111111111111111111111111111111116uk5ne",
      "p2p_address": "127.0.0.1:9017 - 2101bd8",
      "last_irreversible_block_num": 129,
      "last_irreversible_block_id": "0000008117074454dbaa7e8662c6f3d6918e776cc063c45f52b37bdc945ddc5d",
      "head_num": 130,
      "head_id": "0000008248cc3498b4bbf14a183711d9a73a36517a8069367a343bd4060fed14",
      "os": "osx",
      "agent": "EOS Test Agent",
      "generation": 3
    }
  },{

  ...

  },{
    "peer": "",
    "connecting": false,
    "syncing": false,
    "last_handshake": {
      "network_version": 1210,
      "chain_id": "60fb0eb4742886af8a0e147f4af6fd363e8e8d8f18bdf73a10ee0134fec1c551",
      "node_id": "d9eb85085d09581521d0ec53b87a9657d0176c74fdf8657c56b09a91b3821c6f",
      "key": "EOS1111111111111111111111111111111114T1Anm",
      "time": "1621013127894327000",
      "token": "0000000000000000000000000000000000000000000000000000000000000000",
      "sig": "SIG_K1_111111111111111111111111111111111111111111111111111111111111111116uk5ne",
      "p2p_address": "127.0.0.1:9002 - d9eb850",
      "last_irreversible_block_num": 125,
      "last_irreversible_block_id": "0000007d9a3b9cf6a61776ba1bbce226754aefcad664338d2acb5be34cc53a5b",
      "head_num": 126,
      "head_id": "0000007eccafd4f32a437b5fd8b111b326e2da8d0bcb832036984841b81ab64e",
      "os": "osx",
      "agent": "EOS Test Agent",
      "generation": 4
    }
  }
]
```

**Note:** The `last_handshake` field contains the chain state of each connected peer as of the last handshake message with the node. For more information read the [Handshake Message](https://developers.eos.io/welcome/latest/protocol/network_peer_protocol#421-handshake-message) in the *Network Peer Protocol* document.
