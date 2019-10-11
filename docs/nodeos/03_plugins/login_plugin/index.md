# login_plugin

## Description

TODO: add description  
The `login_plugin` ...

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

None
