---
excerpt: "Creates a new keypair and prints the public and private keys."
---

## Usage

```shell
Usage: cleos get account [OPTIONS] name [core-symbol]

Positionals:
  name TEXT                   The name of the account to retrieve (required)
  core-symbol TEXT            The expected core symbol of the chain you are querying

Options:
  -j,--json                   Output in JSON format

```

## Command

```shell
$ ./cleos create key -f passwd
```

## Output

```shell
Private key: 5KCkcSxYKZfh5Cr8CCunS2PiUKzNZLhtfBjudaUnad3PDargFQo
Public key: EOS5uHeBsURAT6bBXNtvwKtWaiDSDJSdSmc96rHVws5M1qqVCkAm6
```
