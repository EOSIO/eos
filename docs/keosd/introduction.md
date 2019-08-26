`keosd` is a key manager for store private keys and sign transactions created by `cleos`. it providdes a secure key storage medium so that when the private keys is stored in a wallet file , it will be encrypted at rest. Only when a wallet is unlocked with the wallet password, `cleos` can request `keosd` to sign a transaction with these private keys. Keosd provides support for hardware-based wallets as well such as Secure Encalve and YubiHSM

`keosd` is intended to be used by EOSIO developers and power user

<!--- Add a diagram --->