#!/bin/bash

# Run local keosd before connecting to external keosd (It look like a bug)
cleos --url=$1  wallet list


password=$(cleos --url=$1 --wallet-url=$2 wallet create | grep -oP '"\K[^"\047]+(?=["\047])')
echo ${password}

# echo ${password} | cleos --url=$1 --wallet-url=$2 wallet unlock

cleos --url=$1 --wallet-url=$2 system listproducers

# Vote for producers
voter1PrivateKey="5J2GtQMvoEAWJeBCs7x12LRwL5FJMhTgh8CJdmtsA11fr9JRi9P"
voter1PublicKey="EOS5itkUjPL1em2ZrXm2G6a2cuLSifoUX1E3jo8CXM8egn68AUqjX"
cleos --url=$1 --wallet-url=$2 wallet import $voter1PrivateKey
cleos --url=$1 --wallet-url=$2 system voteproducer prods voter1 bp1 bp2 bp3

voter2PrivateKey="5Kg56iYDEEgMDFhBcxppKHnkuoeUvYm2eM7gHt989wAatAg2kuX"
voter2PublicKey="EOS8ghsoYCYbtQx7YUAWm2Zm5eWfJat819QTVs8wdTSC4yo52n8W9"
cleos --url=$1 --wallet-url=$2 wallet import $voter2PrivateKey
cleos --url=$1 --wallet-url=$2 system voteproducer prods voter2 bp1 bp2

voter3PrivateKey="5JjhQpVkZ3NmHkv82CrRLukGcnjVaVzroS7SD1HCMamH4GjwFfJ"
voter3PublicKey="EOS87aAYfzamC5d7phTU7HFkeMTiKyLBqvDJfzsRo7zQWsaFjqBtP"
cleos --url=$1 --wallet-url=$2 wallet import $voter3PrivateKey
cleos --url=$1 --wallet-url=$2 system voteproducer prods voter3 bp1 bp2

voter4PrivateKey="5JtqeecHB3WMsAZcLswuf89uhZXDhRvUzgVFEAViQdHAFc8E2pM"
voter4PublicKey="EOS6RxXzVSeH8p4kG5h1iDB5MUhwwuPCvxS2gx8YV2LH7DTTbgQdP"
cleos --url=$1 --wallet-url=$2 wallet import $voter4PrivateKey
cleos --url=$1 --wallet-url=$2 system voteproducer prods voter4 bp1

cleos --url=$1 --wallet-url=$2 system listproducers -j
