## Debugging EOS.IO Smart Contracts

The debug plugin allows for the execution of EOS.IO smart contracts that have been compiled using the native C++ compiler.
The general approach is to enable native compilation of your contract, configure a local nodeos instance to load the debug_plugin, and configure the debug_plugin with the defaults of your native contract. You can then launch nodeos in your favorite debugger, set breakpoints in your contract and use the debugger to step through your contract.

### How to debug your contract

1. Enable native compilation of your contract. Assuming you are using the EOS.IO cmake system, this is easily done by setting  the the DEBUG_FLAG option on the add_wast_executable macro and rebuilding. In addition to building the wasm targets for that contract it will also build a shared library version using your native compiler. For example, to debug the currency contract, modify the currency contract's build file (contracts/currency/CMakeLists.txt) as follows:

<pre>
file(GLOB ABI_FILES "*.abi")
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

add_wast_executable(TARGET currency
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}"
  LIBRARIES libc++ libc eosiolib
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
  <b>DEBUG_FLAG true</b>
)
</pre>

2. Rebuild using the eosio_build.sh script
3. Modify a config.ini file for a single-producer, standalone nodeos node to load the debug_plugin and configure it for your contract. Here is a sample file with debugging enabled for the currency contract:
<pre>
genesis-json = ./genesis.json
block-log-dir = blocks
readonly = 0
send-whole-blocks = true
http-server-address = 127.0.0.1:8888
p2p-listen-endpoint = 0.0.0.0:9876
p2p-server-address = localhost:9876
allowed-connection = any
required-participation = true
private-key = ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"]
producer-name = eosio
plugin = eosio::producer_plugin
plugin = eosio::chain_api_plugin
plugin = eosio::account_history_api_plugin
plugin = eosio::wallet_api_plugin
plugin = eosio::wallet_plugin

<b>plugin = eosio::debug_plugin
debug-account-name = currency
debug-action-name = issue
debug-action-name = transfer
debug-library-name = /home/eos_dev/eos/build/contracts/currency/libcurrency_deb.so</b>
</pre>
4. Load nodeos into your debugger, set a breakpoint in your contract, and run with the normal options for your standalone node.  Because your contract will be dynamically loaded as a shared library, it may be tricky to specify breakpoints in your contract until after the blockchain's configuration occurs. An easy way around this is to set a breakpoint on the contract's global apply() function and then add other breakpoints after it is triggered. An example using gdb and a breakpoint on apply(), looks like this:
<pre>
$ <b>gdb ./programs/nodeos/nodeos</b>
GNU gdb (Ubuntu 7.11.1-0ubuntu1~16.5) 7.11.1
Reading symbols from ./programs/nodeos/nodeos...done.
(gdb) <b>break apply</b>
Function "apply" not defined.
Make breakpoint pending on future shared library load? (y or [n]) y
Breakpoint 1 (apply) pending.
(gdb) <b>run --enable-stale-production true --data-dir my_tn_data_00</b>
</pre>
5. Your nodeos node should now be running in the debugger and will break when the contract's apply function is called. In another window you should do all the normal set up functions that are needed on a new eosio blockchain instance (loading a system contract, issuing tokens, creating accounts, setting the contract on the account you want to test). Note, that you still need to load the wast file via "set contract" even though we are actually running the code natively. The first time you issue an action that you have registered in the config.ini for debugging, the breakpoint on your apply() function should trigger.
6. Debug as you would any C++ program.

### Limitations

The differences between the 32-bit wasm environment and the native 64 bit C++ compiler mean that some things will not work in the native debugging environment and some will work differently. Some important differences are:
* Most of the platform-specific types, such as int, long, and size_t, will have different sizes and should not be used in the external APIs of your contract and should be avoided in general.
* The standard libraries may act differently (libc, libc++, etc.)
* A natively compiled contract uses the program's heap and is not limited by the wasm VM like traditional contracts
* Heap memory is not initialized to zero before a contract is loaded
* The stack sizes will be different
* Initializers will not be called in wasm, but will in native mode
* In native mode, any variables will not have their values reset between action executions. In wasm mode, each action execution is like running a program from scratch, whereas in native mode each action execution occurs in the same context.
* Floating point types may act differently
* Memory access violations and other low-level C++ problems will not be handled by nodeos but will bring the process down (but can now be studied in the debugger)
* For a variety of reasons, some intrinsics (calls that normally cross the wasm/native space) are not currently implemented. Any intrinsics not implemented will result in missing symbols at link time of your contract's shared library.
* In a given process, only one contract's code can be natively debugged at a time.

### Warnings

* Only run natively-compiled contracts in standalone nodes
* Do not configure the debug_plugin and natively-compiled contracts in nodes connected to public or test networks
