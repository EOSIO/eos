## Description
Transfer tokens from account to account

## Positional Parameters
- `sender` _TEXT_ - The account sending EOS
- `recipient` _TEXT_ - The account receiving EOS
- `amount` _UINT_ - The amount of EOS to send
- `memo` _TEXT_ - The memo for the transfer

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
