/**
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/asset.hpp>

namespace eosiosystem {
   
   using eosio::asset;
   using eosio::symbol;

   /**
    * @addtogroup eosiosystem
    * @{
    */

   /**
    * Uses Bancor math to create a 50/50 relay between two asset types.
    *  
    * @details The state of the bancor exchange is entirely contained within this struct. 
    * There are no external side effects associated with using this API.
    */
   struct [[eosio::table, eosio::contract("eosio.system")]] exchange_state {
      asset    supply;

      struct connector {
         asset balance;
         double weight = .5;

         EOSLIB_SERIALIZE( connector, (balance)(weight) )
      };

      connector base;
      connector quote;

      uint64_t primary_key()const { return supply.symbol.raw(); }

      asset convert_to_exchange( connector& reserve, const asset& payment );
      asset convert_from_exchange( connector& reserve, const asset& tokens );
      asset convert( const asset& from, const symbol& to );
      asset direct_convert( const asset& from, const symbol& to );
      /**
       * Given two connector balances (inp_reserve and out_reserve), and an incoming amount
       * of inp, this function calculates the delta out using Banacor equation.
       *
       * @param inp - input amount, same units as inp_reserve
       * @param inp_reserve - the input connector balance
       * @param out_reserve - the output connector balance
       *
       * @return int64_t - conversion output amount
       */
      static int64_t get_bancor_output( int64_t inp_reserve,
                                        int64_t out_reserve,
                                        int64_t inp );

      EOSLIB_SERIALIZE( exchange_state, (supply)(base)(quote) )
   };

   typedef eosio::multi_index< "rammarket"_n, exchange_state > rammarket;
   /** @}*/ // enf of @addtogroup eosiosystem
} /// namespace eosiosystem
