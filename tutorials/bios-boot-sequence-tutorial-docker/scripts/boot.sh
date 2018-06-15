#!/bin/bash

cleos --url=$1 --wallet-url=$2 get info

privateKey="5JGxnezvp3N4V1NxBo8LPBvCrdR85bZqZUFvBZ8ACrbRC3ZWNYv"
publicKey="EOS8VJybqtm41PMmXL1QUUDSfCrs9umYN4U1ZNa34JhPZ9mU5r2Cm"

bpayPrivateKey="5JHgc2CFkGNg5kNcGqxqW2WRXkgvoQDcxNybKJaKEXdyyqQMV45"
bpayPublicKey="EOS7nwzi3nM8qjWE2XemR4p6Czh7r7ny4bRKQBNz93U3Me2HU5hsS"

msigPrivateKey="5Kb3bgcuFaQQ6qERsbPXP3msAX1kfD1QuAhryerGVDjx5D42NUn"
msigPublicKey="EOS8jMUhgf8LQXteuPJJAEDRonQs63XMZd43dGe6hoocyiwdBZP9U"

namesPrivateKey="5K8NYTEawBPauJHWyV2f3YeiRbSwjKqPjPp7EveUKAPdaPJSGgq"
namesPublicKey="EOS4yRZS2Hus2ZdqzkRrWZQxjMjwbp2oG4F8UMpoYR2LJsyfSS6VU"

ramPrivateKey="5JxLzzqHXmp2XpNWDJq8KoDdCyGbEdQoafqJH74rGF3y5AsTaHV"
ramPublicKey="EOS6bn3bvw9Nchg8MbjkS7EaE1Vg16X86DCVfg9r2JrPVKkdCC5hu"

ramfeePrivateKey="5JejtPrVmMCrVR7xDbrxJuSqYQaiTTxqEtg2HA8Q6itAYBHGGUP"
ramfeePublicKey="EOS6XZHa4wA9gdWLuABymbPXxPxHgiSnrBhLXvsS1qQABByb11yW4"

savingPrivateKey="5JHrpRLkFXvYaCm9XcFpNDP24S5bbKxqS5VHboVuCkVRtykg1ZK"
savingPublicKey="EOS6VKydrWF6xzrUzpJxf2mcRb6xFEXVRfn7uAPByMBfpyjHJVTYr"

stakePrivateKey="5JTgiLciJDSM9m27gtRbFLbDqyqojbWjYdaYS1vV4q4Z5Lgy4vJ"
stakePublicKey="EOS8N4XtexRv8PVigMcfiaQW2ghM56eT3uu3irYuMEGhojKiJLnLA"

tokenPrivateKey="5Jr6bP3nste93kPq589e2cc1QNzBLiaYvtUAB7Hjt31BUaP77gB"
tokenPublicKey="EOS83UwK7verEupDBZcrEnkjHcn6qo1XoUKqRSdtJ325UyiGRQPVK"

vpayPrivateKey="5JLbVTrYsd4dUteBEmgFEFsBz6xr7E9G3bjzENTZqiEHTocL4Qd"
vpayPublicKey="EOS7djBFYXLb22S2Goe2a4QEWoDnaD3LqQzE9pRseKHova5KV8YB8"

# Run local keosd before connecting to external keosd (It look like a bug)
cleos --url=$1  wallet list


password=$(cleos --url=$1 --wallet-url=$2 wallet create | grep -oP '"\K[^"\047]+(?=["\047])')
echo ${password}

cleos --url=$1 --wallet-url=$2 wallet import ${privateKey}

# echo ${password} | cleos --url=$1 --wallet-url=$2 wallet unlock

