/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include "producers_election.hpp"
#include "delegate_bandwith.hpp"

#include <eosiolib/generic_currency.hpp>

namespace eosiosystem {

   template<account_name SystemAccount>
   class contract : public producers_election<SystemAccount>, public delegate_bandwith<SystemAccount> {
      public:
         using producers_election<SystemAccount>::on;
         using delegate_bandwith<SystemAccount>::on;
         using pe = producers_election<SystemAccount>;
         using db = delegate_bandwith<SystemAccount>;

         static const account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;

         ACTION( SystemAccount, nonce ) {
            eosio::string                   value;

            EOSLIB_SERIALIZE( nonce, (value) )
         };

         static void on( const nonce& ) {
         }

         static void apply( account_name code, action_name act ) {
            if( !eosio::dispatch<contract, typename delegate_bandwith<SystemAccount>::delegatebw,
                                 typename delegate_bandwith<SystemAccount>::undelegatebw,
                                 typename producers_election<SystemAccount>::register_proxy,
                                 typename producers_election<SystemAccount>::unregister_proxy,
                                 typename producers_election<SystemAccount>::register_producer,
                                 typename producers_election<SystemAccount>::vote_producer,
                                 typename producers_election<SystemAccount>::stake_vote,
                                 typename producers_election<SystemAccount>::unstake_vote,
                                 typename producers_election<SystemAccount>::cancel_unstake_vote_request,
                                 nonce>( code, act) ) {
               if ( !eosio::dispatch<currency, typename currency::transfer, typename currency::issue>( code, act ) ) {
                  eosio::print("Unexpected action: ", eosio::name(act), "\n");
                  eosio_assert( false, "received unexpected action");
               }
            }

         } /// apply 
   };

} /// eosiosystem 

