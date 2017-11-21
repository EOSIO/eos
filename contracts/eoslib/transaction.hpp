/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/transaction.h>
#include <eoslib/print.hpp>

namespace eosio {

   /**
    * @defgroup transactioncppapi transaction C++ API
    * @ingroup transactionapi
    * @brief Type-safe C++ wrappers for transaction C API
    *
    * @note There are some methods from the @ref transactioncapi that can be used directly from C++
    *
    * @{ 
    */

   class transaction;
   class message {
   public:
      template<typename Payload, typename ...Permissions>
      message(const account_name& code, const func_name& type, const Payload& payload, Permissions... permissions )
         : handle(message_create(code, type, &payload, sizeof(Payload)))
      {
         add_permissions(permissions...);
      }

      template<typename Payload>
      message(const account_name& code, const func_name& type, const Payload& payload )
         : handle(message_create(code, type, &payload, sizeof(Payload)))
      {
      }

      message(const account_name& code, const func_name& type)
         : handle(message_create(code, type, nullptr, 0))
      {
      }

      // no copy constructor due to opaque handle
      message( const message& ) = delete;

      message( message&& msg ) {
         handle = msg.handle;
         msg.handle = invalid_message_handle;
      }

      ~message() {
         if (handle != invalid_message_handle) {
            message_drop(handle);
            handle = invalid_message_handle;
         }
      }

      void add_permissions(account_name account, permission_name permission) {
         message_require_permission(handle, account, permission);
      }

      template<typename ...Permissions>
      void add_permissions(account_name account, permission_name permission, Permissions... permissions) {
         add_permissions(account, permission);
         add_permissions(permissions...);
      }

      void send() {
         assert_valid_handle();
         message_send(handle);
         handle = invalid_message_handle;
      }

   private:
      void assert_valid_handle() {
         assert(handle != invalid_message_handle, "attempting to send or modify a finalized message" );
      }

      message_handle handle;

      friend class transaction;

   };

   class transaction {
   public:
      transaction()
         : handle(transaction_create())
      {}

      // no copy constructor due to opaque handle
      transaction( const transaction& ) = delete;

      transaction( transaction&& trx ) {
         handle = trx.handle;
         trx.handle = invalid_transaction_handle;
      }

      ~transaction() {
         if (handle != invalid_transaction_handle) {
            transaction_drop(handle);
            handle = invalid_transaction_handle;
         }
      }

      void add_scope(account_name scope, bool readOnly = false) {
         assert_valid_handle();
         transaction_require_scope(handle, scope, readOnly ? 1 : 0);
      }

      void add_message(message &msg) {
         assert_valid_handle();
         msg.assert_valid_handle();
         transaction_add_message(handle, msg.handle);
         msg.handle = invalid_message_handle;
      }

      void send() {
         assert_valid_handle();
         transaction_send(handle);
         handle = invalid_transaction_handle;
      }

      transaction_handle get() {
         return handle;
      }

   private:
      void assert_valid_handle() {
         assert(handle != invalid_transaction_handle, "attempting to send or modify a finalized transaction" );
      }

      transaction_handle handle;
   };


 ///@} transactioncpp api

} // namespace eos
