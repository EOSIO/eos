# EOS STRESS TEST SCRIPT


The following stress test script will send bunch of transfer transactions equally distributed to each node in the testnet:
 - transfer from inita to initb
 - transfer from initb to initc
 - transfer from initc to initd
 - .....
 - transfer from initu to inita


## Requirement
Node.js version 8.x.x or above

## Setup
```
npm install
```
Also ensure that your eos wallet has key for inita imported and it is unlocked

## Config
- testnetUris: array of testnet uri (e.g. "http://localhost:8888")
- serializeTrxNodeUri: uri of node to serialize transaction (e.g. "http://localhost:8888")
- walletUri: uri of the wallet (e.g. "http://localhost:8888")
- minNumOfTrxToSend: minimum number of transaction to send
- numOfTrxPerBatch: number of transaction to send in batch
- maxSimulHttpReqPerNode: maximum number of http request made to each node at one time

## Run
```
node stress_test.js
```