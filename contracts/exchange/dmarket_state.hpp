#pragma once
#include <exchange/dexchange_state.hpp>
#include <exchange/exchange_accounts.hpp>

namespace eosio {

   /**
    * Maintains a state along with the cache of margin positions and/or limit orders.
    */
   struct dmarket_state {
      dmarket_state( account_name this_contract, symbol_type market_symbol, exchange_accounts& acnts, currency& excur );

      const dexchange_state& initial_state()const;

      void init_market( account_name manager, symbol_type market_symbol, symbol_type peg_market_symbol, extended_asset initial_deposit, double initial_price, double target_reserve_ratio );
      void market_order( account_name seller, extended_asset sell, extended_symbol receive );
      void set_price( account_name oracle, extended_asset col, asset peg );

      void save();

      private:
         symbol_name       marketid;
         dmarkets          market_table;
         dexchange_state   exstate;

         exchange_accounts&        _accounts;
         currency&                 _currencies;
         dmarkets::const_iterator  market_state_itr;
         account_name              _this_contract;
   };

}  /// namespace eosio
