---
title: "transactions"
excerpt: "Retrieves all transactions with specific account name referenced in their scope."
---
## Positional Parameters
- `account_name` _TEXT_ - name of account to query on
- `skip_seq` _TEXT_ - Number of most recent transactions to skip (0 would start at most recent transaction)
- `num_seq` _TEXT_ - Number of transactions to return
## Examples
Get transactions belonging to eosio

```shell
$ ./cleos get transactions eosio
[
  {
    "transaction_id": "eb4b94b72718a369af09eb2e7885b3f494dd1d8a20278a6634611d5edd76b703",
    ...
  },
  {
    "transaction_id": "6acd2ece68c4b86c1fa209c3989235063384020781f2c67bbb80bc8d540ca120",
    ...
  },
  ...
]
```

[[info]]
|Important Note
These transactions will not exist on your blockchain
