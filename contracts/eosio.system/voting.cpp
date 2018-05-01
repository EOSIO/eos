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
   void system_contract::regproducer( const account_name producer, const public_key& producer_key, const std::string& url ) { //, const eosio_parameters& prefs ) {
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

      auto prod = _producers.find( producer );
      eosio_assert( prod != producers_tbl.end(), "producer not found" );

      _producers.modify( prod, 0, [&]( producer_info& info ){
         info.producer_key = public_key();
      });
   }

   void system_contract::adjust_voting_power( account_name acnt, int64_t delta ) {
      auto voter = _voters.find( acnt );

      if( voter == _voters.end() ) {
         voter = _voters.emplace( acnt, [&]( voter_info& a ) {
            a.owner = acnt;
            a.staked.amount = delta; 
         });
      } 

      auto weight = int64_t(now() / (seconds_per_day * 7)) / double( 52 );
      double new_vote_weight = double(voter->staked.amount + delta) * std::pow(2,weight) + voter->proxied_vote_weight;

      auto delta_vote_weight = new_vote_weight - voter->last_vote_weight;

      _voters.modify( voter, 0, [&]( auto& av ) {
         av.staked.amount += delta;
         av.last_vote_weight = new_vote_weight;
         eosio_assert( av.staked.amount >= 0, "underflow" );
      });

      const std::vector<account_name>* producers = nullptr;
      if ( voter->proxy ) {
         auto proxy = voters_tbl.find( voter->proxy );
         eosio_assert( proxy != voters_tbl.end(), "selected proxy not found" ); 

         _voters.modify( proxy, 0, [&](voter_info& a) { 
            a.proxied_vote_weight += delta_vote_weight;
         } );

      } else {
         for( auto p : voter->producers ) {
            auto prod = _producers.find( p );
            _producers.modify( prod, 0, [&]( auto& pro ) {
                 pro.total_votes += delta_vote_weight;
            });
         }
      }
   }

   eosio_global_state system_contract::get_default_parameters() {
      eosio_global_state dp;
      get_blockchain_parameters(dp);
      return dp;
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
    *  @pre producers must be sorted from lowest to highest
    *  @pre if proxy is set then no producers can be voted for
    *  @pre every listed producer or proxy must have been previously registered
    *  @pre voter must authorize this action
    *  @pre voter must have previously staked some EOS for voting
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

      auto voter = _voters.find(voter_name);
      eosio_assert( voter != _voters.end(), "user must stake before they can vote" ); /// staking creates voter object

      auto weight = int64_t(now() / (seconds_per_day * 7)) / double( 52 );
      double new_vote_weight = double(voter->staked) * std::pow(2,weight) + voter->proxied_vote_weight;


      flat_map<account_name, double> producer_deltas;
      for( const auto& p : voter->producers ) {
         producer_deltas[p] -= voter->last_vote_weight;
      }

      for( const auto& p : producers ) {
         producer_deltas[p] += voter->last_vote_weight;
      }

      if( voter->proxy ) {
         if( voter->proxy != proxy ) {

         } else {

         }
      } else { 

      }


      _voters.modify( voter, 0, [&]( auto& av ) {
         av.last_vote_weight = new_vote_weight;
         av.producers = producers;
         av.proxy     = proxy;
      });



      auto voter_it = _voters.find( voter );

      /// remove all votes from existing producers or proxy
      adjust_voting_power( voter, -voter_it->staked.amount );

      /// update producer list


      /// add all votes to new producers or proxy
      adjust_voting_power( voter, voter_it->staked.amount );






      eosio_assert( 0 <= voter_it->staked.amount, "negative stake" );
      eosio_assert( voter_it != voters_tbl.end() && ( 0 < voter_it->staked.amount || ( voter_it->is_proxy && 0 < voter_it->proxied_votes ) ), "no stake to vote" );
      if ( voter_it->is_proxy ) {
         eosio_assert( proxy == 0 , "account registered as a proxy is not allowed to use a proxy" );
      }

      //find old producers, update old proxy if needed
      const std::vector<account_name>* old_producers = nullptr;
      if( voter_it->proxy ) {
         if ( voter_it->proxy == proxy ) {
            return; // nothing changed
         }
         auto old_proxy = voters_tbl.find( voter_it->proxy );
         eosio_assert( old_proxy != voters_tbl.end(), "old proxy not found" ); //data corruption
         voters_tbl.modify( old_proxy, 0, [&](auto& a) { a.proxied_votes -= uint64_t(voter_it->staked.amount); } );
         if ( old_proxy->is_proxy ) { //if proxy stopped being a proxy, the votes were already taken back from producers by on( const unregister_proxy& )
            old_producers = &old_proxy->producers;
         }
      } else {
         old_producers = &voter_it->producers;
      }

      //find new producers, update new proxy if needed
      const std::vector<account_name>* new_producers = nullptr;
      if ( proxy ) {
         auto new_proxy = voters_tbl.find( proxy );
         eosio_assert( new_proxy != voters_tbl.end() && new_proxy->is_proxy, "proxy not found" );
         voters_tbl.modify( new_proxy, 0, [&](auto& a) { a.proxied_votes += uint64_t(voter_it->staked.amount); } );
         new_producers = &new_proxy->producers;
      } else {
         new_producers = &producers;
      }

      producers_table producers_tbl( _self, _self );
      uint128_t votes = uint64_t(voter_it->staked.amount);
      if ( voter_it->is_proxy ) {
         votes += voter_it->proxied_votes;
      }

      if ( old_producers ) { //old_producers == nullptr if proxy has stopped being a proxy and votes were taken back from the producers at that moment
         //revoke votes only from no longer elected
         std::vector<account_name> revoked( old_producers->size() );
         auto end_it = std::set_difference( old_producers->begin(), old_producers->end(), new_producers->begin(), new_producers->end(), revoked.begin() );
         for ( auto it = revoked.begin(); it != end_it; ++it ) {
            auto prod = producers_tbl.find( *it );
            eosio_assert( prod != producers_tbl.end(), "never existed producer" ); //data corruption
            producers_tbl.modify( prod, 0, [&]( auto& pi ) { pi.total_votes -= votes; } );
         }
      }

      //update newly elected
      std::vector<account_name> elected( new_producers->size() );
      auto end_it = elected.begin();
      if( old_producers ) {
         end_it = std::set_difference( new_producers->begin(), new_producers->end(), old_producers->begin(), old_producers->end(), elected.begin() );
      } else {
         end_it = std::copy( new_producers->begin(), new_producers->end(), elected.begin() );
      }
      for ( auto it = elected.begin(); it != end_it; ++it ) {
         auto prod = producers_tbl.find( *it );
         eosio_assert( prod != producers_tbl.end(), "producer is not registered" );
         if ( proxy == 0 ) { //direct voting, in case of proxy voting update total_votes even for inactive producers
            eosio_assert( prod->active(), "producer is not currently registered" );
         }
         producers_tbl.modify( prod, 0, [&]( auto& pi ) { pi.total_votes += votes; } );
      }

      // save new values to the account itself
      voters_tbl.modify( voter_it, 0, [&](voter_info& a) {
            a.proxy = proxy;
            a.last_update = now();
            a.producers = producers;
         });
   }

   void system_contract::regproxy( const account_name proxy ) {
      require_auth( proxy );

      voters_table voters_tbl( _self, _self );
      auto proxy_it = voters_tbl.find( proxy );
      if ( proxy_it != voters_tbl.end() ) {
         eosio_assert( proxy_it->is_proxy == 0, "account is already a proxy" );
         eosio_assert( proxy_it->proxy == 0, "account that uses a proxy is not allowed to become a proxy" );
         voters_tbl.modify( proxy_it, 0, [&](voter_info& a) {
               a.is_proxy = 1;
               a.last_update = now();
               //a.proxied_votes may be > 0, if the proxy has been unregistered, so we had to keep the value
            });
         if ( 0 < proxy_it->proxied_votes ) {
            producers_table producers_tbl( _self, _self );
            for ( auto p : proxy_it->producers ) {
               auto prod = producers_tbl.find( p );
               eosio_assert( prod != producers_tbl.end(), "never existed producer" ); //data corruption
               producers_tbl.modify( prod, 0, [&]( auto& pi ) { pi.total_votes += proxy_it->proxied_votes; });
            }
         }
      } else {
         voters_tbl.emplace( proxy, [&]( voter_info& a ) {
               a.owner = proxy;
               a.last_update = now();
               a.proxy = 0;
               a.is_proxy = 1;
               a.proxied_votes = 0;
               a.staked.amount = 0;
            });
      }
   }

   void system_contract::unregproxy( const account_name proxy ) {
      require_auth( proxy );

      voters_table voters_tbl( _self, _self );
      auto proxy_it = voters_tbl.find( proxy );
      eosio_assert( proxy_it != voters_tbl.end(), "proxy not found" );
      eosio_assert( proxy_it->is_proxy == 1, "account is not a proxy" );

      voters_tbl.modify( proxy_it, 0, [&](voter_info& a) {
            a.is_proxy = 0;
            a.last_update = now();
            //a.proxied_votes should be kept in order to be able to reenable this proxy in the future
         });

      if ( 0 < proxy_it->proxied_votes ) {
         producers_table producers_tbl( _self, _self );
         for ( auto p : proxy_it->producers ) {
            auto prod_it = producers_tbl.find( p );
            eosio_assert( prod_it != producers_tbl.end(), "never existed producer" ); //data corruption
            producers_tbl.modify( prod_it, 0, [&]( auto& pi ) { pi.total_votes -= proxy_it->proxied_votes; });
         }
      }
   }

}
