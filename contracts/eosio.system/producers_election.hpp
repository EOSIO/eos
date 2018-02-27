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
#include <eosiolib/privileged.hpp>
#include <eosiolib/transaction.hpp>

#include <array>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::member;
   using eosio::bytes;
   using eosio::print;
   using eosio::transaction;

   template<account_name SystemAccount>
   class producers_election {
      public:
         static const account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;
         typedef typename currency::token_type system_token_type;

         static constexpr uint32_t max_unstake_requests = 10;
         static constexpr uint32_t unstake_pay_period = 7*24*3600; // one per week
         static constexpr uint32_t unstake_payments = 26; // during 26 weeks

         struct producer_preferences : eosio::blockchain_parameters {
            uint32_t inflation_rate; // inflation coefficient * 10000 (i.e. inflation in percent * 1000)

            producer_preferences() { bzero(this, sizeof(*this)); }

            EOSLIB_SERIALIZE_DERIVED( producer_preferences, eosio::blockchain_parameters, (inflation_rate) )
         };

         struct producer_info {
            account_name      owner;
            uint64_t          padding = 0;
            uint128_t         total_votes = 0;
            producer_preferences prefs;
            eosio::bytes      packed_key; /// a packed public key object

            uint64_t    primary_key()const { return owner;       }
            uint128_t   by_votes()const    { return total_votes; }
            bool active() const { return !packed_key.empty(); }

            EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(prefs) )
         };

         typedef eosio::multi_index< N(producervote), producer_info,
                                     indexed_by<N(prototalvote), const_mem_fun<producer_info, uint128_t, &producer_info::by_votes>  >
                                     >  producer_info_index_type;

         struct account_votes {
            account_name                owner = 0;
            account_name                proxy = 0;
            uint32_t                    last_update = 0;
            uint32_t                    is_proxy = 0;
            system_token_type           staked;
            system_token_type           unstaking;
            system_token_type           unstake_per_week;
            uint128_t                   proxied_votes = 0;
            std::vector<account_name>   producers;
            uint32_t                    deferred_trx_id = 0;
            time                        last_unstake_time = 0; //uint32

            uint64_t primary_key()const { return owner; }

            EOSLIB_SERIALIZE( account_votes, (owner)(proxy)(last_update)(is_proxy)(staked)(unstaking)(unstake_per_week)(proxied_votes)(producers)(deferred_trx_id)(last_unstake_time) )
         };

         typedef eosio::multi_index< N(accountvotes), account_votes>  account_votes_index_type;


         struct producer_config {
            account_name      owner;
            eosio::bytes      packed_key; /// a packed public key object

            uint64_t primary_key()const { return owner;       }
            EOSLIB_SERIALIZE( producer_config, (owner)(packed_key) )
         };

         typedef eosio::multi_index< N(producercfg), producer_config>  producer_config_index_type;

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

            producer_info_index_type producers_tbl( SystemAccount, SystemAccount );
            const auto* existing = producers_tbl.find( reg.producer );
            eosio_assert( !existing, "producer already registered" );

            producers_tbl.emplace( reg.producer, [&]( producer_info& info ){
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

            producer_info_index_type producers_tbl( SystemAccount, SystemAccount );
            const auto* ptr = producers_tbl.find( change.producer );
            eosio_assert( bool(ptr), "producer is not registered" );

            producers_tbl.update( *ptr, change.producer, [&]( producer_info& info ){
                  info.prefs = change.prefs;
               });
         }

         ACTION( SystemAccount, stake_vote ) {
            account_name      voter;
            system_token_type amount;

            EOSLIB_SERIALIZE( stake_vote, (voter)(amount) )
         };

         static void increase_voting_power( account_name voter, system_token_type amount ) {
            account_votes_index_type avotes( SystemAccount, SystemAccount );
            const auto* acv = avotes.find( voter );

            if( !acv ) {
               acv = &avotes.emplace( voter, [&]( account_votes& a ) {
                     a.owner = voter;
                     a.last_update = now();
                     a.proxy = 0;
                     a.is_proxy = 0;
                     a.proxied_votes = 0;
                     a.staked = amount;
                  });
            } else {
               avotes.update( *acv, 0, [&]( auto& av ) {
                     av.last_update = now();
                     av.staked += amount;
                  });

            }

            const std::vector<account_name>* producers = nullptr;
            if ( acv->proxy ) {
               auto proxy = avotes.find( acv->proxy );
               avotes.update( *proxy, 0, [&](account_votes& a) { a.proxied_votes += amount.quantity; } );
               if ( proxy->is_proxy ) { //only if proxy is still active. if proxy has been unregistered, we update proxied_votes, but don't propagate to producers
                  producers = &proxy->producers;
               }
            } else {
               producers = &acv->producers;
            }

            if ( producers ) {
               producer_info_index_type producers_tbl( SystemAccount, SystemAccount );
               for( auto p : *producers ) {
                  auto ptr = producers_tbl.find( p );
                  eosio_assert( bool(ptr), "never existed producer" ); //data corruption
                  producers_tbl.update( *ptr, 0, [&]( auto& v ) {
                        v.total_votes += amount.quantity;
                     });
               }
            }
         }

         static void update_elected_producers() {
            producer_info_index_type producers_tbl( SystemAccount, SystemAccount );
            auto& idx = producers_tbl.template get<>( N(prototalvote) );

            std::array<uint32_t, 21> target_block_size;
            std::array<uint32_t, 21> max_block_size;
            std::array<uint32_t, 21> target_block_acts_per_scope;
            std::array<uint32_t, 21> max_block_acts_per_scope;
            std::array<uint32_t, 21> target_block_acts;
            std::array<uint32_t, 21> max_block_acts;
            std::array<uint64_t, 21> max_storage_size;
            std::array<uint32_t, 21> max_transaction_lifetime;
            std::array<uint16_t, 21> max_authority_depth;
            std::array<uint32_t, 21> max_transaction_exec_time;
            std::array<uint16_t, 21> max_inline_depth;
            std::array<uint32_t, 21> max_inline_action_size;
            std::array<uint32_t, 21> max_generated_transaction_size;
            std::array<uint32_t, 21> inflation_rate;

            std::array<account_name, 21> elected;

            auto it = std::prev( idx.end() );
            size_t n = 0;
            while ( n < 21 ) {
               if ( it->active() ) {
                  elected[n] = it->owner;

                  target_block_size[n] = it->prefs.target_block_size;
                  max_block_size[n] = it->prefs.max_block_size;

                  target_block_acts_per_scope[n] = it->prefs.target_block_acts_per_scope;
                  max_block_acts_per_scope[n] = it->prefs.max_block_acts_per_scope;

                  target_block_acts[n] = it->prefs.target_block_acts;
                  max_block_acts[n] = it->prefs.max_block_acts;

                  max_storage_size[n] = it->prefs.max_storage_size;
                  max_transaction_lifetime[n] = it->prefs.max_transaction_lifetime;
                  max_authority_depth[n] = it->prefs.max_authority_depth;
                  max_transaction_exec_time[n] = it->prefs.max_transaction_exec_time;
                  max_inline_depth[n] = it->prefs.max_inline_depth;
                  max_inline_action_size[n] = it->prefs.max_inline_action_size;
                  max_generated_transaction_size[n] = it->prefs.max_generated_transaction_size;

                  inflation_rate[n] = it->prefs.inflation_rate;
                  ++n;
               }

               if (it == idx.begin()) {
                  break;
               }
               --it;
            }
            set_active_producers( elected.data(), n );
            size_t median = n/2;

            ::blockchain_parameters concensus = {
               target_block_size[median],
               max_block_size[median],
               target_block_acts_per_scope[median],
               max_block_acts_per_scope[median],
               target_block_acts[median],
               max_block_acts[median],
               max_storage_size[median],
               max_transaction_lifetime[median],
               max_transaction_exec_time[median],
               max_authority_depth[median],
               max_inline_depth[median],
               max_inline_action_size[median],
               max_generated_transaction_size[median]
            };

            set_blockchain_parameters(&concensus);
         }

         static void on( const stake_vote& sv ) {
            eosio_assert( sv.amount.quantity > 0, "must stake some tokens" );
            require_auth( sv.voter );

            increase_voting_power( sv.voter, sv.amount );
            currency::inline_transfer( sv.voter, SystemAccount, sv.amount, "stake for voting" );
         }

         ACTION( SystemAccount, unstake_vote ) {
            account_name      voter;
            system_token_type amount;

            EOSLIB_SERIALIZE( unstake_vote, (voter)(amount) )
         };

         static void on( const unstake_vote& usv ) {
            require_auth( usv.voter );
            account_votes_index_type avotes( SystemAccount, SystemAccount );
            const auto* acv = avotes.find( usv.voter );
            eosio_assert( bool(acv), "stake not found" );

            if ( 0 < usv.amount.quantity ) {
               eosio_assert( acv->staked < usv.amount, "cannot unstake more than total stake amount" );

               if (acv->deferred_trx_id) {
                  //XXX cancel_deferred_transaction(acv->deferred_trx_id);
               }

               uint32_t new_trx_id = 0;//XXX send_deferred();

               avotes.update( *acv, 0, [&](account_votes& a) {
                     a.staked -= usv.amount;
                     a.unstaking += a.unstaking + usv.amount;
                     //round up to guarantee that there will be no unpaid balance after 26 weeks, and we are able refund amount < unstake_payments
                     a.unstake_per_week = system_token_type( a.unstaking.quantity /unstake_payments + a.unstaking.quantity % unstake_payments );
                     a.deferred_trx_id = new_trx_id;
                     a.last_update = now();
                  });

               const std::vector<account_name>* producers = nullptr;
               if ( acv->proxy ) {
                  auto proxy = avotes.find( acv->proxy );
                  avotes.update( *proxy, 0, [&](account_votes& a) { a.proxied_votes -= usv.amount.quantity; } );
                  if ( proxy->is_proxy ) { //only if proxy is still active. if proxy has been unregistered, we update proxied_votes, but don't propagate to producers
                     producers = &proxy->producers;
                  }
               } else {
                  producers = &acv->producers;
               }

               if ( producers ) {
                  producer_info_index_type producers_tbl( SystemAccount, SystemAccount );
                  for( auto p : *producers ) {
                     auto prod = producers_tbl.find( p );
                     eosio_assert( bool(prod), "never existed producer" ); //data corruption
                     producers_tbl.update( *prod, 0, [&]( auto& v ) {
                           v.total_votes -= usv.amount.quantity;
                        });
                  }
               }
            } else {
               if (acv->deferred_trx_id) {
                  //XXX cancel_deferred_transaction(acv->deferred_trx_id);
               }
               avotes.update( *acv, 0, [&](account_votes& a) {
                     a.staked += a.unstaking;
                     a.unstaking.quantity = 0;
                     a.unstake_per_week.quantity = 0;
                     a.deferred_trx_id = 0;
                     a.last_update = now();
                  });
            }
         }

         ACTION(  SystemAccount, unstake_vote_deferred ) {
            account_name                voter;

            EOSLIB_SERIALIZE( unstake_vote_deferred, (voter) )
         };

         static void on( const unstake_vote_deferred& usv) {
            require_auth( usv.voter );
            account_votes_index_type avotes( SystemAccount, SystemAccount );
            const auto* acv = avotes.find( usv.voter );
            eosio_assert( bool(acv), "stake not found" );

            auto weeks = (now() - acv->last_unstake_time) / unstake_pay_period;
            eosio_assert( 0 == weeks, "less than one week since last unstaking balance transfer" );

            auto unstake_amount = std::min(weeks * acv->unstake_per_week, acv->unstaking);
            uint32_t new_trx_id = unstake_amount < acv->unstaking ? /* XXX send_deferred() */ 0 : 0;

            currency::inline_transfer( usv.voter, SystemAccount, unstake_amount, "unstake voting" );

            avotes.update( *acv, 0, [&](account_votes& a) {
                  a.unstaking -= unstake_amount;
                  a.deferred_trx_id = new_trx_id;
                  a.last_unstake_time = a.last_unstake_time + weeks * unstake_pay_period;
               });
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
               require_recipient( vp.proxy );
            } else {
               eosio_assert( vp.producers.size() <= 30, "attempt to vote for too many producers" );
               eosio_assert( std::is_sorted( vp.producers.begin(), vp.producers.end() ), "producer votes must be sorted" );
            }

            account_votes_index_type avotes( SystemAccount, SystemAccount );
            auto ptr = avotes.find( vp.voter );

            eosio_assert( bool(ptr), "no stake to vote" );
            if ( ptr->is_proxy ) {
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
               if ( old_proxy->is_proxy ) { //if proxy stoped being proxy, the votes were already taken back from producers by on( const unregister_proxy& )
                  old_producers = &old_proxy->producers;
               }
            } else {
               old_producers = &ptr->producers;
            }

            //find new producers, update new proxy if needed
            const std::vector<account_name>* new_producers = nullptr;
            if ( vp.proxy ) {
               auto new_proxy = avotes.find( vp.proxy );
               eosio_assert( new_proxy->is_proxy, "selected proxy has not elected to be a proxy" );
               avotes.update( *new_proxy, 0, [&](auto& a) { a.proxied_votes += ptr->staked.quantity; } );
               new_producers = &new_proxy->producers;
            } else {
               new_producers = &vp.producers;
            }

            producer_info_index_type producers_tbl( SystemAccount, SystemAccount );

            if ( old_producers ) { //old_producers == 0 if proxy has stoped being a proxy and votes were taken back from producers at that moment
               //revoke votes only from no longer elected
               std::vector<account_name> revoked( old_producers->size() );
               auto end_it = std::set_difference( old_producers->begin(), old_producers->end(), new_producers->begin(), new_producers->end(), revoked.begin() );
               for ( auto it = revoked.begin(); it != end_it; ++it ) {
                  auto prod = producers_tbl.find( *it );
                  eosio_assert( bool(prod), "never existed producer" ); //data corruption
                  producers_tbl.update( *prod, 0, [&]( auto& pi ) { pi.total_votes -= ptr->staked.quantity; });
               }
            }

            //update newly elected
            std::vector<account_name> elected( new_producers->size() );
            auto end_it = std::set_difference( new_producers->begin(), new_producers->end(), old_producers->begin(), old_producers->end(), elected.begin() );
            for ( auto it = elected.begin(); it != end_it; ++it ) {
               auto prod = producers_tbl.find( *it );
               eosio_assert( bool(prod), "never existed producer" ); //data corruption
               if ( vp.proxy == 0 ) { //direct voting, in case of proxy voting update total_votes even for inactive producers
                  eosio_assert( prod->active(), "can vote only for active producers" );
               }
               producers_tbl.update( *prod, 0, [&]( auto& pi ) { pi.total_votes += ptr->staked.quantity; });
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
               eosio_assert( ptr->is_proxy == 0, "account is already a proxy" );
               eosio_assert( ptr->proxy == 0, "account that uses a proxy is not allowed to become a proxy" );
               avotes.update( *ptr, 0, [&](account_votes& a) {
                     a.is_proxy = 1;
                     a.last_update = now();
                     //a.proxied_votes may be > 0, if the proxy has been unregistered, so we had to keep the value
                  });
            } else {
               avotes.emplace( reg.proxy_to_register, [&]( account_votes& a ) {
                     a.owner = reg.proxy_to_register;
                     a.last_update = now();
                     a.proxy = 0;
                     a.is_proxy = 1;
                     a.proxied_votes = 0;
                     a.staked.quantity = 0;
                  });
            }
         }

         ACTION( SystemAccount, unregister_proxy ) {
            account_name proxy_to_unregister;

            EOSLIB_SERIALIZE( unregister_proxy, (proxy_to_unregister) )
         };

         static void on( const unregister_proxy& reg ) {
            require_auth( reg.proxy_to_unregister );

            account_votes_index_type avotes( SystemAccount, SystemAccount );
            auto proxy = avotes.find( reg.proxy_to_unregister );
            eosio_assert( bool(proxy), "proxy not found" );
            eosio_assert( proxy->is_proxy == 1, "account is already a proxy" );

            producer_info_index_type producers_tbl( SystemAccount, SystemAccount );
            for ( auto p : proxy->producers ) {
               auto ptr = producers_tbl.find( p );
               eosio_assert( bool(ptr), "never existed producer" ); //data corruption
               producers_tbl.update( *ptr, 0, [&]( auto& pi ) { pi.total_votes -= proxy->proxied_votes; });
            }

            avotes.update( *proxy, 0, [&](account_votes& a) {
                     a.is_proxy = 0;
                     a.last_update = now();
                     //a.proxied_votes should be kept in order to be able to reenable this proxy in the future
               });
         }

         struct block {};

         static void on( const block& ) {
            update_elected_producers();
         }
   };
}
