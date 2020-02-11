## Description

Creates a new keypair and prints the public and private keys

## Usage

```shell
Usage: cleos create key [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --r1                        Generate a key using the R1 curve (iPhone), instead of the K1 curve (Bitcoin)
  -f,--file TEXT              Name of file to write private/public key output to. (Must be set, unless "--to-console" is passed
  --to-console                Print private/public keys to console.
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
