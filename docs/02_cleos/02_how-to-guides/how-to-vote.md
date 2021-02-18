## Goal

Vote for a block producer

## Before you begin

* Install the latest version of `cleos`

* Ensure the [reference system contracts](https://developers.eos.io/manuals/eosio.contracts/v1.9/build-and-deploy) are deployed and used to manage system resources.

* Understand the following:
  * What is a [block producer](https://developers.eos.io/welcome/v2.1/protocol-guides/consensus_protocol/#11-block-producers).
  * How does [voting](https://developers.eos.io/manuals/eosio.contracts/v1.9/key-concepts/vote) work.

* Unlock your wallet

## Steps

Assume you vote for blockproducer1 and blockproducer2 from an account called `eosiotestts2`, execute the following:

```sh
cleos system voteproducer prods eosiotestts2 blockproducer1 blockproducer2
```

It should produce similar output as below:

```console
executed transaction: 2d8b58f7387aef52a1746d7a22d304bbbe0304481d7751fc4a50b619df62676d  128 bytes  374 us
#         eosio <= eosio::voteproducer          {"voter":"eosiotestts2","proxy":"","producers":["blockproducer1","blockproducer2"]}
```
