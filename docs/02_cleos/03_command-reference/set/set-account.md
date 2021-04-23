## Command
cleos set account [OPTIONS] SUBCOMMAND

**Where**
* [OPTIONS] = See Options in Command Usage section below.

**Note**: The arguments and options enclosed in square brackets are optional.
## Description
Set or update blockchain account state. Can be used to set parameters dealing with account permissions.
## Command Usage
The following information shows the different positionals and options you can use with the `cleos set account` command:
### Options:
- `-h,--help` Print this help message and exit
### Subcommands
`permission` - set parameters dealing with account permissions

## Requirements
* Install the currently supported version of `cleos.`
[[info | Note]] | The cleos tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the cleos comand line tool and the keosd key store or wallet. 
* You have access to a blockchain.
* You have an EOSIO account and access to the account private key.

## Examples

1. Update the active permission key
```shell
cleos set account permission alice active EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC -p alice@owner
```
**Where**
`alice` = The name of the account to update the key.
`active`= The name of the permission to update the key.
`EOS5zG7PsdtzQ9achTdRtXwHieL7yyigBFiJDRAQonqBsfKyL3XhC` = The new public key. 
`-p alice@owner` = The permission used to authorize the transaction.

**Example Output**
```shell
executed transaction: ab5752ecb017f166d56e7f4203ea02631e58f06f2e0b67103b71874f608793e3  160 bytes  231 us
#         eosio <= eosio::updateauth            {"account":"alice","permission":"active","parent":"owner","auth":{"threshold":1,"keys":[{"key":"E...
```
## See Also
- [Accounts and Permissions](https://developers.eos.io/welcome/latest/protocol/accounts_and_permissions) protocol document.
