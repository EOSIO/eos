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
      extended_asset   interest_reserve;
      time_point_sec   interest_start_time;
      double           call_price = 0;

      uint64_t get_call()const { return uint64_t(1000000*call_price); }
      uint64_t get_interest_time_key()const { return -interest_start_time.utc_seconds; }
      uint64_t primary_key()const { return owner; }

      EOSLIB_SERIALIZE( margin_position, (owner)(borrowed)(collateral)(interest_reserve)(interest_start_time)(call_price) )
   };

   typedef eosio::multi_index<N(margins), margin_position,
           indexed_by<N(callprice), eosio::const_mem_fun<margin_position, uint64_t, &margin_position::get_call> >,
           indexed_by<N(interesttime), eosio::const_mem_fun<margin_position, uint64_t, &margin_position::get_interest_time_key> >
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
      market_state( account_name this_contract, symbol_type market_symbol, exchange_accounts& acnts, currency& excur );

      const exchange_state& initial_state()const;

      extended_asset get_interest( extended_asset amount, uint32_t elapsed_time );
      void reserve_interest( margin_position& margin, uint32_t at_time );
      void charge_interest( exchange_state::connector& c, margin_position& margin, uint32_t at_time );
      void charge_yearly_interest( exchange_state::connector& c, margins& marginstable, int& remaining_ops );
      bool market_order( extended_asset sell, extended_symbol receive_symbol, int& remaining_ops, extended_asset& result );
      void market_order( account_name seller, extended_asset sell, extended_symbol receive );
      void margin_call( extended_symbol debt_type );
      void lend( account_name lender, const extended_asset& debt );
      void unlend( account_name lender, double ishares, const extended_symbol& sym );
      void update_margin( account_name borrower, const extended_asset& delta_debt,
                                                 extended_asset& delta_collateral );
      void cover_margin( account_name borrower, const extended_asset& cover_amount );

      void schedule_market_continuation( account_name seller, extended_asset sell, extended_symbol receive_symbol );
      void schedule_continuation();
      void continuation( int max_ops );

      void save();

      symbol_type      market_symbol;
      symbol_name      marketid;
      exchange_state   exstate;

      markets market_table;
      margins base_margins;
      margins quote_margins;
      loans   base_loans;
      loans   quote_loans;
      /// limits and stoploss 

      private:
         exchange_accounts&        _accounts;
         currency&                 _currencies;
         markets::const_iterator   market_state_itr;
         account_name              _this_contract;

         void cover_margin( account_name borrower, margins& m, exchange_state::connector& c,
                             const extended_asset& cover_amount );
         void adjust_margin( account_name borrower, margins& m, exchange_state::connector& c,
                             const extended_asset& delta_debt, extended_asset& delta_col );
         void adjust_lend_shares( account_name lender, loans& l, double delta );
         void margin_call( exchange_state::connector& c, margins& m );
   };

}  /// namespace eosio
