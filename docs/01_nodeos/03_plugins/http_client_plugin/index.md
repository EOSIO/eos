## Description

The `http_client_plugin`  is an internal utility plugin, providing the `producer_plugin` the ability to use securely an external `keosd` instance as its block signer. It can only be used when the `producer_plugin` is configured to produce blocks.

## Usage

```console
# config.ini
plugin = eosio::http_client_plugin
https-client-root-cert = "path/to/my/certificate.pem"
https-client-validate-peers = 1
```
```sh
# command-line
nodeos ... --plugin eosio::http_client_plugin  \
           --https-client-root-cert "path/to/my/certificate.pem"  \
           --https-client-validate-peers 1
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

* [`producer_plugin`](../producer_plugin/index.md)
