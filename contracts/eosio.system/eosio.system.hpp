/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include "delegate_bandwidth.hpp"
#include "native.hpp"
#include <eosiolib/optional.hpp>

#include <eosiolib/generic_currency.hpp>

namespace eosiosystem {

   struct block_header {
      checksum256                               previous;
      time                                      timestamp;
      checksum256                               transaction_mroot;
      checksum256                               action_mroot;
      checksum256                               block_mroot;
      account_name                              producer;
      uint32_t                                  schedule_version;
      eosio::optional<eosio::producer_schedule> new_producers;

      EOSLIB_SERIALIZE(block_header, (previous)(timestamp)(transaction_mroot)(action_mroot)(block_mroot)
                                     (producer)(schedule_version)(new_producers))
   };

   template<account_name SystemAccount>
   class contract : public delegate_bandwidth<SystemAccount>, public native<SystemAccount> {
      public:
         using voting<SystemAccount>::on;
         using delegate_bandwidth<SystemAccount>::on;
         using native<SystemAccount>::on;
         using pe = voting<SystemAccount>;
         using db = delegate_bandwidth<SystemAccount>;
         using currency = typename common<SystemAccount>::currency;
         using system_token_type = typename common<SystemAccount>::system_token_type;
         using producers_table = typename pe::producers_table;
         using global_state_singleton = typename voting<SystemAccount>::global_state_singleton;
         static const uint32_t max_inflation_rate = common<SystemAccount>::max_inflation_rate;
         static const uint32_t seconds_per_day = common<SystemAccount>::seconds_per_day;
         static const uint32_t num_of_payed_producers = 121;

         ACTION( SystemAccount, nonce ) {
            eosio::string                   value;

            EOSLIB_SERIALIZE( nonce, (value) )
         };

         static void on( const nonce& ) {
         }

         static bool update_cycle(time block_time) {
            auto parameters = global_state_singleton::exists() ? global_state_singleton::get()
               : common<SystemAccount>::get_default_parameters();
            if (parameters.first_block_time_in_cycle == 0) {
               // This is the first time onblock is called in the blockchain.
               parameters.last_bucket_fill_time = block_time;
               global_state_singleton::set(parameters);
               voting<SystemAccount>::update_elected_producers(block_time);
               return true;
            }

            static const uint32_t slots_per_cycle = parameters.blocks_per_cycle;
            const uint32_t time_slots = block_time - parameters.first_block_time_in_cycle;
            if (time_slots >= slots_per_cycle) {
               time beginning_of_cycle = block_time - (time_slots % slots_per_cycle);
               voting<SystemAccount>::update_elected_producers(beginning_of_cycle);
               return true;
            }

            return false;
         }

         ACTION(SystemAccount, onblock) {
            block_header header;

            EOSLIB_SERIALIZE(onblock, (header))
         };

         static void on(const onblock& ob) {
            // update parameters if it's a new cycle
            update_cycle(ob.header.timestamp);

            producers_table producers_tbl(SystemAccount, SystemAccount);
            account_name producer = ob.header.producer;
            auto parameters = global_state_singleton::exists() ? global_state_singleton::get()
                  : common<SystemAccount>::get_default_parameters();
            const system_token_type block_payment = parameters.payment_per_block;
            auto prod = producers_tbl.find(producer);
            if ( prod != producers_tbl.end() ) {
               producers_tbl.modify( prod, 0, [&](auto& p) {
                     p.per_block_payments += block_payment;
                     p.last_produced_block_time = ob.header.timestamp;
                  });
            }

            const uint32_t num_of_payments = ob.header.timestamp - parameters.last_bucket_fill_time;
            const system_token_type to_eos_bucket = num_of_payments * parameters.payment_to_eos_bucket;
            parameters.last_bucket_fill_time = ob.header.timestamp;
            parameters.eos_bucket += to_eos_bucket;
            global_state_singleton::set(parameters);
         }

         ACTION(SystemAccount, claimrewards) {
            account_name owner;

            EOSLIB_SERIALIZE(claimrewards, (owner))
         };

