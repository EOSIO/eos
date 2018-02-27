/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/transaction.h>
#include <eosiolib/action.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/serialize.hpp>
#include <vector>

namespace eosio {

   /**
    * @defgroup transactioncppapi Transaction C++ API
    * @ingroup transactionapi
    * @brief Type-safe C++ wrappers for transaction C API
    *
    * @note There are some methods from the @ref transactioncapi that can be used directly from C++
    *
    * @{
    */

   class transaction_header {
   public:
      transaction_header( eosio_time exp = now() + 60, region_id r = 0 )
         :expiration(exp),region(r)
      {}

      eosio_time      expiration;
      region_id       region;
      uint16_t        ref_block_num;
      uint32_t        ref_block_prefix;
      unsigned_int    net_usage_words = 0UL; /// number of 8 byte words this transaction can serialize into after compressions
      unsigned_int    kcpu_usage = 0UL; /// number of CPU usage units to bill transaction for
      unsigned_int    delay_sec = 0UL; /// number of CPU usage units to bill transaction for

      EOSLIB_SERIALIZE( transaction_header, (expiration)(region)(ref_block_num)(ref_block_prefix)(net_usage_words)(kcpu_usage)(delay_sec) )
   };

   class transaction : public transaction_header {
   public:
      transaction(eosio_time exp = now() + 60, region_id r = 0) : transaction_header( exp, r ) {}

      void send(uint64_t sender_id, account_name payer) const {
         auto serialize = pack(*this);
         send_deferred(sender_id, payer, serialize.data(), serialize.size());
      }

      vector<action>  context_free_actions;
      vector<action>  actions;

      EOSLIB_SERIALIZE_DERIVED( transaction, transaction_header, (context_free_actions)(actions) )
   };

   class deferred_transaction : public transaction {
      public:
         uint128_t     sender_id;
         account_name  sender;
         account_name  payer;
         eosio_time    execute_after;

         static deferred_transaction from_current_action() {
            return unpack_action_data<deferred_transaction>();
         }

         EOSLIB_SERIALIZE_DERIVED( deferred_transaction, transaction, (sender_id)(sender)(payer)(execute_after) )
   };

   /**
    * Retrieve the indicated action from the active transaction.
    * @param type - 0 for context free action, 1 for action
    * @param index - the index of the requested action
    * @return the indicated action
    */
   inline action get_action( uint32_t type, uint32_t index ) {
      auto size = ::get_action(type, index, nullptr, 0);
      eosio_assert( size > 0, "get_action size failed" );
      char buf[size];
      auto size2 = ::get_action(type, index, &buf[0], static_cast<size_t>(size) );
      eosio_assert( size == size2, "get_action failed" );
      return eosio::unpack<eosio::action>(&buf[0], static_cast<size_t>(size));
   }

   inline void check_auth(const bytes& trx_packed, const vector<permission_level>& permissions) {
      auto perm_packed = pack(permissions);
      ::check_auth( trx_packed.data(), trx_packed.size(), perm_packed.data(), perm_packed.size() );
   }

   inline void check_auth(const char *serialized_transaction, size_t size, const vector<permission_level>& permissions) {
      auto perm_packed = pack(permissions);
      ::check_auth( serialized_transaction, size, perm_packed.data(), perm_packed.size() );
   }

   inline void check_auth(const transaction& trx, const vector<permission_level>& permissions) {
      auto trx_packed = pack(trx);
      check_auth( trx_packed, permissions );
      //return res > 0;
   }

   ///@} transactioncpp api

} // namespace eos
