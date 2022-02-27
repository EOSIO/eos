# TODO List EOS contract

Example how to use a smart contract with filter

### Build

Before to start, make sure that `eosio.cdt` is installed in your machine.
Then in this folder run the commands bellow.

```bash
$ mkdir build && cd $_
$ cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_FRAMEWORK_PATH=<EOSIO_CDT_PATH>
$ make
```

### Testing

In the build folder run:

```bash
$ eosio-tester -v todo_test.wasm
```

And that's all, happy coding! 💻☕

### Pending
- Adding query contract
- Pushing data to Rabbit Queue
