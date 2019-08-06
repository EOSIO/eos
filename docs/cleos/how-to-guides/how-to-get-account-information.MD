# How to get infomation of an eosio account

## Goal

Query infomation of an EOSIO account

## Before you begin

* Install the currently supported version of cleos

* Understand the following:
  * What is an account

## Steps

Execute the command below:

```shell
cleos get account ACCOUNT_NAME
```

You should be see something like below:

```shell
created: 2019-08-05T07:28:06.000
permissions:
     owner     1:    1 EOS8FBXJUfbANf3xeDWPoJxnip3Ych9HjzLBr1VaXRQFdkVAxwLE7
        active     1:    1 EOS8FBXJUfbANf3xeDWPoJxnip3Ych9HjzLBr1VaXRQFdkVAxwLE7
memory:
     quota:     5.365 KiB    used:     4.134 KiB

net bandwidth:
     staked:          0.9989 EOS           (total stake delegated from account to self)
     delegated:       0.0000 EOS           (total staked delegated to account from others)
     used:               553 bytes
     available:        131.1 KiB
     limit:            131.6 KiB

cpu bandwidth:
     staked:          1.0000 EOS           (total stake delegated from account to self)
     delegated:       0.0000 EOS           (total staked delegated to account from others)
     used:             1.147 ms
     available:        46.33 ms
     limit:            47.47 ms

unstaking tokens:
     time of unstake request:  2019-08-06T02:17:17 (funds will be available in 72 hr)
     from net bandwidth:              0.0012 EOS
     from cpu bandwidth:              0.0000 EOS
     total:                           0.0012 EOS

EOS balances:
     liquid:           99.9999 EOS
     staked:            1.9989 EOS
     unstaking:         0.0012 EOS
     total:           102.0000 EOS

producers:     <not voted>
```