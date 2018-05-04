#include "eosio.system.hpp"

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

static const uint32_t num_of_payed_producers = 121;


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

   // const system_token_type block_payment = parameters.payment_per_block;
   const asset block_payment = parameters.payment_per_block;
   auto prod = producers_tbl.find(producer);
   if ( prod != producers_tbl.end() ) {
      producers_tbl.modify( prod, 0, [&](auto& p) {
            p.per_block_payments += block_payment;
            p.last_produced_block_time = timestamp;
         });
   }

   const uint32_t num_of_payments = timestamp - parameters.last_bucket_fill_time;
   //            const system_token_type to_eos_bucket = num_of_payments * parameters.payment_to_eos_bucket;
   const asset to_eos_bucket = num_of_payments * parameters.payment_to_eos_bucket;
   parameters.last_bucket_fill_time = timestamp;
   parameters.eos_bucket += to_eos_bucket;
   gs.set( parameters, _self );
}

void system_contract::claimrewards(const account_name& owner) {
   require_auth(owner);

   producers_table producers_tbl( _self, _self );
   auto prod = producers_tbl.find(owner);
   eosio_assert(prod != producers_tbl.end(), "account name is not in producer list");
   if( prod->last_rewards_claim > 0 ) {
      eosio_assert(now() >= prod->last_rewards_claim + seconds_per_day, "already claimed rewards within a day");
   }

   //            system_token_type rewards = prod->per_block_payments;
   eosio::asset rewards = prod->per_block_payments;
   auto idx = producers_tbl.template get_index<N(prototalvote)>();
   auto itr = --idx.end();

   bool is_among_payed_producers = false;
   uint128_t total_producer_votes = 0;
   uint32_t n = 0;
   while( n < num_of_payed_producers ) {
      if( !is_among_payed_producers ) {
         if( itr->owner == owner )
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
      global_state_singleton gs( _self, _self );
      if( gs.exists() ) {
         auto parameters = gs.get();
         //                  auto share_of_eos_bucket = system_token_type( static_cast<uint64_t>( (prod->total_votes * parameters.eos_bucket.quantity) / total_producer_votes ) ); // This will be improved in the future when total_votes becomes a double type.
         auto share_of_eos_bucket = eosio::asset( static_cast<int64_t>( (prod->total_votes * parameters.eos_bucket.amount) / total_producer_votes ) );
         rewards += share_of_eos_bucket;
         parameters.eos_bucket -= share_of_eos_bucket;
         gs.set( parameters, _self );
      }
   }

   //            eosio_assert( rewards > system_token_type(), "no rewards available to claim" );
   eosio_assert( rewards > asset(0, S(4,EOS)), "no rewards available to claim" );

   producers_tbl.modify( prod, 0, [&](auto& p) {
         p.last_rewards_claim = now();
         p.per_block_payments.amount = 0;
      });

   INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                 { N(eosio), owner, rewards, std::string("producer claiming rewards") } );
}

} //namespace eosiosystem
