# Bios Boot Sequence Tutorial Docker Compose Implementaion 
Create an automatic imitation of a Bios boot sequence using Docker Compose. (https://github.com/EOSIO/eos/wiki/Tutorial-Bios-Boot-Sequence)

## Docker Compose Network Diagram
![Docker Compose Network Diagram](./images/figure1.jpeg)

## Boot Sequence
1. [bios_tool] Create core system accounts.
```
eosio.bpay, eosio.msig, eosio.names, eosio.ram, eosio.ramfee, eosio.saving, eosio.stake, eosio.token, eosio.vpay
```
2. [bios_tool] Create and publish eosio.token and eosio.msig contracts.
3. [bios_tool] Create SYS tokens, and eosio issues 1,000,000,000.0000 SYS tokens
3. [bios_tool] Create BP Accounts and send SYS tokens to each BP.
```
bp1, bp2, bp3
```
4. [bios_tool] Create voting accounts, and send SYS tokens to each voters.
```
voter1, voter2, voter3, voter4
```
5. [bp1_tool] Register bp1 account as a BP.
6. [bp2_tool] Register bp2 account as a BP.
7. [bp3_tool] Register bp3 account as a BP.
8. [voter_tool] voter1 votes for bp1, bp2, and bp3.
9. [voter_tool] voter2 votes for bp1 and bp2.
10. [voter_tool] voter3 votes for bp1 and bp2.
11. [voter_tool] voter4 votes for bp1.
12. [bios_tool] Remove eosio owner and active keys to allow block production of the BPs.
13. [bp1_tool, bp2_tool, bp3_tool] BP claims their initial reward after 60 seconds since the first block, then claims rewards every 24hours.

## Initial Setup
```bash
# Creates the datas directory
./init.sh
```

## How to Start the sequence tutorial
```bash
docker-compose up
```

## Restart Sequence
```bash
# Removes all the files in the datas directory
./restart.sh 
```
