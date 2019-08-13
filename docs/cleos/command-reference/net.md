---
title: "net"
excerpt: ""
---
## Description
Interact with local p2p network connections.
## Info
**Command**

```shell
$ ./cleos net
```
Subcommands
## Subcommands
  `cleos net connect` start a new connection to a peer
  `cleos net disconnect` close an existing connection
 `cleos net status` status of existing connection
  `cleos net peers` status of all existing peers

```shell
ERROR: RequiredError: Subcommand required
Interact with local p2p network connections
Usage: ./cleos net SUBCOMMAND

Subcommands:

```
**Command**

```shell
$ ./cleos net connect
```
**Output**

```shell
ERROR: RequiredError: host
start a new connection to a peer
Usage: ./cleos net connect host

Positionals:
  host TEXT                   The hostname:port to connect to.
```
**Command**

```shell
$ ./cleos net disconnect
```
**Output**

```shell
ERROR: RequiredError: host
close an existing connection
Usage: ./cleos net disconnect host

Positionals:
  host TEXT                   The hostname:port to disconnect from.
```
**Command**

```shell
$ ./cleos net status
```
**Output**

```shell
ERROR: RequiredError: host
status of existing connection
Usage: ./cleos net status host

Positionals:
  host TEXT                   The hostname:port to query status of connection
```
