# txn\_test\_gen\_plugin

This plugin provides a way to generate a given amount of test transactions per second. It runs internally to eosd to reduce overhead.

## Setup

Create the two accounts used for transactions. Must give account & private key used for creation.

```
curl --data-binary '["inita", "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]' http://localhost:7777/v1/txn_test_gen/create_test_accounts
```

## Starting/Stopping

Start generation:

```
curl --data-binary '["one", 400]' http://localhost:7777/v1/txn_test_gen/start_generation
```

First parameter is some "salt" added to the transaction memos to ensure if this test is being run on other nodes concurrently no identical transactions are created. The second parameter is the requested transactions per second. This knob is currently quite crude so you don't always get what you ask for. Also, 400 is the limit.
Stop generation

```
curl http://localhost:7777/v1/txn_test_gen/stop_generation
```

Generation of transactions is also stopped if a transaction fails to be accepted for any reason.