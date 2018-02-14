/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/db.hpp>
#include <eosiolib/reflect.hpp>
#include <eosiolib/print.hpp>

#include <eosiolib/generic_currency.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.h>

namespace eosiosystem {

   template<account_name SystemAccount>
   class contract {
      public:
         static const account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;

         struct total_resources {
            account_name owner;
            typename currency::token_type total_net_weight; 
            typename currency::token_type total_cpu_weight; 
            uint32_t total_ram = 0;

            uint64_t primary_key()const { return owner; }

            EOSLIB_SERIALIZE( total_resources, (owner)(total_net_weight)(total_cpu_weight)(total_ram) );
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
                              (start_pending_cpu_withdraw)(pending_cpu_withdraw)(deferred_cpu_withdraw_handler) );

         };


         typedef eosio::multi_index< N(totalband), total_resources>  total_resources_index_type;
         typedef eosio::multi_index< N(delband), delegated_bandwidth> del_bandwidth_index_type;


         ACTION( SystemAccount, finshundel ) {
            account_name from;
            account_name to;
         };

         ACTION( SystemAccount, regproducer ) {
            account_name producer_to_register;

            EOSLIB_SERIALIZE( regproducer, (producer_to_register) );
         };

         ACTION( SystemAccount, regproxy ) {
            account_name proxy_to_register;

            EOSLIB_SERIALIZE( regproxy, (proxy_to_register) );
         };

         ACTION( SystemAccount, delnetbw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   stake_quantity;


            EOSLIB_SERIALIZE( delnetbw, (from)(receiver)(stake_quantity) )
         };

         ACTION( SystemAccount, undelnetbw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   stake_quantity;

            EOSLIB_SERIALIZE( delnetbw, (delegator)(receiver)(stake_quantity) )
         };

         ACTION( SystemAccount, nonce ) {
            eosio::string                   value;

            EOSLIB_SERIALIZE( nonce, (value) );
         };

         /// new id options:
         //  1. hash + collision 
         //  2. incrementing count  (key=> tablename 

         static void on( const delnetbw& del ) {
            require_auth( del.from );

            del_bandwidth_index_type     del_index( SystemAccount, del.from );
            total_resources_index_type   total_index( SystemAccount, del.receiver );

            //eosio_assert( is_account( del.receiver ), "can only delegate resources to an existing account" );

            auto itr = del_index.find( del.receiver);
            if( itr != nullptr ) {
               del_index.emplace( del.from, [&]( auto& dbo ){
                  dbo.from       = del.from;      
                  dbo.to         = del.receiver;      
                  dbo.net_weight = del.stake_quantity;
               });
            }
            else {
               del_index.update( *itr, del.from, [&]( auto& dbo ){
                  dbo.net_weight = del.stake_quantity;
               });
            }

            auto tot_itr = total_index.find( del.receiver );
            if( tot_itr == nullptr ) {
               tot_itr = &total_index.emplace( del.from, [&]( auto& tot ) {
                  tot.owner = del.receiver;
                  tot.total_net_weight += del.stake_quantity;
               });
            } else {
               total_index.update( *tot_itr, 0, [&]( auto& tot ) {
                  tot.total_net_weight += del.stake_quantity;
               });
            }

            set_resource_limits( tot_itr->owner, tot_itr->total_ram, tot_itr->total_net_weight.quantity, tot_itr->total_cpu_weight.quantity, 0 );

            currency::inline_transfer( del.from, SystemAccount, del.stake_quantity, "stake bandwidth" );
         } // delnetbw

         static void on( const regproducer& reg ) {
            require_auth( reg.producer_to_register );
         }

         static void on( const regproxy& reg ) {
            require_auth( reg.proxy_to_register );
         }

         static void on( const nonce& ) {
         }

         static void apply( account_name code, action_name act ) {

            if( !eosio::dispatch<contract, regproducer, regproxy, delnetbw, nonce>( code, act) ) {
               if ( !eosio::dispatch<currency, typename currency::transfer, typename currency::issue>( code, act ) ) {
                  eosio::print("Unexpected action: ", eosio::name(act), "\n");
                  eosio_assert( false, "received unexpected action");
               }
            }

         } /// apply 
   };

} /// eosiosystem 

