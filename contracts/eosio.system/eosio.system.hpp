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
   /*
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
   */
   struct PACKED(block_header) {
      checksum256                 previous;
      time                        timestamp;
      checksum256                 transaction_mroot;
      checksum256                 action_mroot;
      checksum256                 block_mroot;
      account_name                producer;
      eosio::optional<eosio::producer_schedule> new_producers;
      
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
         using system_token_type = typename common<SystemAccount>::system_token_type;
         using producers_table = typename pe::producers_table;
         using global_state_singleton = typename voting<SystemAccount>::global_state_singleton;

         ACTION( SystemAccount, nonce ) {
            eosio::string                   value;

            EOSLIB_SERIALIZE( nonce, (value) )
         };

         static void on( const nonce& ) {
         }

         static bool is_new_cycle(time block_time) {
            auto parameters = global_state_singleton::get();
            if (parameters.first_block_time_in_cycle == 0) {
               parameters.first_block_time_in_cycle = block_time;
               global_state_singleton::set(parameters);
               return true;
            }

            static const time slot = 500;
            static const uint32_t slots_per_cycle = common<SystemAccount>::blocks_per_cycle;
            const time delta = block_time - parameters.first_block_time_in_cycle;
            uint32_t time_slots = delta / slot;
            if (time_slots >= common<SystemAccount>::blocks_per_cycle) {
               time beginning_of_cycle = block_time - (time_slots % slots_per_cycle) * slot;
               parameters.first_block_time_in_cycle = beginning_of_cycle;
               global_state_singleton::set(parameters);
               return true;
            }

            return false;
         }

         ACTION(SystemAccount, onblock) {
            block_header header;

            EOSLIB_SERIALIZE(onblock, (header))
         };

         static void on(const onblock& ob) {
            if (is_new_cycle(ob.header.timestamp)) {
               voting<SystemAccount>::update_elected_producers();
            }
            producers_table producers_tbl(SystemAccount, SystemAccount);
            account_name producer = ob.header.producer;
            const system_token_type payment = global_state_singleton::get_or_default().payment_per_block;
            const auto* prod = producers_tbl.find(producer);
            // This check is needed in a real production system
            //            eosio_assert(prod != nullptr, "something wrong here");
            if (prod != nullptr) {
               producers_tbl.update(*prod, 0, [&](auto& p) {
                     p.per_block_payments += payment;
                     p.last_produced_block_time = ob.header.timestamp;
                  });
            }

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
                                 onblock,
                                 nonce>( code, act) ) {
                  eosio::print("Unexpected action: ", eosio::name(act), "\n");
                  eosio_assert( false, "received unexpected action");
               }
            }

         } /// apply 
   };

} /// eosiosystem 

