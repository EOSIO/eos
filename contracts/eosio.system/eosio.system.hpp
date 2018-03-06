/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include "voting.hpp"
#include "delegate_bandwith.hpp"
#include <eosiolib/optional.hpp>

#include <eosiolib/generic_currency.hpp>

namespace eosiosystem {

   struct PACKED(producer_key) {
      account_name producer_name;
      public_key block_signing_key;

      EOSLIB_SERIALIZE(producer_key, (producer_name)(block_signing_key))
   };

   struct PACKED(producer_schedule) {
      uint32_t version;
      std::vector<producer_key> producers;

      EOSLIB_SERIALIZE(producer_schedule, (version)(producers))
   };

   struct PACKED(block_header) {
      checksum256                 previous;
      time                        timestamp;
      checksum256                 transaction_mroot;
      checksum256                 action_mroot;
      checksum256                 block_mroot;
      account_name                producer;
      eosio::optional<producer_schedule> new_producers;
      
      EOSLIB_SERIALIZE(block_header, (previous)(timestamp)(transaction_mroot)(action_mroot)(block_mroot)(producer)(new_producers))
   };

   template<account_name SystemAccount>
   class contract : public voting<SystemAccount>, public delegate_bandwith<SystemAccount> {
      public:
         using voting<SystemAccount>::on;
         using delegate_bandwith<SystemAccount>::on;
         using pe = voting<SystemAccount>;
         using db = delegate_bandwith<SystemAccount>;
         using currency = typename common<SystemAccount>::currency;

         ACTION( SystemAccount, nonce ) {
            eosio::string                   value;

            EOSLIB_SERIALIZE( nonce, (value) )
         };

         static void on( const nonce& ) {
         }

         ACTION(SystemAccount, onblock) {
            block_header header;

            EOSLIB_SERIALIZE(onblock, (header))
         };

         static void on(const onblock& ob) {
         }

         static void apply( account_name code, action_name act ) {
            if ( !eosio::dispatch<currency, typename currency::transfer, typename currency::issue>( code, act ) ) {
               if( !eosio::dispatch<contract, typename delegate_bandwith<SystemAccount>::delegatebw,
                                 typename delegate_bandwith<SystemAccount>::undelegatebw,
                                 typename voting<SystemAccount>::register_proxy,
                                 typename voting<SystemAccount>::unregister_proxy,
                                 typename voting<SystemAccount>::regproducer,
                                 typename voting<SystemAccount>::vote_producer,
                                 typename voting<SystemAccount>::stakevote,
                                 typename voting<SystemAccount>::unstakevote,
                                 typename voting<SystemAccount>::unstake_vote_deferred,
                                 typename voting<SystemAccount>::onblock,
                                 nonce>( code, act) ) {
                  eosio::print("Unexpected action: ", eosio::name(act), "\n");
                  eosio_assert( false, "received unexpected action");
               }
            }

         } /// apply 
   };

} /// eosiosystem 

