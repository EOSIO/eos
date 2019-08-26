<!---What is keosd. What is its goal and scope--->
`keosd` is a key manager for store private keys and sign transactions created by `cleos`. When the private keys is stored in a wallet file , it will be encrypted at rest. Only when a wallet is unlocked with the wallet password, cleos can request `keosd` to sign a transaction with these private keys. Keosd provides support for hardware-based wallet as well such as Secure Encalve or YubiHSM

`keosd` is intended to be used by `eosio` developers and `eosio` power users