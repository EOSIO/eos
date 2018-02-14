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
      uint32_t        ref_block_prefix;
      uint16_t        packed_bandwidth_words = 0; /// number of 8 byte words this transaction can compress into
      uint16_t        context_free_cpu_bandwidth = 0; /// number of CPU usage units to bill transaction for

      vector<action>  context_free_actions;
      vector<action>  actions;

      EOSLIB_SERIALIZE( transaction, (expiration)(region)(ref_block_num)(ref_block_prefix)(packed_bandwidth_words)(context_free_cpu_bandwidth)(context_free_actions)(actions) );
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

