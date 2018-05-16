#include "eosio.system.hpp"

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   const int64_t  min_daily_tokens = 100;

   const double   continuous_rate  = 0.04879;          // 5% annual rate
   const double   perblock_rate    = 0.0025;           // 0.25%
   const double   standby_rate     = 0.0075;           // 0.75%
   const uint32_t blocks_per_year  = 52*7*24*2*3600;   // half seconds per year
   const uint32_t seconds_per_year = 52*7*24*3600;
   const uint32_t blocks_per_day   = 2 * 24 * 3600;
   const uint32_t blocks_per_hour  = 2 * 3600;
   const uint64_t useconds_per_day = 24 * 3600 * uint64_t(1000000);
   
   eosio::asset system_contract::payment_per_block( double rate, const eosio::asset& token_supply,  uint32_t num_blocks ) {
      const int64_t payment = static_cast<int64_t>( (rate * double(token_supply.amount) * double(num_blocks)) / double(blocks_per_year) );
      return eosio::asset( payment, token_supply.symbol );
   }

   eosio::asset system_contract::supply_growth( double rate, const eosio::asset& token_supply, time seconds ) {
      const int64_t payment = static_cast<int64_t>( (rate * double(token_supply.amount) * double(seconds)) / double(seconds_per_year) );
      return eosio::asset( payment, token_supply.symbol );
   }

   void system_contract::onblock( block_timestamp timestamp, account_name producer ) {
      using namespace eosio;

      require_auth(N(eosio));

      /** until activated stake crosses this threshold no new rewards are paid */
      if( _gstate.total_activated_stake < 150'000'000'0000 )
         return;

      if( _gstate.last_pervote_bucket_fill == 0 )  /// start the presses
         _gstate.last_pervote_bucket_fill = current_time();

      auto prod = _producers.find(producer);
      if ( prod != _producers.end() ) {
         _producers.modify( prod, 0, [&](auto& p ) {
               p.produced_blocks++;
               p.last_produced_block_time = timestamp;
         });
      }
      
      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp - _gstate.last_producer_schedule_update > 120 ) {
         update_elected_producers( timestamp );
      }

   }
   
   eosio::asset system_contract::payment_per_vote( const account_name& owner, double owners_votes, const eosio::asset& pervote_bucket ) {
      eosio::asset payment(0, S(4,EOS));
      const int64_t min_daily_amount = 100 * 10000;
      if ( pervote_bucket.amount < min_daily_amount ) {
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
         if ( !itr->active() ) {
            continue;
         }
         
         if ( itr->owner == owner ) {
            to_be_payed = true;
         }
         
         total_producer_votes += itr->total_votes;
         running_payment_amount = (itr->total_votes) * double(pervote_bucket.amount) / total_producer_votes;
         if ( running_payment_amount < min_daily_amount ) {
            if ( itr->owner == owner ) {
               to_be_payed = false;
            }
            total_producer_votes -= itr->total_votes;
            break;
         }
      }
      
      if ( to_be_payed ) {
         payment.amount = static_cast<int64_t>( (double(pervote_bucket.amount) * owners_votes) / total_producer_votes );
      }
      
      return payment;
   }

   void system_contract::claimrewards( const account_name& owner ) {
      using namespace eosio;

      require_auth(owner);

      auto prod = _producers.find( owner );
      eosio_assert( prod != _producers.end(), "account name is not in producer list" );
      eosio_assert( prod->active(), "producer does not have an active key" );
      if( prod->last_claim_time > 0 ) {
         eosio_assert(current_time() >= prod->last_claim_time + useconds_per_day, "already claimed rewards within a day");
      }

      const asset token_supply = token( N(eosio.token)).get_supply(symbol_type(system_token_symbol).name() );
      const uint32_t secs_since_last_fill = static_cast<uint32_t>( (current_time() - _gstate.last_pervote_bucket_fill) / 1000000 );

      const asset to_pervote_bucket = supply_growth( standby_rate, token_supply, secs_since_last_fill );
      const asset to_savings    = supply_growth( continuous_rate - (perblock_rate + standby_rate), token_supply, secs_since_last_fill );
      const asset perblock_pay  = payment_per_block( perblock_rate, token_supply, prod->produced_blocks );
      const asset issue_amount  = to_pervote_bucket + to_savings + perblock_pay;
      
      const asset pervote_pay = payment_per_vote( owner, prod->total_votes, to_pervote_bucket + _gstate.pervote_bucket );

      if ( perblock_pay.amount + pervote_pay.amount == 0 ) {
         _producers.modify( prod, 0, [&](auto& p) {
               p.last_claim_time = current_time();
            });
         return;
      }
      
      INLINE_ACTION_SENDER(eosio::token, issue)( N(eosio.token), {{N(eosio),N(active)}},
                                                 {N(eosio), issue_amount, std::string("issue tokens for producer pay and savings")} );

      _gstate.pervote_bucket          += ( to_pervote_bucket - pervote_pay );
      _gstate.last_pervote_bucket_fill = current_time();
      _gstate.savings                 += to_savings;
      
      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                    { N(eosio), owner, perblock_pay + pervote_pay, std::string("producer claiming rewards") } );

      _producers.modify( prod, 0, [&](auto& p) {
            p.last_claim_time = current_time();
            p.produced_blocks = 0;
         });

   }

} //namespace eosiosystem
