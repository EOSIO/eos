
## Overview

The `resource_monitor_plugin` monitors space usage in the computing system where `nodeos` is running. Specifically, every `resource-monitor-interval-seconds` seconds,
it measures the individual space used by each of the file systems mounted
by `data-dir`, `state-dir`, `blocks-log-dir`, `snapshots-dir`,
`state-history-dir`, and `trace-dir`.
When space usage in any of the monitored file system is within `5%` of the threshold
specified by `resource-monitor-space-threshold`, a warning containing the file system
path and percentage of space has used is printed out.
When space usage exceeds the threshold,
if `resource-monitor-not-shutdown-on-threshold-exceeded` is not set,
`nodeos` gracefully shuts down; if `resource-monitor-not-shutdown-on-threshold-exceeded` is set, `nodeos` prints out warnings periodically
until space usage goes under the threshold.

`resource_monitor_plugin` is always loaded.
## Usage

```console
# config.ini
plugin = eosio::resource_monitor_plugin
[options]
```
```sh
# command-line
nodeos ... --plugin eosio::resource_monitor_plugin [options]
```

## Configuration Options

These can be specified from both the `nodeos` command-line or the `config.ini` file:

```console
Config Options for eosio::resource_monitor_plugin:

  --resource-monitor-interval-seconds arg (=2)
                                        Time in seconds between two consecutive
                                        checks of resource usage. Should be 
                                        between 1 and 300
  --resource-monitor-space-threshold arg (=90)
                                        Threshold in terms of percentage of 
                                        used space vs total space. If used 
                                        space is above (threshold - 5%), a 
                                        warning is generated.  If used space is
                                        above the threshold and 
                                        resource-monitor-not-shutdown-on-thresh
                                        old-exceeded is enabled, a graceful 
                                        shutdown is initiated. The value should
                                        be between 6 and 99
  --resource-monitor-not-shutdown-on-threshold-exceeded 
                                        Used to indicate nodeos will not 
                                        shutdown when threshold is exceeded.
  --resource-monitor-warning-interval arg (=30)
                                        Number of resource monitor intervals 
                                        between two consecutive warnings when 
                                        the threshold is hit. Should be between
                                        1 and 450
```

## Plugin Dependencies

* None
