/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "eosio.system.hpp"

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.token/eosio.token.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::bytes;
   using eosio::print;
   using eosio::singleton;
   using eosio::transaction;


   static constexpr uint32_t blocks_per_year = 52*7*24*2*3600; // half seconds per year
   static constexpr uint32_t blocks_per_producer = 12;


   /**
    *  This method will create a producer_config and producer_info object for 'producer'
    *
    *  @pre producer is not already registered
    *  @pre producer to register is an account
    *  @pre authority of producer to register
    *
    */
   void system_contract::regproducer( const account_name producer, const eosio::public_key& producer_key, const std::string& url ) { //, const eosio_parameters& prefs ) {
      eosio_assert( url.size() < 512, "url too long" );
      //eosio::print("produce_key: ", producer_key.size(), ", sizeof(public_key): ", sizeof(public_key), "\n");
      require_auth( producer );

      auto prod = _producers.find( producer );

      if ( prod != _producers.end() ) {
         if( producer_key != prod->producer_key ) {
             _producers.modify( prod, producer, [&]( producer_info& info ){
                  info.producer_key = producer_key;
             });
         }
      } else {
         _producers.emplace( producer, [&]( producer_info& info ){
               info.owner       = producer;
               info.total_votes = 0;
               info.producer_key =  producer_key;
         });
      }
   }

   void system_contract::unregprod( const account_name producer ) {
      require_auth( producer );

      const auto& prod = _producers.get( producer );

      _producers.modify( prod, 0, [&]( producer_info& info ){
         info.producer_key = eosio::public_key();
      });
   }


   eosio::asset system_contract::payment_per_block(uint32_t percent_of_max_inflation_rate) {
      const eosio::asset token_supply = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name());
      const double annual_rate = double(max_inflation_rate * percent_of_max_inflation_rate) / double(10000);
      const double continuous_rate = std::log1p(annual_rate);
      int64_t payment = static_cast<int64_t>((continuous_rate * double(token_supply.amount)) / double(blocks_per_year));
      return eosio::asset(payment, system_token_symbol);
   }

   void system_contract::update_elected_producers(time cycle_time) {
      auto idx = _producers.get_index<N(prototalvote)>();

      eosio::producer_schedule schedule;
      schedule.producers.reserve(21);
      size_t n = 0;
      for ( auto it = idx.crbegin(); it != idx.crend() && n < 21 && 0 < it->total_votes; ++it ) {
         if ( it->active() ) {
            schedule.producers.emplace_back();
            schedule.producers.back().producer_name = it->owner;
            schedule.producers.back().block_signing_key = it->producer_key; 
            ++n;
         }
      }
      if ( n == 0 ) { //no active producers with votes > 0
         return;
      }

      // should use producer_schedule_type from libraries/chain/include/eosio/chain/producer_schedule.hpp
      bytes packed_schedule = pack(schedule);
      set_active_producers( packed_schedule.data(),  packed_schedule.size() );

      // not voted on
      _gstate.first_block_time_in_cycle = cycle_time;

      // derived parameters
      auto half_of_percentage = _gstate.percent_of_max_inflation_rate / 2;
      auto other_half_of_percentage = _gstate.percent_of_max_inflation_rate - half_of_percentage;
      _gstate.payment_per_block = payment_per_block(half_of_percentage);
      _gstate.payment_to_eos_bucket = payment_per_block(other_half_of_percentage);
      _gstate.blocks_per_cycle = blocks_per_producer * schedule.producers.size();

      if (_gstate.max_storage_size <_gstate.total_storage_bytes_reserved ) {
         _gstate.max_storage_size =_gstate.total_storage_bytes_reserved;
      }

      auto issue_quantity =_gstate.blocks_per_cycle * (_gstate.payment_per_block +_gstate.payment_to_eos_bucket);
      INLINE_ACTION_SENDER(eosio::token, issue)( N(eosio.token), {{N(eosio),N(active)}},
                                                 {N(eosio), issue_quantity, std::string("producer pay")} );

      set_blockchain_parameters( _gstate );
   }

   /**
    *  @pre producers must be sorted from lowest to highest and must be registered and active
    *  @pre if proxy is set then no producers can be voted for
    *  @pre if proxy is set then proxy account must exist and be registered as a proxy
    *  @pre every listed producer or proxy must have been previously registered
    *  @pre voter must authorize this action
    *  @pre voter must have previously staked some EOS for voting
    *  @pre voter->staked must be up to date
    *
    *  @post every producer previously voted for will have vote reduced by previous vote weight 
    *  @post every producer newly voted for will have vote increased by new vote amount
    *  @post prior proxy will proxied_vote_weight decremented by previous vote weight
    *  @post new proxy will proxied_vote_weight incremented by new vote weight
    *
    *  If voting for a proxy, the producer votes will not change until the proxy updates their own vote.
    */
   void system_contract::voteproducer( const account_name voter_name, const account_name proxy, const std::vector<account_name>& producers ) {
      require_auth( voter_name );

      //validate input
      if ( proxy ) {
         eosio_assert( producers.size() == 0, "cannot vote for producers and proxy at same time" );
         eosio_assert( voter_name != proxy, "cannot proxy to self" );
         require_recipient( proxy );
      } else {
         eosio_assert( producers.size() <= 30, "attempt to vote for too many producers" );
         for( size_t i = 1; i < producers.size(); ++i ) {
            eosio_assert( producers[i-1] < producers[i], "producer votes must be unique and sorted" );
         }
      }

      print( __FILE__, ":", __LINE__, "   ");
      auto voter = _voters.find(voter_name);
      eosio_assert( voter != _voters.end(), "user must stake before they can vote" ); /// staking creates voter object

      auto weight = int64_t(now() / (seconds_per_day * 7)) / double( 52 );
      double new_vote_weight = double(voter->staked) * std::pow(2,weight);

      if( voter->is_proxy ) {
         new_vote_weight += voter->proxied_vote_weight;
      }

      boost::container::flat_map<account_name, double> producer_deltas;
      for( const auto& p : voter->producers ) {
         producer_deltas[p] -= voter->last_vote_weight;
      }

      if( new_vote_weight >= 0 ) {
         for( const auto& p : producers ) {
            producer_deltas[p] += new_vote_weight;
         }
      }

      if( voter->proxy != account_name() ) {
         auto old_proxy = _voters.find( voter->proxy );
         _voters.modify( old_proxy, 0, [&]( auto& vp ) {
             vp.proxied_vote_weight -= voter->last_vote_weight;
         });
      }

      if( proxy != account_name() && new_vote_weight > 0 ) {
         auto new_proxy = _voters.find( voter->proxy );
         eosio_assert( new_proxy != _voters.end() && new_proxy->is_proxy, "invalid proxy specified" );
         _voters.modify( new_proxy, 0, [&]( auto& vp ) {
             vp.proxied_vote_weight += new_vote_weight;
         });
      }

      _voters.modify( voter, 0, [&]( auto& av ) {
         av.last_vote_weight = new_vote_weight;
         av.producers = producers;
         av.proxy     = proxy;
      });

      print( __FILE__, ":", __LINE__, "   ");
      for( const auto& pd : producer_deltas ) {
         auto pitr = _producers.find( pd.first );
         if( pitr != _producers.end() ) {
            _producers.modify( pitr, 0, [&]( auto& p ) {
               p.total_votes += pd.second;
               eosio_assert( p.total_votes >= 0, "something bad happened" );
               eosio_assert( p.active(), "producer is not active" );
            });
         }
      }
   }

   /**
    *  An account marked as a proxy can vote with the weight of other accounts which
    *  have selected it as a proxy. Other accounts must refresh their voteproducer to
    *  update the proxy's weight.    
    *
    *  @param isproxy - true if proxy wishes to vote on behalf of others, false otherwise
    *  @pre proxy must have something staked (existing row in voters table)
    *  @pre new state must be different than current state
    */
   void system_contract::regproxy( const account_name proxy, bool isproxy ) {
      require_auth( proxy );

      auto pitr = _voters.find(proxy);
      eosio_assert( pitr != _voters.end(), "proxy must have some stake first" );
      eosio_assert( !pitr->is_proxy, "account is already a proxy" );
      eosio_assert( pitr->is_proxy != isproxy, "action has no effect" );

      _voters.modify( pitr, 0, [&]( auto& p ) {
         p.is_proxy = isproxy;
      });
   }

}
