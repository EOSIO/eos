/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   template <account_name Account>
   class voting;

   template <account_name Account>
   class delegate_bandwidth;
}

namespace eosio {

   using std::string;

   class token : public contract {
      public:
         token( account_name self ):contract(self){}

         void create( account_name issuer,
                      asset        maximum_supply,
                      uint8_t      issuer_can_freeze,
                      uint8_t      issuer_can_recall,  
                      uint8_t      issuer_can_whitelist );


         void issue( account_name to, asset quantity, string memo );

         void transfer( account_name from, 
                        account_name to,
                        asset        quantity,
                        string       memo );

         asset get_total_supply( const symbol_type& symbol );

      private:

         friend eosiosystem::voting<N(eosio)>;
         friend eosiosystem::delegate_bandwidth<N(eosio)>;

         struct account {
            asset    balance;
            bool     frozen    = false;
            bool     whitelist = true;

            uint64_t primary_key()const { return balance.symbol.name(); }
         };

         struct currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;
            bool           can_freeze         = true;
            bool           can_recall         = true;
            bool           can_whitelist      = true;
            bool           is_frozen          = false;
            bool           enforce_whitelist  = false;

            uint64_t primary_key()const { return supply.symbol.name(); }
         };

         typedef eosio::multi_index<N(accounts), account> accounts;
         typedef eosio::multi_index<N(stat), currency_stats> stats;

         void sub_balance( account_name owner, asset value, const currency_stats& st );
         void add_balance( account_name owner, asset value, const currency_stats& st, 
                           account_name ram_payer );
   };

   typedef std::tuple<account_name, account_name, asset, string> transfer_args;
   void inline_transfer( permission_level permissions, account_name code, transfer_args args )
   {
      action act( permissions, code, N(transfer), args );
      act.send();
   }

   typedef std::tuple<account_name, asset, string> issue_args;
   void inline_issue( permission_level permissions, account_name code, issue_args args )
   {
      action act( permissions, code, N(issue), args );
      act.send();
   }

} /// namespace eosio
