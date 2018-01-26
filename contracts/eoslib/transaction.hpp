/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/transaction.h>
#include <eoslib/action.hpp>
#include <eoslib/print.hpp>
#include <eoslib/string.hpp>
#include <eoslib/types.hpp>
#include <eoslib/raw.hpp>

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
         auto serialize = raw::pack(*this);
         send_deferred(sender_id, delay_until, serialize.data(), serialize.size());
      }

      time            expiration;
      region_id       region;
      uint16_t        ref_block_num;
      uint32_t        ref_block_id;

      vector<action>  actions;

      template<typename DataStream>
      friend DataStream& operator << ( DataStream& ds, const transaction& t ){
         ds << t.expiration << t.region << t.ref_block_num << t.ref_block_id;
         raw::pack(ds, t.actions);
         return ds;
      }

      template<typename DataStream>
      friend DataStream& operator >> ( DataStream& ds,  transaction& t ){
         ds >> t.expiration >> t.region >> t.ref_block_num >> t.ref_block_id;
         raw::unpack(ds, t.actions);
         return ds;
      }

   };

   class deferred_transaction : public transaction {
      public:
         uint32_t     sender_id;
         account_name sender;
         time         delay_until;

         static deferred_transaction from_current_action() {
            return unpack_action<deferred_transaction>();
         }

         template<typename DataStream>
         friend DataStream& operator >> ( DataStream& ds,  deferred_transaction& t ){
            return ds >> *static_cast<transaction *>(&t) >> t.sender_id >> t.sender >> t.delay_until;
         }

      private:
         deferred_transaction()
         {}

         friend deferred_transaction eosio::raw::unpack<deferred_transaction>( const char*, uint32_t );

   };

 ///@} transactioncpp api

} // namespace eos

EOSLIB_REFLECT( eosio::transaction, (expiration)(region)(ref_block_num)(ref_block_id) )
EOSLIB_REFLECT_DERIVED( eosio::deferred_transaction, (eosio::transaction), (sender_id)(sender)(delay_until))