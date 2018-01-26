/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>
#include <eoslib/reflect.hpp>

#include <eoslib/generic_currency.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/serialize.hpp>

namespace eosiosystem {

   template<account_name SystemAccount>
   class contract {
      public:
         static const account_name system_account = N(eosio.system);
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;

         ACTION( SystemAccount, regproducer ) {
            account_name producer_to_register;

            EOSLIB_SERIALIZE( regproducer, (producer_to_register) );
         };

         ACTION( SystemAccount, regproxy ) {
            account_name proxy_to_register;

            EOSLIB_SERIALIZE( regproxy, (proxy_to_register) );
         };

         static void on( const regproducer& reg ) {
            require_auth( reg.producer_to_register );
         }

         static void on( const regproxy& reg ) {
            require_auth( reg.proxy_to_register );
         }


         static void apply( account_name code, action_name act ) {
            if( !eosio::dispatch<contract, 
//                   typename currency::transfer_memo, 
 //                  typename currency::issue,
                   regproducer,
                   regproxy
                   >( code, act) ) 
            {
               assert( false, "received unexpected action" );
            }
         } /// apply 
   };

} /// eosiosystem 

