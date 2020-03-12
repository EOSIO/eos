# trace_api_plugin

## Description

The `trace_api_plugin` provides a consumer-focused long-term API for retrieving retired actions and related metadata from a specified block. The plugin defines a new HTTP endpoint accessible (see the [API reference](api-reference/index.md) for more information).

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
* [`http_plugin`](../http_plugin/index.md)

### Load Dependency Examples

The following plugins are loaded with default settings if not specified on the command line or `config.ini`:

```console
# config.ini
plugin = eosio::chain_plugin
[options]
plugin = eosio::http_plugin 
[options]
```
```sh
# command-line
nodeos ... --plugin eosio::chain_plugin [options]  \
           --plugin eosio::http_plugin [options]
```

## Purpose

While integrating applications such as block explorers and exchanges with an EOSIO blockchain, the user might require a complete transcript of actions that are processed by the blockchain, including those spawned from the execution of smart contracts and scheduled transactions. The `trace_api_plugin` aims to serve this need. The purpose of the plugin is to provide:

* A transcript of retired actions and related metadata
* A consumer focused long-term API to retrieve blocks
* Maintainable resource commitments at the EOSIO nodes

Therefore, one crucial goal of the `trace_api_plugin` is to have better maintenance of node resources (file system, disk, memory, etc.). This goal is different from the existing `history_plugin` which provides far more configurable filtering and querying capabilities, or the existing `state_history_plugin` which provides a binary streaming interface to access structural chain data, action data, as well as state deltas.

## Examples

Below it is a `nodeos` configuration example for the `trace_api_plugin` when tracing some EOSIO reference contracts:

```sh
nodeos --data-dir data_dir --config-dir config_dir --trace-dir traces_dir
--plugin eosio::trace_api_plugin 
--trace-rpc-abi=eosio=abis/eosio.abi 
--trace-rpc-abi=eosio.token=abis/eosio.token.abi 
--trace-rpc-abi=eosio.msig=abis/eosio.msig.abi 
--trace-rpc-abi=eosio.wrap=abis/eosio.wrap.abi
```

## Maintenance Note

To reduce the disk space consummed by the `trace_api_plugin`, you can configure the following option: 

```console
  --trace-minimum-irreversible-history-blocks N (=-1) 
```

Once the value is no longer `-1`, only `N` number of blocks before the current LIB block will be kept on disk.

If resource usage cannot be effectively managed via the `trace-minimum-irreversible-history-blocks` configuration option, then there might be a need for ongoing maintenance. In that case, the user may prefer to manage resources with an external system or process.

### Manual Filesystem Management

The `trace-dir` configuration option defines a location on the filesystem where all artefacts created by the `trace_api_plugin` are stored. These files are stable once the LIB block has progressed past that slice and then can be deleted at any time to reclaim filesystem space. The conventions regarding these files are to-be-determined. However, the remainder of the system will tolerate any out-of-process management system that removes some or all of these files in this directory regardless of what data they represent, or whether there is a running `nodeos` instance accessing them or not.  Data which would nominally be available, but is no longer so due to manual maintenance, will result in a HTTP 404 response from the appropriate API endpoint(s).

In conjunction with the `trace-minimum-irreversible-history-blocks=-1` option, administrators can take full control over the lifetime of the data available via the `trace-api-plugin` and the associated filesystem resources. 
