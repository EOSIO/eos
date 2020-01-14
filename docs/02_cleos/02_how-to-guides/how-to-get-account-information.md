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
created: 2018-06-01T12:00:00.000
privileged: true
permissions:
     owner     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
        active     1:    1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
memory:
     quota:       unlimited  used:     3.004 KiB

net bandwidth:
     used:               unlimited
     available:          unlimited
     limit:              unlimited

cpu bandwidth:
     used:               unlimited
     available:          unlimited
     limit:              unlimited
```

[[info]]
| Depends on the EOSIO network you are connected, you might see different fields associated with an account. That depends on which system contract has been deployed tp the network.