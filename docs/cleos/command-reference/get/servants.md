---
title: "servants"
excerpt: ""
---
## Description

## Info
**Command**

```shell
$ ./cleos get servants
```
**Output**

```shell
ERROR: RequiredError: account
Retrieve accounts which are servants of a given account 
Usage: ./cleos get servants account

Positionals:
  account TEXT                The name of the controlling account
```

## Command


```shell
$ ./cleos get servants inita
```

## Output


```shell
{
  "controlled_accounts": [
    "tester"
  ]
}
```
