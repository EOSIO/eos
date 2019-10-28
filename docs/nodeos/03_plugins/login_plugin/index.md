# login_plugin

## Description

The `login_plugin` supports the concept of applications authenticating with the EOSIO blockchain. The `login_plugin` API allows an application to verify whether an account is allowed to sign in order to satisfy a specified authority (See [Accounts and Permissions](https://developers.eos.io/eosio-nodeos/docs/accounts-and-permissions).

## Usage

```sh
# config.ini
plugin = eosio::login_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::login_plugin [options]
```

## Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::login_plugin:
  --max-login-requests arg (=1000000)   The maximum number of pending login 
                                        requests
  --max-login-timeout arg (=60)         The maximum timeout for pending login 
                                        requests (in seconds)
```

## Dependencies

- [`chain_plugin`](../chain_plugin/index.md)
- [`http_plugin`](../http_plugin/index.md)

### Load Dependency Examples

```sh
# config.ini
plugin = eosio::chain_plugin
[options]
plugin = eosio::http_plugin 
[options]

# command-line
$ nodeos ... --plugin eosio::chain_plugin [options]  \
             --plugin eosio::http_plugin [options]
```
