The following steps must be taken for the example script to work.

0. Create wallet
0. Create account for enumivo.coin
0. Create account for scott
0. Create account for exchange
0. Set token contract on enumivo.coin
0. Create EOS token
0. Issue initial tokens to scott

**Note**:
Deleting the `transactions.txt` file will prevent replay from working.


### Create wallet
`enucli wallet create`

### Create account steps
`enucli create key`

`enucli create key`

`enucli wallet import <private key from step 1>`

`enucli wallet import <private key from step 2>`

`enucli create account enumivo <account_name> <public key from step 1> <public key from step 2>`

### Set contract steps
`enucli set contract enumivo.coin /contracts/enumivo.coin -p enumivo.coin@active`

### Create EOS token steps
`enucli push action enumivo.coin create '{"issuer": "enumivo.coin", "maximum_supply": "100000.0000 EOS", "can_freeze": 1, "can_recall": 1, "can_whitelist": 1}' -p enumivo.coin@active`

### Issue token steps
`enucli push action enumivo.coin issue '{"to": "scott", "quantity": "900.0000 EOS", "memo": "testing"}' -p enumivo.coin@active`