         static void on(const claimrewards& cr) {
            require_auth(cr.owner);
            eosio_assert(current_sender() == account_name(), "claimrewards can not be part of a deferred transaction");
            producers_table producers_tbl(SystemAccount, SystemAccount);
            auto prod = producers_tbl.find(cr.owner);
            eosio_assert(prod != producers_tbl.end(), "account name is not in producer list");
            eosio_assert(prod->active(), "producer is not active"); // QUESTION: Why do we want to prevent inactive producers from claiming their earned rewards?
            if( prod->last_rewards_claim > 0 ) {
               eosio_assert(now() >= prod->last_rewards_claim + seconds_per_day, "already claimed rewards within a day");
            }
            system_token_type rewards = prod->per_block_payments;
            auto idx = producers_tbl.template get_index<N(prototalvote)>();
            auto itr = --idx.end();

            bool is_among_payed_producers = false;
            uint128_t total_producer_votes = 0;
            uint32_t n = 0;
            while( n < num_of_payed_producers ) {
               if( !is_among_payed_producers ) {
                  if( itr->owner == cr.owner )
                     is_among_payed_producers = true;
               }
               if( itr->active() ) {
                  total_producer_votes += itr->total_votes;
                  ++n;
               }
               if( itr == idx.begin() ) {
                  break;
               }
               --itr;
            }

            if (is_among_payed_producers && total_producer_votes > 0) {
               if( global_state_singleton::exists() ) {
                  auto parameters = global_state_singleton::get();
                  auto share_of_eos_bucket = system_token_type( static_cast<uint64_t>( (prod->total_votes * parameters.eos_bucket.quantity) / total_producer_votes ) ); // This will be improved in the future when total_votes becomes a double type.
                  rewards += share_of_eos_bucket;
                  parameters.eos_bucket -= share_of_eos_bucket;
                  global_state_singleton::set(parameters);
               }
            }

            eosio_assert( rewards > system_token_type(), "no rewards available to claim" );

            producers_tbl.modify( prod, 0, [&](auto& p) {
                  p.last_rewards_claim = now();
                  p.per_block_payments.quantity = 0;
               });

            currency::inline_transfer(cr.owner, SystemAccount, rewards, "producer claiming rewards");
         }

         static void apply( account_name receiver, account_name code, action_name act ) {
            if ( !eosio::dispatch<currency, typename currency::transfer, typename currency::issue>( code, act ) ) {
               if( !eosio::dispatch<contract, typename delegate_bandwidth<SystemAccount>::delegatebw,
                                 typename delegate_bandwidth<SystemAccount>::refund,
                                 typename voting<SystemAccount>::regproxy,
                                 typename voting<SystemAccount>::unregproxy,
                                 typename voting<SystemAccount>::regproducer,
                                 typename voting<SystemAccount>::unregprod,
                                 typename voting<SystemAccount>::voteproducer,
                                 onblock,
                                 claimrewards,
                                 typename native<SystemAccount>::newaccount,
                                 typename native<SystemAccount>::updateauth,
                                 typename native<SystemAccount>::deleteauth,
                                 typename native<SystemAccount>::linkauth,
                                 typename native<SystemAccount>::unlinkauth,
                                 typename native<SystemAccount>::postrecovery,
                                 typename native<SystemAccount>::passrecovery,
                                 typename native<SystemAccount>::vetorecovery,
                                 typename native<SystemAccount>::onerror,
                                 typename native<SystemAccount>::canceldelay,
                                 nonce>( code, act) ) {
                  //TODO: Small hack until we refactor eosio.system like eosio.token
                  using undelegatebw = typename delegate_bandwidth<SystemAccount>::undelegatebw;
                  if(code == undelegatebw::get_account() && act == undelegatebw::get_name() ){
                     contract().on( receiver, eosio::unpack_action_data<undelegatebw>() );
                  }
               }
            }

         } /// apply
   };

} /// eosiosystem
