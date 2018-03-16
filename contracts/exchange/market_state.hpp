#pragma once
#include <exchange/exchange_state.hpp>
#include <exchange/exchange_accounts.hpp>

namespace eosio {

   /**
    *  We calculate a unique scope for each market/borrowed_symbol/collateral_symbol and then
    *  instantiate a table of margin positions... with in this table each user has exactly
    *  one position and therefore the owner can serve as the primary key.
    */
   struct margin_position {
      account_name     owner;
      extended_asset   borrowed;
      extended_asset   collateral;
      double           call_price = 0;

      uint64_t get_call()const { return uint64_t(1000000*call_price); }
      uint64_t primary_key()const { return owner; }

      EOSLIB_SERIALIZE( margin_position, (owner)(borrowed)(collateral)(call_price) )
   };

   typedef eosio::multi_index<N(margins), margin_position,
           indexed_by<N(callprice), eosio::const_mem_fun<margin_position, uint64_t, &margin_position::get_call> >
   > margins;


   struct loan_position {
      account_name     owner; /// the owner
      double           interest_shares; /// the number of shares in the total lent pool

      uint64_t primary_key()const  { return owner; }

      EOSLIB_SERIALIZE( loan_position, (owner)(interest_shares) )
   };

   typedef eosio::multi_index<N(loans), loan_position> loans;

   /**
    * Maintains a state along with the cache of margin positions and/or limit orders.
    */
   struct market_state {
      market_state( account_name this_contract, symbol_type market_symbol, exchange_accounts& acnts );

      const exchange_state& initial_state()const;
      void margin_call( extended_symbol debt_type );
      void lend( account_name lender, const extended_asset& debt );
      void unlend( account_name lender, double ishares, const extended_symbol& sym );
      void update_margin( account_name borrower, const extended_asset& delta_debt,
                                                 const extended_asset& delta_collateral );
      void cover_margin( account_name borrower, const extended_asset& cover_amount );

      void save();

      symbol_name      marketid;
      exchange_state   exstate;

      markets market_table;
      margins base_margins;
      margins quote_margins;
      loans   base_loans;
      loans   quote_loans;

      private:
         exchange_accounts&        _accounts;
         markets::const_iterator   market_state_itr;

         void cover_margin( account_name borrower, margins& m, exchange_state::connector& c,
                             const extended_asset& cover_amount );
         void adjust_margin( account_name borrower, margins& m, exchange_state::connector& c,
                             const extended_asset& delta_debt, const extended_asset& delta_col );
         void adjust_lend_shares( account_name lender, loans& l, double delta );
         void margin_call( exchange_state::connector& c, margins& m );
   };

}  /// namespace eosio
