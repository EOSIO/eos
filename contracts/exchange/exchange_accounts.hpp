#pragma once
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>

namespace eosio {

   using boost::container::flat_map;

   /**
    *  Each user has their own account with the exchange contract that keeps track
    *  of how much a user has on deposit for each extended asset type. The assumption
    *  is that storing a single flat map of all balances for a particular user will
    *  be more practical than breaking this down into a multi-index table sorted by
    *  the extended_symbol.  
    */
   struct exaccount {
      account_name                         owner;
      flat_map<extended_symbol, int64_t>   balances;

      uint64_t primary_key() const { return owner; }
      EOSLIB_SERIALIZE( exaccount, (owner)(balances) )
   };

   typedef eosio::multi_index<N(exaccounts), exaccount> exaccounts;


   /**
    *  Provides an abstracted interface around storing balances for users. This class
    *  caches tables to make multiple accesses effecient.
    */
   struct exchange_accounts {
      exchange_accounts( account_name code ):_this_contract(code){}

      void adjust_balance( account_name owner, extended_asset delta, const string& reason = string() ); 

      private: 
         account_name _this_contract;
         /**
          *  Keep a cache of all accounts tables we access
          */
         flat_map<account_name, std::unique_ptr<exaccounts>> exaccounts_cache;
   };
} /// namespace eosio
