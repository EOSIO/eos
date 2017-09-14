# Developing Smart Contracts / Decentralized Applications with EOS.IO

This tutorial is targeted toward those who would like to get started building decentralized applications or
smart contracts using the EOS.IO software development kit. It will cover writing contracts/applications in C++
and uploading them to the blockchain.  It will not cover how to build or deploy interactive web interfaces.

For the rest of this tutorial we will use the term "contract" to refer to the code that powers decentralized applications.

## Required Background Knowledge

This tutorial assumes that you have basic understanding of how to use 'eosd' and 'eosc' to setup a node
and deploy the example contracts.  If you have not yet successfully followed that tutorial, then do that
first and come back.  

### C / C++ Experience Required 

EOS.IO based blockchains execute user generated applications and code using Web Assembly (WASM). WASM is an
emerging web standard with widespread support of Google, Microsoft, Apple, and others.  At the moment the
most mature toolchain for building applications that compile to WASM is clang/llvm with their C/C++ compiler.

Other toolchains in development by 3rd parties include: Rust, Python, and Solidiity. While these other languages
may appear simpler, their performance will likely impact the scale of application you can build. We expect that
C++ will be the best language for developing high-performance and secure smart contacts.

If you are not experienced with C/C++, then you should learn the basics first and then come back.

### Linux / Mac OS X Command Line

The EOS.IO software only officially supports unix environments and building with clang.

## Basics of Smart Contracts / Decentralized Applications

Fundamentally every contract is a state machine that responds to signed user actions. These contracts are
uploaded to the blockchain as pre-compiled Web Assembly (WASM) files with an ABI (Application Binary Interface).

To keep things simple we have created a tool called `eoscpp` which can be used to bootstrap a new contract. For this
to work we assume you have installed the eosio/eos code installed and that ${CMAKE_INSTALL_PREFIX}/bin is in your path.


```bash
$ eoscpp -n hello
$ cd hello
$ ls
```

The above will create a new empty project in the './hello' folder with three files:

```bash
hello.abi hello.hpp hello.cpp
```

Let's take a look at the simplest possible contract:

```bash
cat hello.cpp

#include <hello.hpp>

/**
 *  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
 *  call these methods.
 */
extern "C" {

    /**
     *  This method is called once when the contract is published or updated.
     */
    void init()  {
       eos::print( "Init World!\n" );
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       eos::print( "Hello World: ", eos::Name(code), "->", eos::Name(action), "\n" );
    }

} // extern "C"
```

This contract implements the two entry points, `init` and `apply`. All it does is log the messages delivered and makes no other checks. Anyone can deliver any message at any time provided the block producers allow it.  Absent any required signatures, the contract will be billed for the bandwidth consumed.

You can compile this contract into a text version of WASM (.wast) like so:

```bash
$ eoscpp -o hello.wast hello.cpp
```

### Deploying your Contract

Now that you have compiled your application it is time to deploy it. This will require you to do the following first:

1. start eosd with wallet plugin enabled
2. create a wallet & import keys for at least one account
3. keep your wallet unlocked

Assuming your wallet is unlocked and has keys for `${account}`, you can now upload this contract to the blockchain with the following command:

```bash
$ eosc set contract ${account} hello.wast hello.abi
Reading WAST...
Assembling WASM...
Publishing contract...
{
  "transaction_id": "1abb46f1b69feb9a88dbff881ea421fd4f39914df769ae09f66bd684436443d5",
  "processed": {
    "refBlockNum": 144,
    "refBlockPrefix": 2192682225,
    "expiration": "2017-09-14T05:39:15",
    "scope": [
      "eos",
      "${account}"
    ],
    "signatures": [
      "2064610856c773423d239a388d22cd30b7ba98f6a9fbabfa621e42cec5dd03c3b87afdcbd68a3a82df020b78126366227674dfbdd33de7d488f2d010ada914b438"
    ],
    "messages": [{
        "code": "eos",
        "type": "setcode",
        "authorization": [{
            "account": "${account}",
            "permission": "active"
          }
        ],
        "data": "0000000080c758410000f1010061736d0100000001110460017f0060017e0060000060027e7e00021b0203656e76067072696e746e000103656e76067072696e7473000003030202030404017000000503010001071903066d656d6f7279020004696e69740002056170706c7900030a20020600411010010b17004120100120001000413010012001100041c00010010b0b3f050041040b04504000000041100b0d496e697420576f726c64210a000041200b0e48656c6c6f20576f726c643a20000041300b032d3e000041c0000b020a000029046e616d6504067072696e746e0100067072696e7473010004696e697400056170706c790201300131010b4163636f756e744e616d65044e616d6502087472616e7366657200030466726f6d0b4163636f756e744e616d6502746f0b4163636f756e744e616d6506616d6f756e740655496e743634076163636f756e740002076163636f756e74044e616d650762616c616e63650655496e74363401000000b298e982a4087472616e736665720100000080bafac6080369363401076163636f756e7400076163636f756e74"
      }
    ],
    "output": [{
        "notify": [],
        "deferred_transactions": []
      }
    ]
  }
}
```

