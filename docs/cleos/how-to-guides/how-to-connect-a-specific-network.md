## Goal

Connect to a specific `nodeos` or `keosd` host

`cleos` and `keosd` can connecte to a specific node by using the `-H, --host` and `-p, --port` optional arguments.

[[info]]
| If no optional arguments are used (i.e. -H and -p), `cleos` automatically tries to connect to a locally running `nodeos` and `keosd` node on the default port

## Before you begin

* Install the currently supported version of cleos

## Steps

### Connecting to Nodeos

```bash
  cleos -url http://nodeos-host:8888 ${subcommand}
```

### Connecting to Keosd

```bash
  cleos --wallet-url http://keosd-host:8888 ${subcommand}
```
