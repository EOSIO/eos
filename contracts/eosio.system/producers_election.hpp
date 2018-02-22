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
#include <eosiolib/transaction.hpp>

#include <map>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::member;
   using eosio::bytes;
   using eosio::print;
   using eosio::transaction;
   using std::map;
   using std::pair;

   template<account_name SystemAccount>
   class producers_election {
      public:
         static const account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;
         typedef typename currency::token_type system_token_type;

         static constexpr uint32_t max_unstake_requests = 10;
         static constexpr uint32_t voting_stake_freez_time = 180*24*3600;

         struct producer_preferences {
            uint32_t max_blk_size;
            uint32_t target_blk_size;

            uint64_t max_storage_size;
            uint64_t rescource_window_size;

            uint32_t max_blk_cpu;
            uint32_t target_blk_cpu;

            uint16_t inflation_rate; // inflation in percents * 10000;

            uint32_t max_trx_lifetime;
            uint16_t max_transaction_recursion;

            producer_preferences() { bzero(this, sizeof(*this)); }

            EOSLIB_SERIALIZE( producer_preferences, (max_blk_size)(target_blk_size)(max_storage_size)(rescource_window_size)
                              (max_blk_cpu)(target_blk_cpu)(inflation_rate)(max_trx_lifetime)(max_transaction_recursion) )
         };

         struct producer_info {
            account_name      owner;
            uint64_t          padding = 0;
            uint128_t         total_votes;
            producer_preferences prefs;

            uint64_t    primary_key()const { return owner;       }
            uint128_t   by_votes()const    { return total_votes; }

            EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(prefs) )
         };

         typedef eosio::multi_index< N(producervote), producer_info,
                                     indexed_by<N(prototalvote), const_mem_fun<producer_info, uint128_t, &producer_info::by_votes>  >
                                     >  producer_info_index_type;

         struct account_votes {
            account_name                owner;
            account_name                proxy;
            uint32_t                    last_update;
            uint32_t                    i_am_proxy;
            uint128_t                   proxied_votes;
            system_token_type           staked;
            std::vector<account_name>   producers;

            uint64_t primary_key()const { return owner; }

            EOSLIB_SERIALIZE( account_votes, (owner)(proxy)(last_update)(i_am_proxy)(staked)(producers) )
         };
         typedef eosio::multi_index< N(accountvotes), account_votes>  account_votes_index_type;


         struct producer_config {
            account_name      owner;
            eosio::bytes      packed_key; /// a packed public key object

            uint64_t primary_key()const { return owner;       }
            EOSLIB_SERIALIZE( producer_config, (owner)(packed_key) )
         };
         typedef eosio::multi_index< N(producercfg), producer_config>  producer_config_index_type;

         struct unstake_request {
            uint64_t id;
            account_name account;
            system_token_type amount;
            time refund_time;

            uint64_t primary_key() const { return id; }
            uint64_t rt() const { return refund_time; }
            EOSLIB_SERIALIZE( unstake_request, (id)(account)(amount)(refund_time) )
         };

         typedef eosio::multi_index< N(unstakerequests), unstake_request,
                                     indexed_by<N(bytime), const_mem_fun<unstake_request, uint64_t, &unstake_request::rt> >
                                     > unstake_requests_table;

         struct unstake_requests_count {
            account_name account;
            uint16_t count;
            uint64_t primary_key() const { return account; }
            EOSLIB_SERIALIZE( unstake_requests_count, (account)(count) )
         };

         typedef eosio::multi_index< N(unstakecount), unstake_requests_count> unstake_requests_counts_table;

         ACTION( SystemAccount, register_producer ) {
            account_name producer;
            bytes        producer_key;
            producer_preferences prefs;

            EOSLIB_SERIALIZE( register_producer, (producer)(producer_key)(prefs) )
         };

         /**
          *  This method will create a producer_config and producer_info object for 'producer' 
          *
          *  @pre producer is not already registered
          *  @pre producer to register is an account
          *  @pre authority of producer to register 
          *  
          */
         static void on( const register_producer& reg ) {
            require_auth( reg.producer );

            producer_info_index_type producers( SystemAccount, SystemAccount );
            const auto* existing = producers.find( reg.producer );
            eosio_assert( !existing, "producer already registered" );

            producers.emplace( reg.producer, [&]( producer_info& info ){
                  info.owner       = reg.producer;
                  info.total_votes = 0;
                  info.prefs = reg.prefs;
               });

            producer_config_index_type proconfig( SystemAccount, SystemAccount );
            proconfig.emplace( reg.producer, [&]( auto& pc ) {
                  pc.owner      = reg.producer;
                  pc.packed_key = reg.producer_key;
               });
         }

         ACTION( SystemAccount, change_producer_preferences ) {
            account_name producer;
            bytes        producer_key;
            producer_preferences prefs;

            EOSLIB_SERIALIZE( register_producer, (producer)(producer_key)(prefs) )
         };

         static void on( const change_producer_preferences& change) {
            require_auth( change.producer );

            producer_info_index_type producers( SystemAccount, SystemAccount );
            const auto* ptr = producers.find( change.producer );
            eosio_assert( bool(ptr), "producer is not registered" );

            producers.update( *ptr, change.producer, [&]( producer_info& info ){
                  info.prefs = change.prefs;
               });
         }

         ACTION( SystemAccount, stake_vote ) {
            account_name      voter;
            system_token_type amount;

            EOSLIB_SERIALIZE( stake_vote, (voter)(amount) )
         };

         static void on( const stake_vote& sv ) {
            eosio_assert( sv.amount.quantity > 0, "must stake some tokens" );
            require_auth( sv.voter );

            account_votes_index_type avotes( SystemAccount, SystemAccount );

            const auto* acv = avotes.find( sv.voter );
            if( !acv ) {
               acv = &avotes.emplace( sv.voter, [&]( account_votes& a ) {
                     a.owner = sv.voter;
                     a.last_update = now();
                     a.proxy = 0;
                     a.i_am_proxy = 0;
                     a.proxied_votes = 0;
                     a.staked.quantity = 0;
                  });
            }

            producer_info_index_type producers( SystemAccount, SystemAccount );

            for( auto p : acv->producers ) {
               producers.update( producers.get( p ), 0, [&]( auto& v ) {
                     v.total_votes += sv.amount.quantity;
                  });
            }

            avotes.update( *acv, 0, [&]( auto& av ) {
                  av.last_update = now();
                  av.staked += sv.amount;
               });

            currency::inline_transfer( sv.voter, SystemAccount, sv.amount, "stake for voting" );
         }

         ACTION( SystemAccount, unstake_vote ) {
            account_name      voter;
            system_token_type amount;

            EOSLIB_SERIALIZE( unstake_vote, (voter)(amount) )
         };

         static void on( const unstake_vote& usv ) {
            eosio_assert( usv.amount.quantity > 0, "unstake amount should be > 0" );
            require_auth( usv.voter );

            unstake_requests_counts_table counts( SystemAccount, SystemAccount );
            auto ptr = counts.find( usv.voter );
            eosio_assert( !ptr || ptr->count < max_unstake_requests, "unstake requests limit exceeded");

            if ( ptr ) {
               counts.update(*ptr, usv.voter, [&](auto& r) { ++r.count; });
            } else {
               counts.emplace(usv.voter, [&](auto& r) { r.count = 1; });
            }

            unstake_requests_table requests( SystemAccount, SystemAccount );
            auto pk = requests.available_primary_key();
            requests.emplace( usv.voter, [&](unstake_request& r) {
                  r.id = pk;
                  r.account = usv.voter;
                  r.refund_time = now() + voting_stake_freez_time;
               });

            account_votes_index_type avotes( SystemAccount, SystemAccount );

            const auto* acv = avotes.find( usv.voter );
            eosio_assert( bool(acv), "stake not found" );

            eosio_assert( acv->staked.quantity < usv.amount.quantity, "attempt to unstake more than total stake amount" );

            producer_info_index_type producers( SystemAccount, SystemAccount );

            for( auto p : acv->producers ) {
               producers.update( producers.get( p ), 0, [&]( auto& v ) {
                     v.total_votes -= usv.amount.quantity;
                  });
            }

            if ( usv.amount < acv->staked ) {
               avotes.update( *acv, 0, [&]( auto& av ) {
                     av.last_update = now();
                     av.staked -= usv.amount;
                  });
            } else {
               eosio_assert( usv.amount == acv->staked, "unstaking more than is at staked" );
               avotes.remove( *acv );
            }
         }

         ACTION( SystemAccount, vote_producer ) {
            account_name                voter;
            account_name                proxy;
            std::vector<account_name>   producers;

            EOSLIB_SERIALIZE( vote_producer, (voter)(proxy)(producers) )
         };

         /**
          *  @pre vp.producers must be sorted from lowest to highest
          *  @pre if proxy is set then no producers can be voted for
          *  @pre every listed producer or proxy must have been previously registered
          *  @pre vp.voter must authorize this action
          *  @pre voter must have previously staked some EOS for voting
          */
         static void on( const vote_producer& vp ) {
            require_auth( vp.voter );

            //validate input
            if ( vp.proxy ) {
               eosio_assert( vp.producers.size() == 0, "cannot vote for producers and proxy at same time" );
            } else {
               eosio_assert( vp.producers.size() <= 30, "attempt to vote for too many producers" );
               eosio_assert( std::is_sorted( vp.producers.begin(), vp.producers.end() ), "producer votes must be sorted" );
            }

            account_votes_index_type avotes( SystemAccount, SystemAccount );
            auto ptr = avotes.find( vp.voter );

            eosio_assert( bool(ptr), "no stake to vote" );
            if ( ptr->i_am_proxy ) {
               eosio_assert( vp.proxy == 0 , "accounts elected to be proxy are not allowed to use another proxy" );
            }

            //find old producers, update old proxy if needed
            const std::vector<account_name>* old_producers = nullptr;
            if( ptr->proxy ) {
               if ( ptr->proxy == vp.proxy ) {
                  return; // nothing changed
               }
               auto old_proxy = avotes.find( ptr->proxy );
               avotes.update( *old_proxy, 0, [&](auto& a) { a.proxied_votes -= ptr->staked.quantity; } );
               old_producers = &old_proxy->producers;
            } else {
               old_producers = &ptr->producers;
            }

            //find new producers, update new proxy if needed
            const std::vector<account_name>* new_producers = nullptr;
            if ( vp.proxy ) {
               auto new_proxy = avotes.find( ptr->proxy );
               avotes.update( *new_proxy, 0, [&](auto& a) { a.proxied_votes += ptr->staked.quantity; } );
               new_producers = &new_proxy->producers;
            } else {
               new_producers = &vp.producers;
            }

            producer_info_index_type producers( SystemAccount, SystemAccount );

            //revoke votes only from no longer elected
            std::vector<account_name> revoked( old_producers->size() );
            auto end_it = std::set_difference( old_producers->begin(), old_producers->end(), new_producers->begin(), new_producers->end(), revoked.begin() );
            for ( auto it = revoked.begin(); it != end_it; ++it ) {
               producers.update( producers.get( *it ), 0, [&]( auto& pi ) { pi.total_votes -= ptr->staked.quantity; });
            }

            //update newly elected
            std::vector<account_name> elected( new_producers->size() );
            end_it = std::set_difference( new_producers->begin(), new_producers->end(), old_producers->begin(), old_producers->end(), elected.begin() );
            for ( auto it = elected.begin(); it != end_it; ++it ) {
               producers.update( producers.get( *it ), 0, [&]( auto& pi ) { pi.total_votes += ptr->staked.quantity; });
            }

            // save new values to the account itself
            avotes.update( *ptr, 0, [&](account_votes& a) {
                  a.proxy = vp.proxy;
                  a.last_update = now();
                  a.producers = vp.producers;
               });
         }

         ACTION( SystemAccount, register_proxy ) {
            account_name proxy_to_register;

            EOSLIB_SERIALIZE( register_proxy, (proxy_to_register) )
         };

         static void on( const register_proxy& reg ) {
            require_auth( reg.proxy_to_register );

            account_votes_index_type avotes( SystemAccount, SystemAccount );
            auto ptr = avotes.find( reg.proxy_to_register );
            if ( ptr ) {
               eosio_assert( ptr->i_am_proxy == 0, "account is already a proxy" );
               eosio_assert( ptr->proxy == 0, "account that uses a proxy is not allowed to become a proxy" );
               avotes.update( *ptr, 0, [&](account_votes& a) {
                     a.i_am_proxy = 1;
                     a.last_update = now();
                  });
            } else {
               avotes.emplace( reg.proxy_to_register, [&]( account_votes& a ) {
                     a.owner = reg.proxy_to_register;
                     a.last_update = now();
                     a.proxy = 0;
                     a.i_am_proxy = 1;
                     a.proxied_votes = 0;
                     a.staked.quantity = 0;
                  });
            }
         }

         struct block {};

         static void on( const block& ) {
         }
   };
}