If you are monitoring the output of your eosd process you should see:

```bash
...] initt generated block #188249 @ 2017-09-13T22:00:24 with 0 trxs  0 pending
Init World!
Init World!
Init World!
```

You will notice the lines "Init World!" are executed 3 times, this isn't a mistake.  When the blockchain is processing transactions the following happens:

   1. eosd receives a new transaction
     - creates a temporary session
     - attempts to apply the transaction
     - **succeeds and prints "Init World!"**
     - or fails undoes the changes (potentially failing after printing "Init World!")
   2. eosd starts to produce a block
     - undoes all pending state
     - pushes all transactions as it builds the block
     - **prints "Init World!" a second time**
     - finishes building the block
     - undoes all of the temporary changes while creating block
   3. eosd pushes the generated block as if it received it from the network
     - **prints "Init World!" a third time**

At this point your contract is ready to start receiving messages. Since the default message handler accepts all messages we can send it
anything we want. Let's try sending it an empty message:

```bash
$ eosc push message ${account} hello '"abcd"' --scope ${account}
```

This command will send the message "hello" with binary data represented by the hex string "abcd". Note, in a bit we will show how to define the ABI so that
you can replace the hex string with a pretty, easy-to-read, JSON object. For now we merely want to demonstrate how the message type "hello" is dispatched to
account.


The result is:
```bash
{
  "transaction_id": "69d66204ebeeee68c91efef6f8a7f229c22f47bcccd70459e0be833a303956bb",
  "processed": {
    "refBlockNum": 57477,
    "refBlockPrefix": 1051897037,
    "expiration": "2017-09-13T22:17:04",
    "scope": [
      "${account}"
    ],
    "signatures": [],
    "messages": [{
        "code": "${account}",
        "type": "hello",
        "authorization": [],
        "data": "abcd"
      }
    ],
    "output": [{
        "notify": [],
        "deferred_transactions": []
      }
    ]
  }
}
```

If you are following along in `eosd` then you should have seen the following scroll by the screen:

```bash
Hello World: ${account}->hello
Hello World: ${account}->hello
Hello World: ${account}->hello
```

Once again your contract was executed and undone twice before being applied the 3rd time as part of a generated block.  

### Message Name Restrictions

Message types (eg. "hello") are actually base32 encoded 64 bit integers. This means they are limited to the charcters a-z, 1-5, and '.' for the first
12 charcters and if there is a 13th character then it is restricted to the first 16 characters ('.' and a-p).


### ABI - Application Binary Interface

The ABI is a JSON-based description on how to convert user actions between their JSON and Binary representations. The ABI also
describes how to convert the database state to/from JSON. Once you have described your contract via an ABI then developers and
users will be able to interact with your contract seemlessly via JSON.

We are working on tools that will automate the generation of the ABI from the C++ source code, but for the time being you may have
to generate it manually. 

Here is an example of what the skeleton contract ABI looks like:

```json
{
  "types": [{
      "newTypeName": "AccountName",
      "type": "Name"
    }
  ],
  "structs": [{
      "name": "transfer",
      "base": "",
      "fields": {
        "from": "AccountName",
        "to": "AccountName",
        "amount": "UInt64"
      }
    },{
      "name": "account",
      "base": "",
      "fields": {
        "account": "Name",
        "balance": "UInt64"
      }
    }
  ],
  "actions": [{
      "action": "transfer",
      "type": "transfer"
    }
  ],
  "tables": [{
      "table": "account",
      "type": "account",
      "indextype": "i64",
      "keynames" : ["account"],
      "keytypes" : ["Name"]
    }
  ]
}
```

You will notice that this ABI defines an action `transfer` of type `transfer`.  This tells EOS.IO that when `${account}->transfer` message is seen that the payload is of type
`transfer`.  The type `transfer` is defined in the `structs` array in the object with `name` set to `"transfer"`.  

```json
...
  "structs": [{
      "name": "transfer",
      "base": "",
      "fields": {
        "from": "AccountName",
        "to": "AccountName",
        "amount": "UInt64"
      }
    },{
...
```

