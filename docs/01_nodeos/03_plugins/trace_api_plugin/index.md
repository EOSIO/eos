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

### Load Dependency Examples

## Remarks

## Examples

## How-To Guides
