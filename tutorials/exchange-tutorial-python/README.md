The following steps must be taken for the example script to work.

0. Create wallet
0. Create account for arisen.token
0. Create account for jared
0. Create account for exchange
0. Set token contract on arisen.token
0. Create RSN token
0. Issue initial tokens to jared

**Note**:
Deleting the `transactions.txt` file will prevent replay from working.


### Create wallet
`arisecli wallet create`

### Create account steps
`arisecli create key`

`arisecli create key`

`arisecli wallet import  --private-key <private key from step 1>`

`arisecli wallet import  --private-key <private key from step 2>`

`arisecli create account eosio <account_name> <public key from step 1> <public key from step 2>`

### Set contract steps
`arisecli set contract arisen.token /contracts/arisen.token -p arisen.token@active`

### Create RSN token steps
`arisecli push action arisen.token create '{"issuer": "arisen.token", "maximum_supply": "100000.0000 RSN", "can_freeze": 1, "can_recall": 1, "can_whitelist": 1}' -p arisen.token@active`

### Issue token steps
`arisecli push action arisen.token issue '{"to": "jared", "quantity": "900.0000 RSN", "memo": "testing"}' -p arisen.token@active`
