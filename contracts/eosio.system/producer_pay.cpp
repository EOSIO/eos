#include "eosio.system.hpp"

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

const int64_t  min_daily_tokens       = 100;
const double   continuous_rate        = std::log1p(5);
const uint32_t blocks_per_year        = 52*7*24*2*3600; // half seconds per year
const uint32_t blocks_per_hour        = 2 * 3600; 

eosio::asset system_contract::payment_per_block( double rate ) {
   const eosio::asset token_supply = eosio::token( N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name() );
   const int64_t payment = static_cast<int64_t>( (rate * double(token_supply.amount)) / double(blocks_per_year) );
   return eosio::asset(payment, system_token_symbol);
}

void system_contract::onblock( block_timestamp timestamp, account_name producer ) {

   global_state_singleton gs( _self, _self );
   auto parameters = gs.exists() ? gs.get() : get_default_parameters();
   if( parameters.first_block_time_in_cycle == 0 ) {
      // This is the first time onblock is called in the blockchain.
      parameters.last_bucket_fill_time = timestamp;
      gs.set( parameters, _self );
      update_elected_producers( timestamp );
   }

   static const uint32_t slots_per_cycle = parameters.blocks_per_cycle;
   const uint32_t time_slots = timestamp - parameters.first_block_time_in_cycle;
   if (time_slots >= slots_per_cycle) {
      auto beginning_of_cycle = timestamp - (time_slots % slots_per_cycle);
      update_elected_producers(beginning_of_cycle);
   }

   producers_table producers_tbl( _self, _self );

   const asset block_payment = parameters.payment_per_block;
   auto prod = producers_tbl.find(producer);
   if ( prod != producers_tbl.end() ) {
      producers_tbl.modify( prod, 0, [&](auto& p) {
            p.per_block_payments += block_payment;
            p.last_produced_block_time = timestamp;
         });
   }

   const uint32_t num_of_payments = timestamp - parameters.last_bucket_fill_time;
   const asset to_eos_bucket = num_of_payments * parameters.payment_to_eos_bucket;
   parameters.last_bucket_fill_time = timestamp;
   parameters.eos_bucket += to_eos_bucket;
   gs.set( parameters, _self );
}

eosio::asset system_contract::payment_per_vote( const account_name& owner, double owners_votes, const eosio::asset& eos_bucket ) {
   eosio::asset payment(0, S(4,EOS));
   if (eos_bucket < threshold) {
      return payment;
   }
   
   auto idx = _producers.template get_index<N(prototalvote)>();

   double total_producer_votes   = 0;   
   double running_payment_amount = 0;
   bool   to_be_payed            = false;
   for ( auto itr = idx.begin(); itr != idx.end(); ++itr ) {
      if ( !(itr->total_votes > 0) ) {
         break;
      }
      if ( !(itr->active()) && !(itr->owner != owner) ) {
         continue;
      }

      if ( itr->owner == owner ) {
         to_be_payed = true;
      }
      
      total_producer_votes += itr->total_votes;
      running_payment_amount = (itr->total_votes) * double(eos_bucket.amount) / total_producer_votes;
      if ( running_payment_amount < min_daily_tokens ) {
         if ( itr->owner == owner ) {
            to_be_payed = false;
         }
         total_producer_votes -= itr->total_votes;
         break;
      }
   }

   if ( to_be_payed ) {
      payment.amount = static_cast<int64_t>( (double(eos_bucket.amount) * owners_votes) / total_producer_votes );
   }
   
   return payment;
}

void system_contract::claimrewards( const account_name& owner ) {
   require_auth(owner);

   producers_table producers_tbl( _self, _self );
   auto prod = producers_tbl.find( owner );
   eosio_assert(prod != producers_tbl.end(), "account name is not in producer list");
   if( prod->last_rewards_claim > 0 ) {
      eosio_assert(now() >= prod->last_rewards_claim + seconds_per_day, "already claimed rewards within a day");
   }

   eosio::asset rewards = prod->per_block_payments;

   global_state_singleton gs( _self, _self );
   if ( gs.exists() ) {
      auto parameters = gs.get();
      if ( parameters.eos_bucket.amount > 0 && prod->total_votes > 0 ) {
         eosio::asset standby_payment = payment_per_vote( owner, prod->total_votes, parameters.eos_bucket );
         if ( standby_payment.amount > 0 ) {
            rewards               += standby_payment;
            parameters.eos_bucket -= standby_payment;
            gs.set( parameters, _self );
         }
      }
   }

   eosio_assert( rewards > asset(0, S(4,EOS)), "no rewards available to claim" );
   
   producers_tbl.modify( prod, 0, [&](auto& p) {
         p.last_rewards_claim = now();
         p.per_block_payments.amount = 0;
      });
   
   INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                 { N(eosio), owner, rewards, std::string("producer claiming rewards") } );

}

} //namespace eosiosystem
