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
* `host` _TEXT_ REQUIRED - The hostname:port to disconnect from.

### Options
* `-h,--help` - Print this help message and exit.

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
      "node_id": "f6c768c8609ffeb1b6b81e4cd7cf3a485b16f6525d3aa65e608ac97653a29ff6",
      "key": "EOS1111111111111111111111111111111114T1Anm",
      "time": "1620935836749878000",
      "token": "0000000000000000000000000000000000000000000000000000000000000000",
      "sig": "SIG_K1_111111111111111111111111111111111111111111111111111111111111111116uk5ne",
      "p2p_address": "127.0.0.1:9003 - f6c768c",
      "last_irreversible_block_num": 125,
      "last_irreversible_block_id": "0000007d082ab6192e17c5d72c702999aaa72dcd198f8a1368aa433b08469a0d",
      "head_num": 126,
      "head_id": "0000007e741296acf83ec771a61bc01f49d25e17e1cc088cdd82df42af7fdd4a",
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
      "node_id": "c200a0e509db18d55ea66b014f20eaf4608220a21a43130fbd9e33cc975bc1fd",
      "key": "EOS1111111111111111111111111111111114T1Anm",
      "time": "1620935837960123000",
      "token": "0000000000000000000000000000000000000000000000000000000000000000",
      "sig": "SIG_K1_111111111111111111111111111111111111111111111111111111111111111116uk5ne",
      "p2p_address": "127.0.0.1:9010 - c200a0e",
      "last_irreversible_block_num": 128,
      "last_irreversible_block_id": "00000080ac79cafa0835367576bca3490340758303da9dbd01126d1cb094dc64",
      "head_num": 129,
      "head_id": "00000081bb2d7a8f14e45a48532e08bfa8d377ee5422c779ab14fdef0de3e6b6",
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
      "node_id": "e351c9b9ccfde7a295b6a503cea80bf4bc3c667eca8b5954a9fa0188fdef8f35",
      "key": "EOS1111111111111111111111111111111114T1Anm",
      "time": "1620935836805188000",
      "token": "0000000000000000000000000000000000000000000000000000000000000000",
      "sig": "SIG_K1_111111111111111111111111111111111111111111111111111111111111111116uk5ne",
      "p2p_address": "127.0.0.1:9004 - e351c9b",
      "last_irreversible_block_num": 125,
      "last_irreversible_block_id": "0000007d082ab6192e17c5d72c702999aaa72dcd198f8a1368aa433b08469a0d",
      "head_num": 126,
      "head_id": "0000007e741296acf83ec771a61bc01f49d25e17e1cc088cdd82df42af7fdd4a",
      "os": "osx",
      "agent": "EOS Test Agent",
      "generation": 4
    }
  }
]
```

**Note:** The `last_handshake` field contains the chain state of each connected peer as of the last handshake message with the node. For more information read the [Handshake Message](https://developers.eos.io/welcome/latest/protocol/network_peer_protocol#421-handshake-message) in the *Network Peer Protocol* document.
