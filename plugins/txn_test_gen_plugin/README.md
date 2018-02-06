# txn\_test\_gen\_plugin

This plugin provides a way to generate a given amount of transactions per second against the currency contract. It runs internally to eosd to reduce overhead.

## Setup

Create the two accounts used for transactions and set the currency contract code. Must give account & private key used for creation.

```
curl --data-binary '["inita", "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]' http://localhost:7777/v1/txn_test_gen/create_test_accounts
```

## Starting/Stopping

Start generation:

```
curl --data-binary '["one", 20, 16]' http://localhost:7777/v1/txn_test_gen/start_generation
```

First parameter is some "salt" added to the transaction memos to ensure if this test is being run on other nodes concurrently no identical transactions are created. The second parameter is the frequency in milliseconds to wake up and send transactions. The third parameter is how many transactions to send on each wake up (must be an even number). There are some constraints on these numbers to prevent anything too wild.

Stop generation

```
curl http://localhost:7777/v1/txn_test_gen/stop_generation
```

Generation of transactions is also stopped if a transaction fails to be accepted for any reason.