<!-- ## How to generate a blocks.log file -->

The `blocks.log` file is used by `nodeos` to persist irreversible blocks. This is the actual local copy of the immutable blockchain maintained by the node. The default location of the `blocks.log` file is in the `data/blocks` directory. However the default data directory can be overridden with the `-d [ --data-dir ]` option on the `nodeos` command line.

[[info]]
| You can also download a `blocks.log` file from third party providers.
