#pragma once
#include <eoslib/transaction.h>
#include <eoslib/print.hpp>

namespace eos {

   /**
    * @defgroup transactioncppapi Transaction C++ API
    * @ingroup transactionapi
    * @brief Type-safe C++ wrapers for Transaction C API
    *
    * @note There are some methods from the @ref transactioncapi that can be used directly from C++
    *
    * @{ 
    */

   class Transaction {
   public:
      Transaction() 
         : handle(transactionCreate())
      {}

      // no copy construtor due to opaque handle
      Transaction( const Transaction& ) = delete;

      Transaction( const Transaction&& trx ) {
         handle = trx.handle;
         trx.handle = InvalidTransactionHandle;
      }

      ~Transaction() {
         if (handle != InvalidTransactionHandle) {
            transactionDrop(handle);
            handle = InvalidTransactionHandle;
         }
      }

      void addScope(AccountName scope, bool readOnly) {
         assertValidHandle();
         transactionAddScope(handle, scope, readOnly ? 1 : 0);
      }

      template<typename P, typename T>
      void addMessage(AccountName code, FuncName name, const P& permissions, const T& data ) {
         assertValidHandle();
         transactionResetMessage(handle);
         transactionSetMessageDestination(handle, code, name);
         for (const auto &p: permissions) {
            transactionAddMessagePermission(handle, p);
         }
         transactionPushMessage(handle, &data, sizeof(data));
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
