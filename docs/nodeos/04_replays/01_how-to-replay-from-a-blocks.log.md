# Hot To Replay from a Blocks Log

Once you have obtained a copy of the blocks.log file which you wish to replay the blockchain from, copy it to your data/blocks directory, backing up any existing contents if you wish to keep them.

location                | name               | action
----------------------- | ------------------ | ------
data/blocks             | blocks.index       | remove
data/blocks             | blocks.log         | replace this file with the block.log you want to replay
data/blocks/reversible  | forkdb.dat         | remove
data/blocks/reversible  | shared_memory.bin  | remove
data/blocks/reversible  | shared_memory.meta | remove

You can use `blocks-dir = "blocks"` in the `config.ini` file, or use the `--blocks-dir` command line option, to specify where to find the `blocks.log` file to replay.

```sh
$ nodeos --replay-blockchain \
  -e -p eosio \
  --plugin eosio::producer_plugin  \
  --plugin eosio::chain_api_plugin \
  --plugin eosio::http_plugin      \
  >> nodeos.log 2>&1 &
```