It has several fields, including `from`, `to` and `amount`.  These fields have the coresponding types `AccountName`, `AccountName`, and `UInt64`.  `AccountName` is defined as a typedef in
the `types` array to `Name`, which is a built in type used to encode a uint64_t as base32 (eg account names).

```json
{
  "types": [{
      "newTypeName": "AccountName",
      "type": "Name"
    }
  ],
...
```

Now that we have reviewed the ABI defined by the skeleton, we can construct a message call for `transfer`:

```bash
eosc push message ${account} transfer '{"from":"currency","to":"inita","amount":50}' --scope initc
2570494ms thread-0   main.cpp:797                  operator()           ] Converting argument to binary...
{
  "transaction_id": "b191eb8bff3002757839f204ffc310f1bfe5ba1872a64dda3fc42bfc2c8ed688",
  "processed": {
    "refBlockNum": 253,
    "refBlockPrefix": 3297765944,
    "expiration": "2017-09-14T00:44:28",
    "scope": [
      "initc"
    ],
    "signatures": [],
    "messages": [{
        "code": "initc",
        "type": "transfer",
        "authorization": [],
        "data": {
          "from": "currency",
          "to": "inita",
          "amount": 50
        },
        "hex_data": "00000079b822651d000000008040934b3200000000000000"
      }
    ],
    "output": [{
        "notify": [],
        "deferred_transactions": []
      }
    ]
  }
}
```

If you observe the output of `eosd` you should see:

```
Hello World: ${account}->transfer
Hello World: ${account}->transfer
Hello World: ${account}->transfer
```

## Processing Arguments of Transfer Message

According to the ABI the transfer message has the format: 

```json
	 "fields": {
		 "from": "AccountName",
		 "to": "AccountName",
		 "amount": "UInt64"
	 }
```

We also know that AccountName -> Name -> UInt64 which means that the binary representation of the message is the same as:

```c
struct transfer {
    uint64_t from;
    uint64_t to;
    uint64_t amount;
};
```

The EOS.IO C API provides access to the message payload via the Message API:

```
uint32_t messageSize();
uint32_t readMessage( void* msg, uint32_t msglen );
```

Let's modify `hello.cpp` to print out the content of the message:

```c
#include <hello.hpp>

/**
 *  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
 *  call these methods.
 */
extern "C" {

    /**
     *  This method is called once when the contract is published or updated.
     */
    void init()  {
       eos::print( "Init World!\n" );
    }

    struct transfer {
       uint64_t from;
       uint64_t to;
       uint64_t amount;
    };

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       eos::print( "Hello World: ", eos::Name(code), "->", eos::Name(action), "\n" );
       if( action == N(transfer) ) {
          transfer message;
          static_assert( sizeof(message) == 3*sizeof(uint64_t), "unexpected padding" );
          auto read = readMessage( &message, sizeof(message) );
          assert( read == sizeof(message), "message too short" );
          eos::print( "Transfer ", message.amount, " from ", eos::Name(message.from), " to ", eos::Name(message.to), "\n" );
       }
    }

} // extern "C"
```

Then we can recompile and deploy it with:

```bash
eoscpp -o hello.wast hello.cpp 
eosc set contract ${account} hello.wast hello.abi
```

`eosd` will call init() again because of the redeploy

```bash
Init World!
Init World!
Init World!
```

Then we can execute transfer:

```bash
$ eosc push message ${account} transfer '{"from":"currency","to":"inita","amount":50}' --scope ${account}
{
  "transaction_id": "a777539b7d5f752fb40e6f2d019b65b5401be8bf91c8036440661506875ba1c0",
  "processed": {
    "refBlockNum": 20,
    "refBlockPrefix": 463381070,
    "expiration": "2017-09-14T01:05:49",
    "scope": [
      "${account}"
    ],
    "signatures": [],
    "messages": [{
        "code": "${account}",
        "type": "transfer",
        "authorization": [],
        "data": {
          "from": "currency",
          "to": "inita",
          "amount": 50
        },
        "hex_data": "00000079b822651d000000008040934b3200000000000000"
      }
    ],
    "output": [{
        "notify": [],
        "deferred_transactions": []
      }
    ]
  }
}
```

And on `eosd` we should see the following output:

```bash
Hello World: ${account}->transfer
Transfer 50 from currency to inita
Hello World: ${account}->transfer
Transfer 50 from currency to inita
Hello World: ${account}->transfer
Transfer 50 from currency to inita
```

