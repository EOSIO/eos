## Overview

This how-to guide provides instructions on how to vote for block producers.

## Before you begin

* Install the latest version of `cleos`.

* Ensure the [reference system contracts](https://developers.eos.io/manuals/eosio.contracts/v1.9/build-and-deploy) are deployed and used to manage system resources.

* Understand the following:
  * What a [block producer](https://developers.eos.io/welcome/v2.1/protocol-guides/consensus_protocol/#11-block-producers) is.
  * How [voting](https://developers.eos.io/manuals/eosio.contracts/v1.9/key-concepts/vote) works.

* Unlock your wallet.

## Command Reference

See the following reference guides for command line usage and related options for the `cleos` command:

* The [cleos system voteproducer prods](https://developers.eos.io/manuals/eos/v2.1/cleos/command-reference/system/system-voteproducer-prods) reference.

## Procedure

The following steps show:

1. How to vote for blockproducer1 and blockproducer2 from an account called `eosiotestts2`:

```sh
cleos system voteproducer prods eosiotestts2 blockproducer1 blockproducer2
```

Where:

* `eosiotestts2` = the account that votes.
* `blockproducer1` and `blockproducer2` = the accounts receiving the votes. The number of accounts receiving the votes can vary from one to multiple. Maximum default number of block producers one account can vote for is 30.

Example output:

```console
executed transaction: 2d8b58f7387aef52a1746d7a22d304bbbe0304481d7751fc4a50b619df62676d  128 bytes  374 us
#         eosio <= eosio::voteproducer          {"voter":"eosiotestts2","proxy":"","producers":["blockproducer1","blockproducer2"]}
```

## Summary

In conclusion, the above instructions show how to vote for block producers.
