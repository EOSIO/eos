<!---What is keosd. What is its goal and scope--->
`keosd` is a key manager for store private keys and sign transactions created by `cleos`. 
When the private keys is stored in a wallet file , it will be encrypted at rest. Only when a wallet is unlocked with the wallet password, cleos or other third-party can request `keosd` to sign a transaction with these private keys.

`keosd`is intended to be used by `eosio` developers and `eosio` node operators

## Cryptography

