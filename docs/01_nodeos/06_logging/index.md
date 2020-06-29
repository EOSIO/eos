---
content_title: Nodeos Logging
---

Logging for `nodeos` is controlled by the `logging.json` file. CLI options can be passed to `nodeos` to [setup `logging.json`](00_setup-logging.json.md). The logging configuration file can be used to define [appenders](#appenders) and tie them to [loggers](#loggers) and [logging levels](01_logging-levels.md).

## Appenders

The logging library built into EOSIO supports two appender types:

- [Console](#console)
- [GELF](#gelf) (Graylog Extended Log Format)

### Console 

This will output log messages to the screen. The configuration options are:

- `name` - arbitrary name to identify instance for use in loggers
- `type` - "console"
- `stream` - "std_out" or "std_err"
- `level_colors` - maps a log level to a colour
  - level - see [logging levels](01_logging-levels.md)
  - color - may be one of ("red", "green", "brown", "blue", "magenta", "cyan", "white", "console_default")
- `enabled` - bool value to enable/disable the appender.

Example:

```json
{
    "name": "consoleout",
    "type": "console",
    "args": {
    "stream": "std_out",

    "level_colors": [{
        "level": "debug",
        "color": "green"
        },{
        "level": "warn",
        "color": "brown"
        },{
        "level": "error",
        "color": "red"
        }
    ]
    },
    "enabled": true
}
```

### GELF

This sends the log messages to `Graylog`. `Graylog` is a fully integrated platform for collecting, indexing, and analyzing log messages. The configuration options are:

 - `name` - arbitrary name to identify instance for use in loggers
 - `type` - "gelf"
 - `endpoint` - ip address and port number
 - `host` - Graylog hostname, identifies you to Graylog.
 - `enabled` - bool value to enable/disable the appender.

Example:

```json
{
    "name": "net",
    "type": "gelf",
    "args": {
        "endpoint": "104.198.210.18:12202”,
        "host": <YOURNAMEHERE IN QUOTES>
    },
    "enabled": true
}
```

## Loggers

The logging library built into EOSIO currently supports the following loggers:

- `default` - the default logger, always enabled.
- `net_plugin_impl` - detailed logging for the net plugin.
- `http_plugin` - detailed logging for the http plugin.
- `producer_plugin` - detailed logging for the producer plugin.
- `transaction_tracing` - detailed log that emits verdicts from relay nodes on the P2P network.
- `transaction_failure_tracing` - detailed log that emits failed verdicts from relay nodes on the P2P network.
- `trace_api` - detailed logging for the trace_api plugin.

The configuration options are:

 - `name` - must match one of the names described above.
 - `level` - see logging levels below.
 - `enabled` - bool value to enable/disable the logger.
 - `additivity` - true or false
 - `appenders` - list of appenders by name (name in the appender configuration)

Example:

```json
{
    "name": "net_plugin_impl",
    "level": "debug",
    "enabled": true,
    "additivity": false,
    "appenders": [
        "net"
    ]
}
```

[[info]]
| The default logging level for all loggers if no `logging.json` is provided is `info`. Each logger can be configured independently in the `logging.json` file.
