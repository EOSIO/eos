---
content_title: Cleos FAQ
---

## Domain Socket (IPC) vs. HTTPS (RPC)

There are two options to connect `cleos` to `keosd`. You can either use domain sockets or HTTPS. Uses domain socket offering many benefits. It reduces the chance of leaking access of `keosd` to the LAN, WAN or Internet. Also, unlike HTTPS protocol where many attack vectors exist such as CORS, a domain socket can only be used for the intended use case Inter-Processes Communication

## What does "transaction executed locally, but may not be confirmed by the network yet" means?

It means the transaction has been successfully accepted and executed by the instance of `nodeos` that `cleos` submitted it directly to. That instance of `nodeos` should relay the transaction to additional instances via the peer-to-peer protocol but, there is no guarantee that these additional instances accepted or executed the transaction. There is also no guarantee, at this point, that the transaction has been accepted and executed by a valid block producer and subsequently included in a valid block in the blockchain. If you require stronger confirmation of a transaction's inclusion in the immutable blockchain, you must take extra steps to monitor for the transaction's presence in an irreversible block