### Using C++ API to Read Messages
So far we used the C API because it is the lowest level API that is directly exposed by EOS.IO to the WASM virtual machine. Fortunately, eoslib provides a higher level API that
removes much of the boiler plate. 

```c
/// eoslib/message.hpp
namespace eos {
	 template<typename T>
	 T currentMessage();
}
```

We can update `hello.cpp` to be more concise as follows:

```c
#include <hello.hpp>

/**
 *  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
 *  call these methods.
 */
extern "C" {

    /**
     *  This method is called once when the contract is published or updated.
     */
    void init()  {
       eos::print( "Init World!\n" );
    }

    struct transfer {
       eos::Name from;
       eos::Name to;
       uint64_t amount;
    };

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       eos::print( "Hello World: ", eos::Name(code), "->", eos::Name(action), "\n" );
       if( action == N(transfer) ) {
          auto message = eos::currentMessage<transfer>();
          eos::print( "Transfer ", message.amount, " from ", message.from, " to ", message.to, "\n" );
       }
    }

} // extern "C"

```

You will notice that we updated the `transfer` struct to use the `eos::Name` type directly, and then condenced the checks around `readMessage` to a single call to `currentMessage`.

After compiling and uploading it you should get the same results as the C version.

## Requiring Sender Authority to Transfer

One of the most common requirements of any contract is to define who is allowed to perform the action. In the case of a curency transfer, we want require that the account defined by the `from` parameter signs off on the message. 

The EOS.IO software will take care of enforcing and validating the signatures, all you need to do is require the necessary authority.

```c
    ...
    void apply( uint64_t code, uint64_t action ) {
       eos::print( "Hello World: ", eos::Name(code), "->", eos::Name(action), "\n" );
       if( action == N(transfer) ) {
          auto message = eos::currentMessage<transfer>();
          eos::requireAuth( message.from );
          eos::print( "Transfer ", message.amount, " from ", message.from, " to ", message.to, "\n" );
       }
    }
    ...
```

After building and deploying we can attempt to transfer again:

```bash
 eosc push message ${account} transfer '{"from":"initb","to":"inita","amount":50}' --scope ${account}
 1881603ms thread-0   main.cpp:797                  operator()           ] Converting argument to binary...
 1881630ms thread-0   main.cpp:851                  main                 ] Failed with error: 10 assert_exception: Assert Exception
 status_code == 200: Error
 : 3030001 tx_missing_auth: missing required authority
 Transaction is missing required authorization from initb
     {"acct":"initb"}
         thread-0  message_handling_contexts.cpp:19 require_authorization
...
```

If you look on `eosd` you will see this:
```
Hello World: initc->transfer
1881629ms thread-0   chain_api_plugin.cpp:60       operator()           ] Exception encountered while processing chain.push_transaction:
...
```

This shows that it attempted to apply your transaction, printed the initial "Hello World" and then aborted when `eos::requireAuth` failed to find authorization of account `initb`.

We can fix that by telling `eosc` to add the required permission:

```bash
 eosc push message ${account} transfer '{"from":"initb","to":"inita","amount":50}' --scope ${account} --permission initb@active
```

The `--permission` command defines the account and permission level, in this case we use the `active` authority which is the default.

This time the transfer should have worked like we saw before.

## Aborting a Message on Error

A large part of contract development is verifying preconditions, such that the amount transferred is greater than 0. If a user attempts to execute an invalid action, then
the contract must abort and any changes made get automatically reverted.

```c
    ...
    void apply( uint64_t code, uint64_t action ) {
       eos::print( "Hello World: ", eos::Name(code), "->", eos::Name(action), "\n" );
       if( action == N(transfer) ) {
          auto message = eos::currentMessage<transfer>();
          assert( message.amount > 0, "Must transfer an amount greater than 0" );
          eos::requireAuth( message.from );
          eos::print( "Transfer ", message.amount, " from ", message.from, " to ", message.to, "\n" );
       }
    }
    ...
```
We can now compile, deploy, and attempt to execute a transfer of 0:

```bash
 $ eoscpp -o hello.wast hello.cpp
 $ eosc set contract ${account} hello.wast hello.abi
 $ eosc push message ${account} transfer '{"from":"initb","to":"inita","amount":0}' --scope initc --permission initb@active
 3071182ms thread-0   main.cpp:851                  main                 ] Failed with error: 10 assert_exception: Assert Exception
 status_code == 200: Error
 : 10 assert_exception: Assert Exception
 test: assertion failed: Must transfer an amount greater than 0
```

