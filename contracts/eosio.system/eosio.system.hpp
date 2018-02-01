/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/db.hpp>
#include <eosiolib/reflect.hpp>

#include <eosiolib/generic_currency.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>

namespace eosiosystem {

   template<account_name SystemAccount>
   class contract {
      public:
         static const account_name system_account = N(eosio.system);
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;

         struct total_bandwidth {
            account_name owner;
            typename currency::token_type total_net_weight; 
            typename currency::token_type total_cpu_weight; 
         };

         typedef eosio::table64<SystemAccount, N(totalband), total_bandwidth>      total_bandwidth;

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
         };

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

            EOSLIB_SERIALIZE( delnetbw, (delegator)(receiver)(stake_quantity) )
         };

         ACTION( SystemAccount, undelnetbw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   stake_quantity;

            EOSLIB_SERIALIZE( delnetbw, (delegator)(receiver)(stake_quantity) )
         };

         static void on( const delnetbw& del ) {
            require_auth( del.from );
          //  require_account( receiver );

            currency::inline_transfer( del.from, SystemAccount, del.stake_quantity, "stake bandwidth" );
         }

         static void on( const regproducer& reg ) {
            require_auth( reg.producer_to_register );
         }

         static void on( const regproxy& reg ) {
            require_auth( reg.proxy_to_register );
         }


         static void apply( account_name code, action_name act ) {
            if( !eosio::dispatch<contract, regproducer, regproxy>( code, act) ) {
               if ( !eosio::dispatch<currency, typename currency::transfer_memo, typename currency::issue>( code, act ) ) {
                  assert( false, "received unexpected action" );
               }
            }
         } /// apply 
   };

} /// eosiosystem 

