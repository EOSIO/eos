## Command

```sh
cleos get account [OPTIONS] name [core-symbol]
```

**Where**:

* [`OPTIONS`] = See **Options** in **Command Usage** section below
* `name` = The name of the account to retrieve
* [`core-symbol`] = The expected core symbol of the chain you are querying

## Description
Use this command to retrieve information associated with a blockchain account. 


## Command Usage

The following information shows the different positionals and options you can use with the `cleos get account` command:

```console
Usage: cleos get account [OPTIONS] name [core-symbol]

Positionals:
  name TEXT                   The name of the account to retrieve (required)
  core-symbol TEXT            The expected core symbol of the chain you are querying

Options:
  -j,--json                   Output in JSON format
```

## Requirements

For prerequisites to run this command, see the **Before you Begin** section of the [_How to Get Account Information_](../02_how-to-guides/how-to-get-account-information.md) topic.

## Requirements
* Install the currently supported version of `cleos.`
[[info | Note]] 
| The `cleos` tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will install the `cleos` and `keosd` command line tools. 
* You have access to an EOSIO blockchain.

## Examples

The following examples retrieves data associated with the `eosio` account:

**Example 1: Retrieve formatted data for `eosio` account**


```shell
cleos get account eosio
```
**Where**
`eosio` = The name of the account.

**Example Output**
```console
privileged: true
permissions: 
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory: 
     quota:        -1 bytes  used:      1.22 Mb   

net bandwidth: (averaged over 3 days)
     used:                -1 bytes
     available:           -1 bytes
     limit:               -1 bytes

cpu bandwidth: (averaged over 3 days)
     used:                -1 us
     available:           -1 us   
     limit:               -1 us   

producers:     <not voted>
```


**Example 2: Retrieve formatted JSON data for `eosio` account**

```sh
cleos get account eosio --json
```
**Example Output**

```json
{
  "account_name": "eosio",
  "privileged": true,
  "last_code_update": "2018-05-23T18:00:25.500",
  "created": "2018-03-02T12:00:00.000",
  "ram_quota": -1,
  "net_weight": -1,
  "cpu_weight": -1,
  "net_limit": {
    "used": -1,
    "available": -1,
    "max": -1
  },
  "cpu_limit": {
    "used": -1,
    "available": -1,
    "max": -1
  },
  "ram_usage": 1279625,
  "permissions": [{
      "perm_name": "active",
      "parent": "owner",
      "required_auth": {
        "threshold": 1,
        "keys": [{
            "key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
            "weight": 1
          }
        ],
        "accounts": [],
        "waits": []
      }
    },{
      "perm_name": "owner",
      "parent": "",
      "required_auth": {
        "threshold": 1,
        "keys": [{
            "key": "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
            "weight": 1
          }
        ],
        "accounts": [],
        "waits": []
      }
    }
  ],
  "total_resources": null,
  "delegated_bandwidth": null,
  "voter_info": {
    "owner": "eosio",
    "proxy": "",
    "producers": [],
    "staked": 0,
    "last_vote_weight": "0.00000000000000000",
    "proxied_vote_weight": "0.00000000000000000",
    "is_proxy": 0,
    "deferred_trx_id": 0,
    "last_unstake_time": "1970-01-01T00:00:00",
    "unstaking": "0.0000 SYS"
  }
}
```

## See Also
- [Accounts and Permissions](https://developers.eos.io/welcome/v2.1/protocol/accounts_and_permissions) protocol document.
