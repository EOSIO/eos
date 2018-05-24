The following steps must be taken for the example script to work.

0. Create wallet
0. Create account for eosio.token
0. Create account for scott
0. Create account for exchange
0. Set token contract on eosio.token
0. Create EOS token
0. Issue initial tokens to scott

**Note**:
Deleting the `transactions.txt` file will prevent replay from working.


### Create wallet
`cleos wallet create`

### Create account steps
`cleos create key`

`cleos create key`

`cleos wallet import <private key from step 1>`

`cleos wallet import <private key from step 2>`

`cleos create account eosio <account_name> <public key from step 1> <public key from step 2>`

### Set contract steps
`cleos set contract eosio.token /contracts/eosio.token -p eosio.token@active`

### Create EOS token steps
`cleos push action eosio.token create '{"issuer": "eosio.token", "maximum_supply": "100000.0000 EOS", "can_freeze": 1, "can_recall": 1, "can_whitelist": 1}' -p eosio.token@active`

### Issue token steps
`cleos push action eosio.token issue '{"to": "scott", "quantity": "900.0000 EOS", "memo": "testing"}' -p eosio.token@active`
