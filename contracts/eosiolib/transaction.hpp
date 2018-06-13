/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/transaction.h>
#include <eosiolib/action.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/time.hpp>
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
      transaction_header( time_point_sec exp = time_point_sec(now() + 60) )
         :expiration(exp)
      {}

      time_point_sec  expiration;
      uint16_t        ref_block_num;
      uint32_t        ref_block_prefix;
      unsigned_int    net_usage_words = 0UL; /// number of 8 byte words this transaction can serialize into after compressions
      uint8_t         max_cpu_usage_ms = 0UL; /// number of CPU usage units to bill transaction for
      unsigned_int    delay_sec = 0UL; /// number of CPU usage units to bill transaction for

      EOSLIB_SERIALIZE( transaction_header, (expiration)(ref_block_num)(ref_block_prefix)(net_usage_words)(max_cpu_usage_ms)(delay_sec) )
   };

   class transaction : public transaction_header {
   public:
      transaction(time_point_sec exp = time_point_sec(now() + 60)) : transaction_header( exp ) {}

      void send(const uint128_t& sender_id, account_name payer, bool replace_existing = false) const {
         auto serialize = pack(*this);
         send_deferred(sender_id, payer, serialize.data(), serialize.size(), replace_existing);
      }

      vector<action>  context_free_actions;
      vector<action>  actions;
      extensions_type transaction_extensions;

      EOSLIB_SERIALIZE_DERIVED( transaction, transaction_header, (context_free_actions)(actions)(transaction_extensions) )
   };

   struct onerror {
      uint128_t sender_id;
      bytes     sent_trx;

      static onerror from_current_action() {
         return unpack_action_data<onerror>();
      }

      transaction unpack_sent_trx() const {
         return unpack<transaction>(sent_trx);
      }

      EOSLIB_SERIALIZE( onerror, (sender_id)(sent_trx) )
   };

   /**
    * Retrieve the indicated action from the active transaction.
    * @param type - 0 for context free action, 1 for action
    * @param index - the index of the requested action
    * @return the indicated action
    */
   inline action get_action( uint32_t type, uint32_t index ) {
      constexpr size_t max_stack_buffer_size = 512;
      int s = ::get_action( type, index, nullptr, 0 );
      eosio_assert( s > 0, "get_action size failed" );
      size_t size = static_cast<size_t>(s);
      char* buffer = (char*)( max_stack_buffer_size < size ? malloc(size) : alloca(size) );
      auto size2 = ::get_action( type, index, buffer, size );
      eosio_assert( size == static_cast<size_t>(size2), "get_action failed" );
      return eosio::unpack<eosio::action>( buffer, size );
   }

   ///@} transactioncpp api

} // namespace eos
