#include "eosio.system.hpp"

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   const int64_t  min_daily_tokens = 100;
   const double   continuous_rate  = std::log1p(0.05); // 5% annual rate
   const double   per_block_rate   = 0.0025;           // 0.25%
   const double   standby_rate     = 0.0075;           // 0.75%
   const uint32_t blocks_per_year  = 52*7*24*2*3600;   // half seconds per year
   const uint32_t blocks_per_day   = 2 * 24 * 3600;
   const uint32_t blocks_per_hour  = 2 * 3600;

   eosio::asset system_contract::payment_per_block( double rate, const eosio::asset& token_supply ) {
      const int64_t payment = static_cast<int64_t>( (rate * double(token_supply.amount)) / double(blocks_per_year) );
      return eosio::asset( payment, token_supply.symbol );
   }
   
   void system_contract::onblock( block_timestamp timestamp, account_name producer ) {
      
      using namespace eosio;
      
      const asset token_supply     = token( N(eosio.token)).get_supply(symbol_type(system_token_symbol).name() );
      const asset issued           = payment_per_block( continuous_rate, token_supply );
      const asset producer_payment = payment_per_block( per_block_rate, token_supply );
      const asset to_eos_bucket    = payment_per_block( standby_rate, token_supply );
      const asset to_savings       = issued - (producer_payment + to_eos_bucket);
      
      INLINE_ACTION_SENDER(eosio::token, issue)( N(eosio.token), {{N(eosio),N(active)}},
                                                 {N(eosio), issued, std::string("issue tokens per block")} );
   
      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {{N(eosio),N(active)}},
                                                    {N(eosio), N(eosio), to_savings, std::string("transfer to savings per block")} );
      // update producer info and balance
      auto prod = _producers.find(producer);
      if ( prod != _producers.end() ) {
         _producers.modify( prod, 0, [&](auto& p) {
               p.per_block_payments      += producer_payment;
               p.last_produced_block_time = timestamp;
            });
      }

      auto parameters = _global.exists() ? _global.get() : get_default_parameters();
      parameters.eos_bucket += to_eos_bucket;
      parameters.savings    += to_savings;
      _global.set ( parameters, _self );
      
      const auto& producer_schedule_update = parameters.last_producer_schedule_update;
      if ( producer_schedule_update == 0 || producer_schedule_update < timestamp + blocks_per_hour ) {
         update_elected_producers( producer_schedule_update );
      }
      
   }
   
   eosio::asset system_contract::payment_per_vote( const account_name& owner, double owners_votes, const eosio::asset& eos_bucket ) {
      eosio::asset payment(0, S(4,EOS));
      if ( eos_bucket.amount < min_daily_tokens ) {
         return payment;
      }
      
      auto idx = _producers.template get_index<N(prototalvote)>();
      
      double total_producer_votes   = 0;   
      double running_payment_amount = 0;
      bool   to_be_payed            = false;
      for ( auto itr = idx.cbegin(); itr != idx.cend(); ++itr ) {
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
      
      auto prod = _producers.find( owner );
      eosio_assert( prod != _producers.end(), "account name is not in producer list" );
      if( prod->last_rewards_claim > 0 ) {
         eosio_assert(now() >= prod->last_rewards_claim + seconds_per_day, "already claimed rewards within a day");
      }
      
      eosio::asset rewards = prod->per_block_payments;
      
      if ( _global.exists() ) {
         auto parameters = _global.get();
         if ( parameters.eos_bucket.amount > 0 && prod->total_votes > 0 ) {
            eosio::asset standby_payment = payment_per_vote( owner, prod->total_votes, parameters.eos_bucket );
            if ( standby_payment.amount > 0 ) {
               rewards               += standby_payment;
               parameters.eos_bucket -= standby_payment;
               _global.set( parameters, _self );
            }
         }
      }
      
      eosio_assert( rewards > asset(0, S(4,EOS)), "no rewards available to claim" );
      
      _producers.modify( prod, 0, [&](auto& p) {
            p.last_rewards_claim = now();
            p.per_block_payments.amount = 0;
         });
      
      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                    { N(eosio), owner, rewards, std::string("producer claiming rewards") } );
      
   }

} //namespace eosiosystem
