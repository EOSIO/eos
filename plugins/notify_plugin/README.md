# notify_plugin

Send real time actions on chain to a `receive_url`, which you can use to do some notifications.

### Usage

Add some configs to your `config.ini` just as follows:

```
## Notify Plugin
plugin = eosio::notify_plugin
# notify-filter-on = account:action
notify-filter-on = b1:
notify-filter-on = b1:transfer
notify-filter-on = eosio:delegatebw
# http endpoint for each action seen on the chain.
notify-receive-url = http://127.0.0.1:8080/notify
# Age limit in seconds for blocks to send notifications. No age limit if set to negative.
# Used to prevent old actions from trigger HTTP request while on replay (seconds)
notify-age-limit = -1
# Retry times of sending http notification if failed.
notify-retry-times = 3
```

And you can receive the actions on chain by watching your server endpoint: `http://127.0.0.1:8080/notify`, the data sent to the API endpoint looks like:

```json
{
  "irreversible": true,
  "actions": [{
      "tx_id": "b31885bada6c2d5e71b1302e87d4006c59ff2a40a12108559d76142548d8cf79",
      "account": "eosio.token",
      "name": "transfer",
      "seq_num": 1,
      "receiver": "b1",
      "block_time": "2018-09-29T11:51:06.000",
      "block_num": 127225,
      "authorization": [{
          "actor": "b1",
          "permission": "active"
        }
      ],
      "action_data": {
        "from": "b1",
        "to": "b11",
        "quantity": "0.0001 EOS",
        "memo": "Transfer from b1 to b11"
      }
    },{
      "tx_id": "b31885bada6c2d5e71b1302e87d4006c59ff2a40a12108559d76142548d8cf79",
      "account": "eosio.token",
      "name": "transfer",
      "seq_num": 2,
      "receiver": "b11",
      "block_time": "2018-09-29T11:51:06.000",
      "block_num": 127225,
      "authorization": [{
          "actor": "b1",
          "permission": "active"
        }
      ],
      "action_data": {
        "from": "b1",
        "to": "b11",
        "quantity": "0.0001 EOS",
        "memo": "Transfer from b1 to b11"
      }
    }
  ]
}
```

In your server side, you can use these actions to do many things, such as creating a telegram alert bot which you can subscribe on and receive your account's information on chain.