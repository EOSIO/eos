---
content_title: trace_api_util
link_text: trace_api_util
---

`trace_api_util` is a command-line interface (CLI) utility that allows node operators to perform low-level tasks associated with the [Trace API Plugin](../01_nodeos/03_plugins/trace_api_plugin/index.md). `trace_api_util` can perform one of the following operations:

* Compress a trace `log` file into the compressed `clog` format.

## Usage
```sh
trace_api_util <options> command ...
```

## Options
Option (=default) | Description
-|-
`-h [ --help` ] | show usage help message

## Commands
Command | Description
-|-
`compress` | Compress a trace file to into the `clog` format

### compress
Compress a trace `log` file into the `clog` format.  By default the name of the compressed file will be the same as the `input-path` but with the file extension changed to `clog`.

#### Usage
```sh
trace_api_util compress <options> input-path [output-path]
```

#### Positional Options
Option (=default) | Description
-|-
`input-path` | path to the file to compress
`output-path` | [Optional] output file or directory path

#### Options
Option (=default) | Description
-|-
`-h [ --help ]` | show usage help message
`-s [ --seek-point-stride ] arg (=512)` | the number of bytes between seek points in a compressed trace.  A smaller stride may degrade compression efficiency but increase read efficiency

## Remarks
When `trace_api_util` is launched, the utility attempts to perform the specified operation, then yields the following possible outcomes:
* If successful, the selected operation is performed and the utility terminates with a zero error code (no error).
* If unsuccessful, the utility outputs an error to `stderr` and terminates with a non-zero error code (indicating an error).
