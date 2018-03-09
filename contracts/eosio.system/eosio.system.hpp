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
         //         using mseconds_per_day = typename common<SystemAccount>::mseconds_per_day;
         static const uint32_t max_inflation_rate = common<SystemAccount>::max_inflation_rate;
         static const uint32_t mseconds_per_day = common<SystemAccount>::mseconds_per_day;

         ACTION( SystemAccount, nonce ) {
            eosio::string                   value;

            EOSLIB_SERIALIZE( nonce, (value) )
         };

         static void on( const nonce& ) {
         }

         static bool update_cycle(time block_time) {
            auto parameters = global_state_singleton::get();
            if (parameters.first_block_time_in_cycle == 0) {
               voting<SystemAccount>::update_elected_producers(block_time);
               return true;
            }

            static const time slot = 500;
            static const uint32_t slots_per_cycle = common<SystemAccount>::blocks_per_cycle;
            const time delta = block_time - parameters.first_block_time_in_cycle;
            uint32_t time_slots = delta / slot;
            if (time_slots >= common<SystemAccount>::blocks_per_cycle) {
               time beginning_of_cycle = block_time - (time_slots % slots_per_cycle) * slot;
               voting<SystemAccount>::update_elected_producers(beginning_of_cycle);
               return true;
            }

            return false;
         }
         /*
         static system_token_type daily_payment(uint32_t percent_of_max_inflation_rate)
         {
            const system_token_type token_supply = currency::get_total_supply();
            const auto inflation_rate = max_inflation_rate * percent_of_max_inflation_rate;
            const auto& inflation_ratio = int_logarithm_one_plus(inflation_rate);
            return ((token_supply * inflation_ratio.first * 4) /
                    (inflation_ratio.second * common<SystemAccount>::days_per_4years));
         }

         static void fill_eos_bucket(time block_time) {
            auto parameters = global_state_singleton::get();
            const time delta = block_time - parameters.last_bucket_fill_time;
            if (delta >= mseconds_per_day) {
               parameters.last_bucket_fill_time = parameters.last_bucket_fill_time + mseconds_per_day;
               uint32_t percent = parameters.percent_of_max_inflation_rate - parameters.percent_of_max_inflation_rate / 2;
               parameters.eos_bucket += daily_payment(percent);
               global_state_singleton::set(parameters);
            }
         }
         */
         ACTION(SystemAccount, onblock) {
            block_header header;

            EOSLIB_SERIALIZE(onblock, (header))
         };

         static void on(const onblock& ob) {
            // update parameters if it's a new cycle
            update_cycle(ob.header.timestamp);
            producers_table producers_tbl(SystemAccount, SystemAccount);
            account_name producer = ob.header.producer;
            auto parameters = global_state_singleton::get_or_default();
            const system_token_type block_payment = parameters.payment_per_block;
            const auto* prod = producers_tbl.find(producer);
            // This check is needed when everything works
            // eosio_assert(prod != nullptr, "something wrong here");
            if (prod != nullptr) {
               producers_tbl.update(*prod, 0, [&](auto& p) {
                     p.per_block_payments += block_payment;
                     p.last_produced_block_time = ob.header.timestamp;
                  });
            }
            
            const uint32_t num_of_payments = (ob.header.timestamp - parameters.last_bucket_fill_time) / 500;
            const system_token_type to_eos_bucket = num_of_payments * parameters.payment_to_eos_bucket;
            parameters.last_bucket_fill_time = ob.header.timestamp;
            parameters.eos_bucket += to_eos_bucket;
            global_state_singleton::set(parameters);
         }

         ACTION(SystemAccount, claim_rewards) {
            account_name owner;
            time         claim_time;

            EOSLIB_SERIALIZE(claim_rewards, (owner)(claim_time))
         };

         static void on(const claim_rewards& cr) {
            producers_table producers_tbl(SystemAccount, SystemAccount);
            const auto* prod = producers_tbl.find(cr.owner);
            eosio_assert(prod != nullptr, "account name not proucer list");
            eosio_assert(prod->active(), "producer is not active");
            const time ctime = cr.claim_time;
            if (prod->last_rewards_claim > 0) {
               eosio_assert(ctime >= prod->last_rewards_claim + mseconds_per_day, "already claimed rewards today");
            }
            system_token_type rewards = prod->per_block_payments;
            auto idx = producers_tbl.template get_index<N(prototalvote)>();
            auto itr = --idx.end();

            bool is_among_payed_producers = false;
            uint64_t total_producer_votes = 0;
            for (uint32_t i = 0; i < 121; ++i) {
               if (!is_among_payed_producers) {
                  if (itr->owner == cr.owner)
                     is_among_payed_producers = true;
               }
               if (itr->active()) {
                  total_producer_votes += itr->total_votes;
               }
               if (itr == idx.begin()) {
                  break;
               }
               --itr;
            }

            if (is_among_payed_producers) {
               const system_token_type eos_bucket = global_state_singleton::get_or_default().eos_bucket;
               rewards += (uint64_t(prod->total_votes) * eos_bucket) / total_producer_votes;
            }
            producers_tbl.update(*prod, 0, [&](auto& p) {
                  p.last_rewards_claim = ctime;
                  p.per_block_payments = system_token_type();
               });
            currency::inline_transfer(cr.owner, SystemAccount, rewards, "producer claiming rewards");
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
                                 claim_rewards,
                                 nonce>( code, act) ) {
                  eosio::print("Unexpected action: ", eosio::name(act), "\n");
                  eosio_assert( false, "received unexpected action");
               }
            }

         } /// apply 
   };

} /// eosiosystem 

