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
#include <eosiolib/privileged.h>

#include <map>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::bytes;
   using eosio::print;
   using std::map;
   using std::pair;

   template<account_name SystemAccount>
   class delegate_bandwith {
      public:
         static const account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;
         typedef typename currency::token_type system_token_type;

         struct total_resources {
            account_name owner;
            typename currency::token_type total_net_weight; 
            typename currency::token_type total_cpu_weight; 
            uint32_t total_ram = 0;

            uint64_t primary_key()const { return owner; }

            EOSLIB_SERIALIZE( total_resources, (owner)(total_net_weight)(total_cpu_weight)(total_ram) )
         };


         /**
          *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
          */
         struct delegated_bandwidth {
            account_name from;
            account_name to;
            typename currency::token_type net_weight; 
            typename currency::token_type cpu_weight; 

            uint32_t start_pending_net_withdraw = 0;
            typename currency::token_type pending_net_withdraw;
            uint64_t deferred_net_withdraw_handler = 0;

            uint32_t start_pending_cpu_withdraw = 0;
            typename currency::token_type pending_cpu_withdraw;
            uint64_t deferred_cpu_withdraw_handler = 0;

            
            uint64_t  primary_key()const { return to; }

            EOSLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight)
                              (start_pending_net_withdraw)(pending_net_withdraw)(deferred_net_withdraw_handler)
                              (start_pending_cpu_withdraw)(pending_cpu_withdraw)(deferred_cpu_withdraw_handler) )

         };

         typedef eosio::multi_index< N(totalband), total_resources>  total_resources_index_type;
         typedef eosio::multi_index< N(delband), delegated_bandwidth> del_bandwidth_index_type;

         ACTION( SystemAccount, finshundel ) {
            account_name from;
            account_name to;
         };

         ACTION( SystemAccount, delegatebw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   stake_net_quantity;
            typename currency::token_type   stake_cpu_quantity;


            EOSLIB_SERIALIZE( delegatebw, (from)(receiver)(stake_net_quantity)(stake_cpu_quantity) )
         };

         ACTION( SystemAccount, undelegatebw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   unstake_net_quantity;
            typename currency::token_type   unstake_cpu_quantity;

            EOSLIB_SERIALIZE( undelegatebw, (from)(receiver)(unstake_net_quantity)(unstake_cpu_quantity) )
         };

               /// new id options:
         //  1. hash + collision 
         //  2. incrementing count  (key=> tablename 

         static void on( const delegatebw& del ) {
            eosio_assert( del.stake_cpu_quantity.quantity >= 0, "must stake a positive amount" );
            eosio_assert( del.stake_net_quantity.quantity >= 0, "must stake a positive amount" );

            auto total_stake = del.stake_cpu_quantity + del.stake_net_quantity;
            eosio_assert( total_stake.quantity >= 0, "must stake a positive amount" );


            require_auth( del.from );

            del_bandwidth_index_type     del_index( SystemAccount, del.from );
            total_resources_index_type   total_index( SystemAccount, del.receiver );

            //eosio_assert( is_account( del.receiver ), "can only delegate resources to an existing account" );

            auto itr = del_index.find( del.receiver);
            if( itr != nullptr ) {
               del_index.emplace( del.from, [&]( auto& dbo ){
                  dbo.from       = del.from;      
                  dbo.to         = del.receiver;      
                  dbo.net_weight = del.stake_net_quantity;
                  dbo.cpu_weight = del.stake_cpu_quantity;
               });
            }
            else {
               del_index.update( *itr, del.from, [&]( auto& dbo ){
                  dbo.net_weight = del.stake_net_quantity;
                  dbo.cpu_weight = del.stake_cpu_quantity;
               });
            }

            auto tot_itr = total_index.find( del.receiver );
            if( tot_itr == nullptr ) {
               tot_itr = &total_index.emplace( del.from, [&]( auto& tot ) {
                  tot.owner = del.receiver;
                  tot.total_net_weight += del.stake_net_quantity;
                  tot.total_cpu_weight += del.stake_cpu_quantity;
               });
            } else {
               total_index.update( *tot_itr, 0, [&]( auto& tot ) {
                  tot.total_net_weight += del.stake_net_quantity;
                  tot.total_cpu_weight += del.stake_cpu_quantity;
               });
            }

            set_resource_limits( tot_itr->owner, tot_itr->total_ram, tot_itr->total_net_weight.quantity, tot_itr->total_cpu_weight.quantity, 0 );

            currency::inline_transfer( del.from, SystemAccount, total_stake, "stake bandwidth" );
         } // delegatebw

         static void on( const undelegatebw& del ) {
            eosio_assert( del.unstake_cpu_quantity.quantity >= 0, "must stake a positive amount" );
            eosio_assert( del.unstake_net_quantity.quantity >= 0, "must stake a positive amount" );

            auto total_stake = del.unstake_cpu_quantity + del.unstake_net_quantity;
            eosio_assert( total_stake.quantity >= 0, "must stake a positive amount" );

            require_auth( del.from );

            del_bandwidth_index_type     del_index( SystemAccount, del.from );
            total_resources_index_type   total_index( SystemAccount, del.receiver );

            //eosio_assert( is_account( del.receiver ), "can only delegate resources to an existing account" );

            const auto& dbw = del_index.get(del.receiver);
            eosio_assert( dbw.net_weight >= del.unstake_net_quantity, "insufficient staked net bandwidth" );
            eosio_assert( dbw.cpu_weight >= del.unstake_cpu_quantity, "insufficient staked cpu bandwidth" );

            del_index.update( dbw, del.from, [&]( auto& dbo ){
               dbo.net_weight -= del.unstake_net_quantity;
               dbo.cpu_weight -= del.unstake_cpu_quantity;

            });

            const auto& totals = total_index.get( del.receiver );
            total_index.update( totals, 0, [&]( auto& tot ) {
               tot.total_net_weight -= del.unstake_net_quantity;
               tot.total_cpu_weight -= del.unstake_cpu_quantity;
            });

            set_resource_limits( totals.owner, totals.total_ram, totals.total_net_weight.quantity, totals.total_cpu_weight.quantity, 0 );

            /// TODO: implement / enforce time delays on withdrawing
            currency::inline_transfer( SystemAccount, del.from, total_stake, "unstake bandwidth" );
         } // undelegatebw
   };
}
