/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/transaction.h>
#include <eoslib/print.hpp>

namespace eos {

   /**
    * @defgroup transactioncppapi Transaction C++ API
    * @ingroup transactionapi
    * @brief Type-safe C++ wrappers for Transaction C API
    *
    * @note There are some methods from the @ref transactioncapi that can be used directly from C++
    *
    * @{ 
    */

   class Transaction;
   class Message {
   public:
      template<typename Payload, typename ...Permissions>
      Message(const AccountName& code, const FuncName& type, const Payload& payload, Permissions... permissions ) 
         : handle(message_create(code, type, &payload, sizeof(Payload)))
      {
         add_permissions(permissions...);
      }

      template<typename Payload>
      Message(const AccountName& code, const FuncName& type, const Payload& payload ) 
         : handle(message_create(code, type, &payload, sizeof(Payload)))
      {
      }

      Message(const AccountName& code, const FuncName& type) 
         : handle(message_create(code, type, nullptr, 0))
      {
      }

      // no copy constructor due to opaque handle
      Message( const Message& ) = delete;

      Message( Message&& msg ) {
         handle = msg.handle;
         msg.handle = InvalidMessageHandle;
      }

      ~Message() {
         if (handle != InvalidMessageHandle) {
            message_drop(handle);
            handle = InvalidMessageHandle;
         }
      }

      void add_permissions(AccountName account, PermissionName permission) {
         message_require_permission(handle, account, permission);
      }

      template<typename ...Permissions>
      void add_permissions(AccountName account, PermissionName permission, Permissions... permissions) {
         add_permissions(account, permission);
         add_permissions(permissions...);
      }

      void send() {
         assert_valid_handle();
         message_send(handle);
         handle = InvalidMessageHandle;
      }

   private:
      void assert_valid_handle() {
         assert(handle != InvalidMessageHandle, "attempting to send or modify a finalized message" );
      }

      MessageHandle handle;

      friend class Transaction;

   };

   class Transaction {
   public:
      Transaction() 
         : handle(transaction_create())
      {}

      // no copy constructor due to opaque handle
      Transaction( const Transaction& ) = delete;

      Transaction( Transaction&& trx ) {
         handle = trx.handle;
         trx.handle = InvalidTransactionHandle;
      }

      ~Transaction() {
         if (handle != InvalidTransactionHandle) {
            transaction_drop(handle);
            handle = InvalidTransactionHandle;
         }
      }

      void addScope(AccountName scope, bool readOnly = false) {
         assert_valid_handle();
         transaction_require_scope(handle, scope, readOnly ? 1 : 0);
      }

      void addMessage(Message &message) {
         assert_valid_handle();
         message.assert_valid_handle();
         transaction_add_message(handle, message.handle);
         message.handle = InvalidMessageHandle;
      }

      void send() {
         assert_valid_handle();
         transaction_send(handle);
         handle = InvalidTransactionHandle;
      }

      TransactionHandle get() {
         return handle;
      }

   private:
      void assert_valid_handle() {
         assert(handle != InvalidTransactionHandle, "attempting to send or modify a finalized transaction" );
      }

      TransactionHandle handle;
   };


 ///@} transactioncpp api

} // namespace eos
