## Description
Transfer tokens from account to account

## Positional Parameters
- `sender` _TEXT_ - The account sending EOS
- `recipient` _TEXT_ - The account receiving EOS
- `amount` _UINT_ - The amount of EOS to send
- `memo` _TEXT_ - The memo for the transfer

## Options
- `-c,--contract` _TEXT_ - The contract which controls the token
- `--pay-ram-to-open` - Pay ram to open recipient's token balance row
- `-x,--expiration` - set the time in seconds before a transaction expires, defaults to 30s
- `-f,--force-unique` - force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
- `-s,--skip-sign` - Specify if unlocked wallet keys should be used to sign transaction
- `-j,--json` - print result as json
- `--json-file` _TEXT_ - save result in json format into a file
- `-d,--dont-broadcast` - don't broadcast transaction to the network (just print to stdout)
- `--return-packed` - used in conjunction with --dont-broadcast to get the packed transaction
- `-r,--ref-block` _TEXT_ - set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)
- `--use-old-rpc` - use old RPC push_transaction, rather than new RPC send_transaction
- `-p,--permission` _TEXT_ ... - An account and permission level to authorize, as in 'account@permission' (defaults to 'sender@active')
- `--max-cpu-usage-ms` _UINT_ - set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)
- `--max-net-usage` _UINT_ - set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)
- `--delay-sec` _UINT_ - set the delay_sec seconds, defaults to 0s

## Example
Transfer 1000 SYS from **inita** to **tester**

```shell
$ ./cleos transfer inita tester 1000
```
The response will look something like this

```shell
{
  "transaction_id": "eb4b94b72718a369af09eb2e7885b3f494dd1d8a20278a6634611d5edd76b703",
  "processed": {
    "refBlockNum": 2206,
    "refBlockPrefix": 221394282,
    "expiration": "2017-09-05T08:03:58",
    "scope": [
      "inita",
      "tester"
    ],
    "signatures": [
      "1f22e64240e1e479eee6ccbbd79a29f1a6eb6020384b4cca1a958e7c708d3e562009ae6e60afac96f9a3b89d729a50cd5a7b5a7a647540ba1678831bf970e83312"
    ],
    "messages": [{
        "code": "eos",
        "type": "transfer",
        "authorization": [{
            "account": "inita",
            "permission": "active"
          }
        ],
        "data": {
          "from": "inita",
          "to": "tester",
          "amount": 1000,
          "memo": ""
        },
        "hex_data": "000000008040934b00000000c84267a1e80300000000000000"
      }
    ],
    "output": [{
        "notify": [{
            "name": "tester",
            "output": { ... }
          },{
            "name": "inita",
            "output": { ... }
          }
        ],
        "sync_transactions": [],
        "async_transactions": []
      }
    ]
  }
}
```