# Create important system accounts
cleos --url=$1 --wallet-url=$2 wallet import ${bpayPrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.bpay ${bpayPublicKey}

cleos --url=$1 --wallet-url=$2 wallet import ${msigPrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.msig ${msigPublicKey}

cleos --url=$1 --wallet-url=$2 wallet import ${namesPrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.names ${namesPublicKey}

cleos --url=$1 --wallet-url=$2 wallet import ${ramPrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.ram ${ramPublicKey}

cleos --url=$1 --wallet-url=$2 wallet import ${ramfeePrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.ramfee ${ramfeePublicKey}

cleos --url=$1 --wallet-url=$2 wallet import ${savingPrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.saving ${savingPublicKey}

cleos --url=$1 --wallet-url=$2 wallet import ${stakePrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.stake ${stakePublicKey}

cleos --url=$1 --wallet-url=$2 wallet import ${tokenPrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.token ${tokenPublicKey}

cleos --url=$1 --wallet-url=$2 wallet import ${vpayPrivateKey}
cleos --url=$1 --wallet-url=$2 create account eosio eosio.vpay ${vpayPublicKey}

# Set the eosio.token contract. This contract enables you to create, issue, transfer, and get information about tokens. 
cleos --url=$1 --wallet-url=$2 set contract eosio.token contracts/eosio.token
# Set the eosio.msig contract. The msig contract enables and simplifies defining and managing permission levels and performing multi-signature actions.
cleos --url=$1 --wallet-url=$2 set contract eosio.msig contracts/eosio.msig

# Create the SYS currency with a maximum value of 10 billion tokens. Then issue one billion tokens
cleos --url=$1 --wallet-url=$2 push action eosio.token create '[ "eosio", "10000000000.0000 SYS" ]' -p eosio.token
cleos --url=$1 --wallet-url=$2 push action eosio.token issue '[ "eosio", "1000000000.0000 SYS", "memo" ]' -p eosio

cleos --url=$1 --wallet-url=$2 get currency balance eosio.token eosio SYS

# Set the eosio.system contract. This contract provides the actions for pretty much all token-based operational behavior.
cleos --url=$1 --wallet-url=$2 set contract eosio contracts/eosio.system

# Make eosio.msig a privileged account
cleos --url=$1 --wallet-url=$2 push action eosio setpriv '["eosio.msig", 1]' -p eosio@active

# Create staked accounts for block producing
bp1PublicKey="EOS8b5mTfstQxMri2vUrpWaUQK3Gqhr7X5EEFCdRpsz7Zj1MrTw5g"
bp2PublicKey="EOS5jMZUD1BB698jmq2PxMzbgULyhj37pRBr3cqi9vpmGyUk7aZgc"
bp3PublicKey="EOS7XZ6WdHWAt8FzaESQJB6yU7sPFtZ3tt3kHCdPMgdNWKxwf8GYH"

cleos --url=$1 --wallet-url=$2 system newaccount eosio --transfer bp1 ${bp1PublicKey} --stake-net "1000.0000 SYS" --stake-cpu "500.0000 SYS" --buy-ram "0.1000 SYS"
cleos --url=$1 --wallet-url=$2 system newaccount eosio --transfer bp2 ${bp2PublicKey} --stake-net "1000.0000 SYS" --stake-cpu "500.0000 SYS" --buy-ram "0.1000 SYS"
cleos --url=$1 --wallet-url=$2 system newaccount eosio --transfer bp3 ${bp3PublicKey} --stake-net "1000.0000 SYS" --stake-cpu "500.0000 SYS" --buy-ram "0.1000 SYS"

# Create staked accounts for voting
voter1PublicKey="EOS5itkUjPL1em2ZrXm2G6a2cuLSifoUX1E3jo8CXM8egn68AUqjX"
voter2PublicKey="EOS8ghsoYCYbtQx7YUAWm2Zm5eWfJat819QTVs8wdTSC4yo52n8W9"
voter3PublicKey="EOS87aAYfzamC5d7phTU7HFkeMTiKyLBqvDJfzsRo7zQWsaFjqBtP"
voter4PublicKey="EOS6RxXzVSeH8p4kG5h1iDB5MUhwwuPCvxS2gx8YV2LH7DTTbgQdP"

cleos --url=$1 --wallet-url=$2 system newaccount eosio --transfer voter1 ${voter1PublicKey} --stake-net "1000000.0000 SYS" --stake-cpu "1000000.0000 SYS" --buy-ram "0.1000 SYS"
cleos --url=$1 --wallet-url=$2 system newaccount eosio --transfer voter2 ${voter2PublicKey} --stake-net "10000000.0000 SYS" --stake-cpu "10000000.0000 SYS" --buy-ram "0.1000 SYS"
cleos --url=$1 --wallet-url=$2 system newaccount eosio --transfer voter3 ${voter3PublicKey} --stake-net "100000000.0000 SYS" --stake-cpu "100000000.0000 SYS" --buy-ram "0.1000 SYS"
cleos --url=$1 --wallet-url=$2 system newaccount eosio --transfer voter4 ${voter4PublicKey} --stake-net "300000000.0000 SYS" --stake-cpu "300000000.0000 SYS" --buy-ram "0.1000 SYS"

sleep 20

# Once producers have been elected and the minimum number requirements have been met, the eosio account can resign, leaving the eosio.msig account as the only privileged account.
# Resigning is basically involves setting the keys of the eosio.* accounts to null. Use the following command to clear the eosio.* accounts' owner and active keys
cleos --url=$1 --wallet-url=$2 push action eosio updateauth '{"account": "eosio", "permission": "owner", "parent": "", "auth": {"threshold": 1, "keys": [], "waits": [], "accounts": [{"weight": 1, "permission": {"actor": "eosio.prods", "permission": "active"}}]}}' -p eosio@owner
cleos --url=$1 --wallet-url=$2 push action eosio updateauth '{"account": "eosio", "permission": "active", "parent": "owner", "auth": {"threshold": 1, "keys": [], "waits": [], "accounts": [{"weight": 1, "permission": {"actor": "eosio.prods", "permission": "active"}}]}}' -p eosio@active
