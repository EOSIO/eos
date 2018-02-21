/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/print.hpp>

#include <eosiolib/generic_currency.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>

#include <map>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::bytes;
   using eosio::print;
   using std::map;
   using std::pair;

   template<account_name SystemAccount>
   class producers_election {
      public:
         static const account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;
         typedef typename currency::token_type system_token_type;

         struct producer_votes {
               account_name      owner;
               uint64_t          padding = 0;
               uint128_t         total_votes;

               uint64_t    primary_key()const { return owner;       }
               uint128_t   by_votes()const    { return total_votes; }

               EOSLIB_SERIALIZE( producer_votes, (owner)(total_votes) )
         };

         typedef eosio::multi_index< N(producervote), producer_votes,
                                     indexed_by<N(prototalvote), const_mem_fun<producer_votes, uint128_t, &producer_votes::by_votes>  >
                                     >  producer_votes_index_type;

         struct account_votes {
               account_name                owner;
               account_name                proxy;
               uint32_t                    last_update;
               system_token_type           staked;
               std::vector<account_name>   producers;

               uint64_t primary_key()const { return owner; }

               EOSLIB_SERIALIZE( account_votes, (owner)(proxy)(last_update)(staked)(producers) )
         };
         typedef eosio::multi_index< N(accountvotes), account_votes>  account_votes_index_type;


         struct producer_config {
               account_name      owner;
               eosio::bytes      packed_key; /// a packed public key object

               uint64_t primary_key()const { return owner;       }
               EOSLIB_SERIALIZE( producer_config, (owner)(packed_key) )
         };
         typedef eosio::multi_index< N(producercfg), producer_config>  producer_config_index_type;

         ACTION( SystemAccount, regproducer ) {
            account_name producer;
            bytes        producer_key;

            EOSLIB_SERIALIZE( regproducer, (producer)(producer_key) )
         };

         /**
          *  This method will create a producer_config and producer_votes object for 'producer' 
          *
          *  @pre producer is not already registered
          *  @pre producer to register is an account
          *  @pre authority of producer to register 
          *  
          */
         static void on( const regproducer& reg ) {
            auto producer = reg.producer;
            require_auth( producer );

            producer_votes_index_type votes( SystemAccount, SystemAccount );
            const auto* existing = votes.find( producer );
            eosio_assert( !existing, "producer already registered" );

            votes.emplace( producer, [&]( auto& pv ){
                  pv.owner       = producer;
                  pv.total_votes = 0;
               });

            producer_config_index_type proconfig( SystemAccount, SystemAccount );
            proconfig.emplace( producer, [&]( auto& pc ) {
                  pc.owner      = producer;
                  pc.packed_key = reg.producer_key;
               });
         }

         ACTION( SystemAccount, stakevote ) {
            account_name      voter;
            system_token_type amount;

            EOSLIB_SERIALIZE( stakevote, (voter)(amount) )
         };

         static void on( const stakevote& sv ) {
            print( "on stake vote\n" );
            eosio_assert( sv.amount.quantity > 0, "must stake some tokens" );
            require_auth( sv.voter );

            account_votes_index_type avotes( SystemAccount, SystemAccount );

            const auto* acv = avotes.find( sv.voter );
            if( !acv ) {
               acv = &avotes.emplace( sv.voter, [&]( auto& av ) {
                     av.owner = sv.voter;
                     av.last_update = now();
                     av.proxy = 0;
                  });
            }

            uint128_t old_weight = acv->staked.quantity;
            uint128_t new_weight = old_weight + sv.amount.quantity;

            producer_votes_index_type votes( SystemAccount, SystemAccount );

            for( auto p : acv->producers ) {
               votes.update( votes.get( p ), 0, [&]( auto& v ) {
                     v.total_votes -= old_weight;
                     v.total_votes += new_weight;
                  });
            }

            avotes.update( *acv, 0, [&]( auto av ) {
                  av.last_update = now();
                  av.staked += sv.amount;
               });
            
            currency::inline_transfer( sv.voter, SystemAccount, sv.amount, "stake for voting" );
         }

         ACTION( SystemAccount, voteproducer ) {
            account_name                voter;
            account_name                proxy;
            std::vector<account_name>   producers;

            EOSLIB_SERIALIZE( voteproducer, (voter)(proxy)(producers) )
               };

         /**
          *  @pre vp.producers must be sorted from lowest to highest
          *  @pre if proxy is set then no producers can be voted for
          *  @pre every listed producer or proxy must have been previously registered
          *  @pre vp.voter must authorize this action
          *  @pre voter must have previously staked some EOS for voting
          */
         static void on( const voteproducer& vp ) {
            eosio_assert( std::is_sorted( vp.producers.begin(), vp.producers.end() ), "producer votes must be sorted" );
            eosio_assert( vp.producers.size() <= 30, "attempt to vote for too many producers" );
            if( vp.proxy != 0 ) eosio_assert( vp.producers.size() == 0, "cannot vote for producers and proxy at same time" );

            require_auth( vp.voter );

            account_votes_index_type avotes( SystemAccount, SystemAccount );
            const auto& existing = avotes.get( vp.voter );

            std::map<account_name, pair<uint128_t, uint128_t> > producer_vote_changes;

            uint128_t old_weight = existing.staked.quantity; /// old time
            uint128_t new_weight = old_weight; /// TODO: update for current weight

            for( const auto& p : existing.producers )
               producer_vote_changes[p].first = old_weight;
            for( const auto& p : vp.producers ) 
               producer_vote_changes[p].second = new_weight;

            producer_votes_index_type votes( SystemAccount, SystemAccount );
            for( const auto& delta : producer_vote_changes ) {
               if( delta.second.first != delta.second.second ) {
                  const auto& provote = votes.get( delta.first );
                  votes.update( provote, 0, [&]( auto& pv ){
                        pv.total_votes -= delta.second.first;
                        pv.total_votes += delta.second.second;
                     });
               }
            }

            avotes.update( existing, 0, [&]( auto& av ) {
                  av.proxy = vp.proxy;
                  av.last_update = now();
                  av.producers = vp.producers;
               });
         }

         ACTION( SystemAccount, regproxy ) {
            account_name proxy_to_register;

            EOSLIB_SERIALIZE( regproxy, (proxy_to_register) )
         };

         static void on( const regproxy& reg ) {
            require_auth( reg.proxy_to_register );

         }
   };
}
