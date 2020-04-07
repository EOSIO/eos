## Description
Retrieve accounts which are servants of a given account 

## Info

**Command**

```sh
cleos get servants
```
**Output**

```console
Usage: cleos get servants account

Positionals:
  account TEXT                The name of the controlling account
```

## Command

```sh
cleos get servants inita
```

## Output

```json
{
  "controlled_accounts": [
    "tester"
  ]
}
```
