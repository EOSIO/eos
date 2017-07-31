Contract Developer API
-------------------------------------------

This document will provide the information developers need to write smart contracts in Web Assembly (WASM). It documents all of
the APIs exposed to WebAssembly by the blockchain environment and explains how to write message handlers.

## Background
All accounts have immutable names and can send messages to other accounts. Every message has the following fields:

```
struct Message
   sender      AccountName 
   recipient   AccountName
   notify      AccountName[]
   type        TypeName
   data        Bytes
```

Every message has a `type` which can be viewed like a function name and `data` which could be viewed as arguments. Every account
defines its own message handlers which operate within their own scope with their own private data.  The following methods are
called if they are defined within each `account scope`:


```
foreach($account in sender, recipient and notify) {
   $account: void onValidate_$type_$recipient()
   $account: void onPrecondition_$type_$recipient()
   $account: void onApply_$type_$recipient()
}
```

### Contract Exports 

The WebAssembly can export these methods with something like the following WAST (Web Assembly Text):

```
(module
  (export "onValidate_Transfer_simplecoin" (func $onApply_Transfer_simplecoin))
  (export "onPrecondition_Transfer_simplecoin" (func $onApply_Transfer_simplecoin))
  (export "onApply_Transfer_simplecoin" (func $onApply_Transfer_simplecoin))
  (func $onValidate_Transfer_simplecoin)
  (func $onPrecondition_Transfer_simplecoin)
  (func $onApply_Transfer_simplecoin)
)
```

### Contract Imports
A contract can access the blockchain API through methods provided by the virtual machine. 

These are defined by declaring Web Assembly imports:

```
(module
  (import "env" "assert" (func $assert (param i32 i32)))
  (import "env" "load" (func $load (param i32 i32 i32 i32) (result i32)))
  (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
  (import "env" "readMessage" (func $readMessage (param i32 i32) (result i32)))
  (import "env" "remove" (func $remove (param i32 i32) (result i32)))
  (import "env" "store" (func $store (param i32 i32 i32 i32)))
  ...
)
```

All of the available imports are defined in a C header with documentation for developers such as yourself.
We will discuss these APIs later, but first lets discuss the different message handling functions.

### Message Handler Functions

#### Validate
This function has the form `void onValidate_$type_$recipient()` where the type and recipient are derived from the
message `type` and `recipient` field.  

The purpose of this method is to verify the internal validity of the message data. This is an operation that
can be performed in parallel and its success or failure cached because it cannot read nor write to the database. Generally
speaking this is where your most computationally intensive calculations should be performed. 

For example, if you need to perform expensive cryptographic operations such as hashing or signature verification then they
should be performed here and result in a success or failure.  It is not possible to generate any other output from this method.

If Validate fails then Preconditon and Apply will be skipped and the transaction will not be propagated nor included in a block.

When validate is called all Database APIs are disabled and will fail. 

This step is skipped when replaying irreversibly confirmed blockchain transactions.

#### Precondition
This function has the form `void onPrecondition_$type_$recipient()` where the type and recipient are derived from the
message `type` and `recipient` field.  

The purpose of this method is to perform validation checks that depend upon read-only access to the application state. Like
Validate, precondition checks can only result in a success or failure and may not store or modify state. 

When Precondition is called all Database APIs that would modify state will fail.

#### Apply
This function has the form `void onApply_$type_$recipient()` where the type and recipient are derived from the
message `type` and `recipient` field.  

This method should contiain the minimal code necessary to effect the desired state transition. It can call all database
APIs and generate new transactions to send to other accounts.


### Example

Suppose there is a currency contract with account name `currency` and a message type called `transfer`. The `currency` account code would implement the following methods:

```
void onValidate_transfer_currency();
void onPrecondition_transfer_currency();
void onApply_transfer_currency();
```

Suppose there is also a exchange contract with account name `exchange` that wishes to accept deposits any time someone transfers 
currency to `exchange`.  It would implement the following:

```
void onApply_transfer_currency();
```

In this case `exchange` simply trusts the validation of the `currency` contract and therefore only has to credit the depositor.

Any time the following message was generated all of the methods above would be called (both for `currency` and `exchange`) and if any fail then the entire transaction is discarded and never included in a block.



