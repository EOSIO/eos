## Goal

Push a transaction

## Before you begin

* Install the currently supported version of clapifiny

* Understand the following:
  * What is a transaction
  * How to generate a valid transaction JSON

## Steps

* Create a JSON snippet contains a valid transaction such as the following:

```JSON
{
  "expiration": "2019-08-01T07:15:49",
  "ref_block_num": 34881,
  "ref_block_prefix": 2972818865,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "apifiny.token",
      "name": "transfer",
      "authorization": [{
          "actor": "han",
          "permission": "active"
        }
      ],
      "data": "000000000000a6690000000000ea305501000000000000000453595300000000016d"
    }
  ],
  "transaction_extensions": [],
  "context_free_data": []
}
```

* You can also create a JSON snippet that uses clear text JSON for `data` field.

[[info]]
| Be aware that if a clear text `data` field is used, clapifiny need to fetch copies of required ABIs using `nodapifiny` API. That operation has a performance overhead on `nodapifiny`

```JSON
{
  "expiration": "2019-08-01T07:15:49",
  "ref_block_num": 34881,
  "ref_block_prefix": 2972818865,
  "max_net_usage_words": 0,
  "max_cpu_usage_ms": 0,
  "delay_sec": 0,
  "context_free_actions": [],
  "actions": [{
      "account": "apifiny.token",
      "name": "transfer",
      "authorization": [{
          "actor": "han",
          "permission": "active"
        }
      ],
      "data": {
        "from": "han",
        "to": "apifiny",
        "quantity": "0.0001 SYS",
        "memo": "m"
      }
    }
  ],
  "transaction_extensions": [],
  "context_free_data": []
}
```

* Execute the following command:

```shell
clapifiny push transaction TRX_FILE.json
```

* Submit a transction from a JSON:

```shell
clapifiny push transaction JSON
```

<!---
Link to Push Action API
-->