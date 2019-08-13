## Domain Socket (IPC) vs. HTTPS (RPC)

There are two options to connect `cleos` to `keosd`. You can either use domain sockets or HTTPS over TCP. Uses domain socket offering many benefits. It reduces the chance of leaking access of `keosd` to the LAN, WAN or Internet. Also unlike HTTPS protocol where many attack vectors exists such as CORS, a domain socket can only be used for the intended use case Inter-Processes Communication

## What does "transaction executed locally, but may not be confirmed by the network yet" means?

You will see this message frequnently while you use `cleos`. It means the transaction has only been executed by your local nodeos, however, it has not been confirmed by EOSIO network therefore you *should not* consider the transaction has execuated successfully on blockchain.