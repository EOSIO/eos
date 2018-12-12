# Bios Boot Tutorial

The `bios-boot-tutorial.py` script simulates the EOSIO bios boot sequence.

Prerequisites:

1. eosio binaries (keosd, nodeos)
2. system contracts binaries (built eosio.contracts)
3. python 3.x


Assumming you installed the precompiled eosio binaries, and they were installed in /user/local/bin, 
and assumming you have built the eosio.contracts and they are located in /Users/test/eos/contracts/eosio.contracts/ folder,
and assuming you have installed python 3.x version,
this is the simplest command to start the script:


```bash
$ cd tutorials/bios-boot-tutorial

$ python3 bios-boot-tutorial.py --cleos="/usr/local/bin/cleos --wallet-url http://127.0.0.1:6666 " --nodeos=/usr/local/bin/nodeos --keosd="/usr/local/bin/keosd" --contracts-dir="/Users/test/eos/contracts/eosio.contracts/" -a
```

See [EOSIO Documentation Wiki: Tutorial - Bios Boot](https://github.com/EOSIO/eos/wiki/Tutorial-Bios-Boot-Sequence) for additional information.
