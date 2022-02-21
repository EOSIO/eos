## Goal
Create a keypair consisting of a public and a private key for signing transactions in the EOSIO blockchain.

## Before you begin
Before you follow the steps to create a new key pair, make sure the following items are fulfilled:


* Install the currently supported version of `cleos`

[[info | Note]]
| The cleos tool is bundled with the EOSIO software. [Installing EOSIO](../../00_install/index.md) will also install the cleos tool. 

* Acquire functional understanding of asymmetric cryptography (public and private keypair) in the context of blockchain

## Steps

To create a keypair and print the result to the console:

```sh
cleos create key --to-console
```

**Example Output**

```sh
cleos create key --to-console
Private key: 5KPzrqNMJdr6AX6abKg*******************************cH
Public key: EOS4wSiQ2jbYGrqiiKCm8oWR88NYoqnmK4nNL1RCtSQeSFkGtqsNc
```


To create a keypair and save it to a file:

```sh
cleos create key --file FILE_TO_SAVEKEY
```
Where: FILE_TO_SAVEKEY = name of the file

**Example Output**
```sh
cleos create key --file pw.txt         
saving keys to pw.txt
```

To view the saved keypair in the file:
```sh
cat pw.txt
Private key: 5K7************************************************
Public key: EOS71k3WdpLDeqeyqVRAAxwpz6TqXwDo9Brik5dQhdvvpeTKdNT59
```
