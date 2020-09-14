---
content_title: eosio-blocklog
link_text: eosio-blocklog
---

`eosio-blocklog` is a command-line interface (CLI) utility that allows node operators to perform low-level tasks on the block logs created by a `nodeos` instance. `eosio-blocklog` can perform one of the following operations:

* Convert a range of blocks to JSON format, as single objects or array.
* Generate `blocks.index` from `blocks.log` in blocks directory.
* Trim `blocks.log` and `blocks.index` between a range of blocks.
* Perform consistency test between `blocks.log` and `blocks.index`.
* Repair `blocks.log` and reconstruct `blocks.index` if corrupted.
* Prune context-free data within given transaction and block number.
* Output the results of the operation to a file or `stdout` (default).

## Options

`eosio-blocklog` supports the following options:

Option (=default) | Description
-|-
`--blocks-dir arg (="blocks")` | The location of the blocks directory (absolute path or relative to the current directory)
`--state-history-dir arg (="state-history")` | The location of the `state-history` directory (absolute path or relative to the current dir)
`-o [ --output-file ] arg` | The file to write the generated output to (absolute or relative path). If not specified then output is to `stdout`
`-f [ --first ] arg (=0)` | The first block number to log or the first block to keep if `trim-blocklog` specified
`-l [ --last ] arg (=4294967295)` | the last block number to log or the last block to keep if `trim-blocklog` specified
`--no-pretty-print` | Do not pretty print the output. Useful if piping to `jq` to improve performance
`--as-json-array` | Print out JSON blocks wrapped in JSON array (otherwise the output is free-standing JSON objects)
`--make-index` | Create `blocks.index` from `blocks.log`. Must give `blocks-dir` location. Give `output-file` relative to current directory or absolute path (default is `<blocks-dir>/blocks.index`)
`--trim-blocklog` | Trim `blocks.log` and `blocks.index`. Must give `blocks-dir` and `first` and/or `last` options.
`--fix-irreversible-blocks` | When the existing block log is inconsistent with the index, allows fixing the block log and index files automatically - it takes the highest indexed block if valid; otherwise, it repairs the block log and reconstructs the index
`--smoke-test` | Quick test that `blocks.log` and `blocks.index` are well formed and agree with each other
`--block-num arg (=0)` | The block number which contains the transactions to be pruned
`-t [ --transaction ] arg` | The transaction id to be pruned
`--prune-transactions` | Prune the context free data and signatures from specified transactions of specified block-num
`-h [ --help ]` | Print this help message and exit

## Remarks

When `eosio-blocklog` is launched, the utility attempts to perform the specified operation, then yields the following possible outcomes:
* If successful, the selected operation is performed and the utility terminates with a zero error code (no error).
* If unsuccessful, the utility outputs an error to `stderr` and terminates with a non-zero error code (indicating an error).
