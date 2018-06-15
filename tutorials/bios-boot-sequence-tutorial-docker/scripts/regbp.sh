#!/bin/bash

# Run local keosd before connecting to external keosd (It look like a bug)
cleos --url=$1  wallet list

password=$(cleos --url=$1 --wallet-url=$2 wallet create | grep -oP '"\K[^"\047]+(?=["\047])')
echo ${password}

cleos --url=$1 --wallet-url=$2 wallet import $6

# echo ${password} | cleos --url=$1 --wallet-url=$2 wallet unlock

# Register as a producer
cleos --url=$1 --wallet-url=$2 system regproducer $3 $5 $4/$5 -p $3

# cleos --url=$1 --wallet-url=$2 system listproducers

sleep 60
while true
do
	# cleos --url=$1 --wallet-url=$2 get currency balance eosio.token $3 SYS
	cleos --url=$1 --wallet-url=$2 system claimrewards $3
	cleos --url=$1 --wallet-url=$2 get currency balance eosio.token $3 SYS
	sleep 86400 # 60 * 60 * 24
done

