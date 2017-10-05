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
         : handle(messageCreate(code, type, &payload, sizeof(Payload)))
      {
         addPermissions(permissions...);
      }

      template<typename Payload>
      Message(const AccountName& code, const FuncName& type, const Payload& payload ) 
         : handle(messageCreate(code, type, &payload, sizeof(Payload)))
      {
      }

      Message(const AccountName& code, const FuncName& type) 
         : handle(messageCreate(code, type, nullptr, 0))
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
            messageDrop(handle);
            handle = InvalidMessageHandle;
         }
      }

      void addPermissions(AccountName account, PermissionName permission) {
         messageRequirePermission(handle, account, permission);
      }

      template<typename ...Permissions>
      void addPermissions(AccountName account, PermissionName permission, Permissions... permissions) {
         addPermissions(account, permission);
         addPermissions(permissions...);
      }

      void send() {
         assertValidHandle();
         messageSend(handle);
         handle = InvalidMessageHandle;
      }

   private:
      void assertValidHandle() {
         assert(handle != InvalidMessageHandle, "attempting to send or modify a finalized message" );
      }

      MessageHandle handle;

      friend class Transaction;

   };

   class Transaction {
   public:
      Transaction() 
         : handle(transactionCreate())
      {}

      // no copy constructor due to opaque handle
      Transaction( const Transaction& ) = delete;

      Transaction( Transaction&& trx ) {
         handle = trx.handle;
         trx.handle = InvalidTransactionHandle;
      }

      ~Transaction() {
         if (handle != InvalidTransactionHandle) {
            transactionDrop(handle);
            handle = InvalidTransactionHandle;
         }
      }

      void addScope(AccountName scope, bool readOnly = false) {
         assertValidHandle();
         transactionRequireScope(handle, scope, readOnly ? 1 : 0);
      }

      void addMessage(Message &message) {
         assertValidHandle();
         message.assertValidHandle();
         transactionAddMessage(handle, message.handle);
         message.handle = InvalidMessageHandle;
      }

      void send() {
         assertValidHandle();
         transactionSend(handle);
         handle = InvalidTransactionHandle;
      }

      TransactionHandle get() {
         return handle;
      }

   private:
      void assertValidHandle() {
         assert(handle != InvalidTransactionHandle, "attempting to send or modify a finalized transaction" );
      }

      TransactionHandle handle;
   };


 ///@} transactioncpp api

} // namespace eos
