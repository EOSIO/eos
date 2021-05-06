
## Overview
This how-to guide provides instructions on how to update an account keys for an EOSIO blockchain account using the cleos CLI tool. 

The example uses `cleos` to update the keys for the **alice** account.

## Before you Begin
Make sure you meet the following requirements: 

* Install the currently supported version of `cleos.`
[[info | Note]]
| `Cleos` is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the `cleos` and `keosd` comand line tools. 
* You have an EOSIO account and access to the account's private key.

## Command Reference
See the following reference guides for command line usage and related options:

* [cleos create key](../03_command-reference/create/key.md) command
* [cleos wallet import](../03_command-reference/wallet/import.md) command
* [cleos set account](../03_command-reference/set/set-account.md) command

## Procedure
The following step shows how to change the keys for the `active` permissions:

1. Create a new key pair for the `active` permissions
```shell
cleos create key --to-console
```
**Where**
`--to-console` = Tells the `cleos create key` command to print the private/public keys to the console.

**Example Output**
```shell
Private key: 5KDNWQvY2seBPVUz7MiiaEDGTwACfuXu78bwZu7w2UDM9A3u3Fs
Public key: EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC
```

2. Import the new private key into your wallet
```shell
cleos wallet import --private-key 5KDNWQvY2seBPVUz7MiiaEDGTwACfuXu78bwZu7w2UDM9A3u3Fs
```
**Where**
`--private-key 5KDNWQvY2seBPVUz7MiiaEDGTwACfuXu78bwZu7w2UDM9A3u3Fs` = The private key, in WIF format, to import.

**Example Output**
```shell
imported private key for: EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC
```

3. Update the active permission key
```shell
cleos set account permission alice active EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC -p alice@owner
```
**Where**
* `alice` = The name of the account to update the key.
* `active`= The name of the permission to update the key.
* `EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC` = The new public key. 
* `-p alice@owner` = The permission used to authorize the transaction.

**Example Output**
```shell
executed transaction: ab5752ecb017f166d56e7f4203ea02631e58f06f2e0b67103b71874f608793e3  160 bytes  231 us
#         eosio <= eosio::updateauth            {"account":"alice","permission":"active","parent":"owner","auth":{"threshold":1,"keys":[{"key":"E...
```

4. Check the account
```shell
cleos get account alice
```
**Where**
`alice` = name, the name of the account to retrieve.

**Example Output**
```shell
permissions: 
     owner     1:    1 EOS6c5UjmyRsZSdikLbpAoMdg4V7FQwvdhep3KMxUifzmpDnoLVPe
        active     1:    1 EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC
memory: 
     quota:       xxx  used:      2.66 KiB  

net bandwidth: 
     used:               xxx
     available:          xxx
     limit:              xxx

cpu bandwidth:
     used:               xxx
     available:          xxx
     limit:              xxx
```

## Summary
In conclusion, by following these instructions you are able to change the keys used by an account. 
