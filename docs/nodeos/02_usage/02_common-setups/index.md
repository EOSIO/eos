# Nodeos Common Configurations

`Nodeos` generally runs in two modes:

 * [Producing Node](00_producing-node.md)
 * [Non-Producing Node](01_non-producing-node.md)

`Producing Nodes` will connect to the peer to peer network and actively produce new blocks. `Non-Producing Nodes` will connect to the peer to peer network, however, in this mode `nodeos` does not actively produce new blocks, it will simply verify blocks and maintain a local copy of the blockchain.

`Non-Producing Nodes` are useful for monitoring the blockchain.
