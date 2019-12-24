<!-- # How to replay from a blocks.log file -->

Once you have obtained a copy of the `blocks.log` file which you wish to replay the blockchain from, copy it to your `data/blocks` directory, backing up any existing contents if you wish to keep them, and remove the `blocks.index`, `forkdb.dat`, `shared_memory.bin`, and `shared_memory.meta`.

The table below sumarizes the actions you should take for each of the files enumerated above:

Folder name             | File name          | Action
----------------------- | ------------------ | ------
data/blocks             | blocks.index       | Remove
data/blocks             | blocks.log         | Replace this file with the `block.log` you want to replay
data/blocks/reversible  | forkdb.dat         | Remove
data/blocks/reversible  | shared_memory.bin  | Remove
data/blocks/reversible  | shared_memory.meta | Remove

You can use `blocks-dir = "blocks"` in the `config.ini` file, or use the `--blocks-dir` command line option, to specify where to find the `blocks.log` file to replay.

```sh
$ nodapifiny --replay-blockchain \
  -e -p apifiny \
  --plugin apifiny::producer_plugin  \
  --plugin apifiny::chain_api_plugin \
  --plugin apifiny::http_plugin      \
  >> nodapifiny.log 2>&1 &
```
