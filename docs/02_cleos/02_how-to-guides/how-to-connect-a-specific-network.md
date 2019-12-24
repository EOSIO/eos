## Goal

Connect to a specific `nodapifiny` or `kapifinyd` host

`clapifiny` and `kapifinyd` can connecte to a specific node by using the `-H, --host` and `-p, --port` optional arguments.

[[info]]
| If no optional arguments are used (i.e. -H and -p), `clapifiny` automatically tries to connect to a locally running `nodapifiny` and `kapifinyd` node on the default port

## Before you begin

* Install the currently supported version of clapifiny

## Steps
### Connecting to Nodapifiny

```bash
  clapifiny -url http://nodapifiny-host:8888 ${subcommand}
```

### Connecting to Kapifinyd

```bash
  clapifiny --wallet-url http://kapifinyd-host:8888 ${subcommand}
```
