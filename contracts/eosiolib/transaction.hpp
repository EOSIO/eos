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


   class transaction {
   public:
      transaction(time expiration = now() + 60, region_id region = 0)
      :expiration(expiration),region(region)
      {}

      void send(uint32_t sender_id, time delay_until = 0) const {
         auto serialize = pack(*this);
         send_deferred(sender_id, delay_until, serialize.data(), serialize.size());
      }

      time            expiration;
      region_id       region;
      uint16_t        ref_block_num;
      uint32_t        ref_block_id;

      vector<action>  actions;

      EOSLIB_SERIALIZE( transaction, (expiration)(region)(ref_block_num)(ref_block_id)(actions) );
   };

   class deferred_transaction : public transaction {
      public:
         uint32_t     sender_id;
         account_name sender;
         time         delay_until;

         static deferred_transaction from_current_action() {
            return unpack_action<deferred_transaction>();
         }

         EOSLIB_SERIALIZE_DERIVED( deferred_transaction, transaction, (sender_id)(sender)(delay_until) )
   };

 ///@} transactioncpp api

} // namespace eos

