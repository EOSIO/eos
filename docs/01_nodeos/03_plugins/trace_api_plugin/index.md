# trace_history_plugin

## Description

The `trace_api_plugin` provides a consumer-focused long-term API for retrieving blocks containing retired actions and related metadata.

## Usage

```console
# config.ini
plugin = eosio::trace_api_plugin
[options]
```
```sh
# command-line
nodeos ... --plugin eosio::trace_api_plugin [options]
```

## Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::trace_api_plugin:

  --trace-dir (="traces")               the location of the trace directory
                                        (absolute path or relative to
                                        application data dir)
  --trace-slice-stride (=10000)         the number of blocks each "slice" of
                                        trace data will contain on the
                                        filesystem
  --trace-minimum-irreversible-history-blocks (=-1)
                                        Number of blocks to ensure are kept
                                        past LIB for retrieval before "slice"
                                        files can be automatically removed.
                                        A value of -1 indicates that automatic
                                        removal of "slice" files will be
                                        turned off.
  --trace-rpc-abi                       ABIs used when decoding trace RPC
                                        responses.
                                        There must be at least one ABI
                                        specified OR the flag trace-no-abis
                                        must be used.
                                        ABIs are specified as "Key=Value" pairs
                                        in the form <account-name>=<abi-def>
                                        Where <abi-def> can be:
                                           an absolute path to a file
                                        containing a valid JSON-encoded ABI
                                           a relative path from `data-dir` to a
                                        file containing valid JSON-encoded ABI.
  --trace-no-abis                       Use to indicate that the RPC responses
                                        will not use ABIs.
                                        Failure to specify this option when
                                        there are no trace-rpc-abi
                                        configurations will result in an Error.
                                        This option is mutually exclusive with
                                        trace-rpc-api
```

## Dependencies

* [`chain_plugin`](../chain_plugin/index.md)

### Load Dependency Examples

```console
# config.ini
plugin = eosio::chain_plugin
[options]
```
```sh
# command-line
nodeos ... --plugin eosio::chain_plugin [options]
```

## Desgin Goals

While integrating applications such as block explorers and exchanges with a EOSIO blockchain, the developer needs a complete transcript of actions that are processed by an EOSIO blockchain including those implied by the execution of smart contracts and scheduled transactions.

This plugin is aim to solve that need. The purpose of the Trace API is to provide:

* A transcript of retired actions and the associated metadata
* A consumer focused long-term API
* Maintainable resource commitments

Thereofre, the design of `trace_api_plugin` gives more emphasis at the resource maintainability. This is distinct from the existing `history-plugin` which provides far more configurable filtering and querying capabilities and the existing `state-history-plugin` which provides a binary streaming interface to structural chain data, action data, as well as state deltas.

## Examples

Below it is an example of configuration and tracing `eosio` reference contract

```sh
nodeos --data-dir data_dir --config-dir config_dir --trace-dir traces_dir
--plugin eosio::trace_api_plugin 
--trace-rpc-abi=eosio=abis/eosio.abi 
--trace-rpc-abi=eosio.token=abis/eosio.token.abi 
--trace-rpc-abi=eosio.msig=abis/eosio.msig.abi 
--trace-rpc-abi=eosio.wrap=abis/eosio.wrap.abi
```

## Maintenance Note

If resource usage can be effectively managed based on number of blocks via the trace-minimum-irreversible-history-blocks configuration, then there is no need for on-going maintenance.  In some deployments, the user may prefer to manage resources with an external system or process.

## How-To Guides
