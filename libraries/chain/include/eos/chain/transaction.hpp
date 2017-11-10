/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/types.hpp>
#include <eos/chain/message.hpp>

#include <numeric>

namespace eosio { namespace chain {

   struct permission_level {
      account_name    actor;
      permission_name level;
   };

   /**
    *  An action is performed by an actor, aka an account. It may
    *  be created explicitly and authorized by signatures or might be
    *  generated implicitly by executing application code. 
    *
    *  This follows the design pattern of React Flux where actions are
    *  named and then dispatched to one or more action handlers (aka stores).
    *  In the context of eosio, every action is dispatched to the handler defined
    *  by account 'scope' and function 'name', but the default handler may also
    *  forward the action to any number of additional handlers. Any application
    *  can write a handler for "scope::name" that will get executed if and only if
    *  this action is forwarded to that application.
    *
    *  Each action may require the permission of specific actors. Actors can define
    *  any number of permission levels. The actors and their respective permission
    *  levels are declared on the action and validated independently of the executing
    *  application code. An application code will check to see if the required permissions
    *  were properly declared when it executes.
    */
   struct action {
      account_name               scope;
      function_name              name;
      vector<permission_level>   permissions;
      bytes                      data;
   };

   struct action_notice : public action {
      account_name receiver;
   };


   /**
    * When a transaction is referenced by a block it could imply one of several outcomes which 
    * describe the state-transition undertaken by the block producer. 
    */
   struct transaction_receipt {
      enum status_enum {
         generated = 0, ///< created, not yet known to be valid or invalid (no state transition)
         executed  = 1, ///< succeed, no error handler executed
         soft_fail = 2, ///< objectively failed (not executed), error handler executed 
         hard_fail = 3  ///< objectively failed and error handler objectively failed thus no state change
      };

      fc::enum_type<uint8_t,status_enum>  status;
      transaction_id_type                 id;
   };

   /**
    *  The transaction header contains the fixed-sized data
    *  associated with each transaction. It is separated from
    *  the transaction body to facilitate partial parsing of
    *  transactions without requiring dynamic memory allocation.
    *
    *  All transactions have an expiration time after which they
    *  may no longer be included in the blockchain. Once a block
    *  with a block_header::timestamp greater than expiration is 
    *  deemed irreversible, then a user can safely trust the transaction
    *  will never be included. 
    *
    *  Transactions are divided into memory regions, the default region is 0.
    *  Each region is an independent blockchain, it is included as routing
    *  information for inter-blockchain communication. A contract in this
    *  region might generate or authorize a transaction intended for a foreign
    *  region.
    */
   struct transaction_header {
      time_point_sec         expiration;   ///< the time at which a transaction expires
      uint16_t               region        = 0; ///< the computational memory region this transaction applies to.
      uint16_t               ref_block_num = 0; ///< specifies a block num in the last 2^16 blocks.
      uint32_t               ref_block_prefix = 0; ///< specifies the lower 32 bits of the blockid at get_ref_blocknum

      /**
       * @return the absolute block number given the relative ref_block_num
       */
      block_num_type get_ref_blocknum( block_num_type head_blocknum )const {
         return ((head_blocknum/0xffff)*0xffff) + head_blocknum%0xffff;
      }
   };

   /**
    *  A transaction consits of a set of messages which must all be applied or
    *  all are rejected. These messages have access to data within the given
    *  read and write scopes.
    */
   struct transaction : public transaction_header {
      vector<account_name>   read_scope;
      vector<account_name>   write_scope;
      vector<action>         actions;

      digest_type  digest()const;
   };

   struct signed_transaction : public transaction {
      vector<signature_type> signatures;

      transaction_id_type id()const;
   };

   /**
    *  When a transaction is generated it can be scheduled to occur
    *  in the future. It may also fail to execute for some reason in
    *  which case the sender needs to be notified. When the sender
    *  sends a transaction they will assign it an ID which will be
    *  passed back to the sender if the transaction fails for some
    *  reason.
    */
   struct deferred_transaction : public transaction
   {
      uint64_t       id; /// ID assigned by sender of generated, accessible via WASM api when executing normal or error
      account_name   sender; /// receives error handler callback
      time_point_sec execute_after; /// delayed exeuction
   };

} } // eosio::chain

