# http_client_plugin

## Description

TODO: add description  
The `http_client_plugin` ...

## Usage

```sh
# config.ini
plugin = eosio::http_client_plugin
[options]

# command-line
$ nodeos ... --plugin eosio::http_client_plugin [options]
```

## Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::http_client_plugin:
  --https-client-root-cert arg          PEM encoded trusted root certificate 
                                        (or path to file containing one) used 
                                        to validate any TLS connections made.  
                                        (may specify multiple times)
                                        
  --https-client-validate-peers arg (=1)
                                        true: validate that the peer 
                                        certificates are valid and trusted, 
                                        false: ignore cert errors
```

## Dependencies

None
