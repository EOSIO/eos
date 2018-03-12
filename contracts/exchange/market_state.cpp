#include <exchange/market_state.hpp>
#include <boost/math/special_functions/relative_difference.hpp>

namespace eosio {

   market_state::market_state( account_name this_contract, symbol_type market_symbol, exchange_accounts& acnts )
   :marketid( market_symbol.name() ),
    market_table( this_contract, marketid ),
    base_margins( this_contract,  (marketid<<4) + 1),
    quote_margins( this_contract, (marketid<<4) + 2),
    base_loans( this_contract,    (marketid<<4) + 1),
    quote_loans( this_contract,   (marketid<<4) + 2),
    _accounts(acnts),
    market_state_itr( market_table.find(marketid) )
   {
      eosio_assert( market_state_itr != market_table.end(), "unknown market" );
      exstate = *market_state_itr;
   }

   void market_state::margin_call( extended_symbol debt_type ) {
      if( debt_type == exstate.base.balance.get_extended_symbol() )
         margin_call( exstate.base, base_margins );
      else
         margin_call( exstate.quote, quote_margins );
   }

   void market_state::margin_call( exchange_state::connector& c, margins& marginstable ) {
      auto price_idx = marginstable.get_index<N(callprice)>();
      auto pos = price_idx.begin();
      if( pos == price_idx.end() ) 
         return;

      auto receipt = exstate.convert( pos->collateral, pos->borrowed.get_extended_symbol() );
      eosio_assert( receipt.amount >= pos->borrowed.amount, "programmer error: insufficient collateral to cover" );/// VERY BAD, SHOULD NOT HAPPEN
      auto change_debt = receipt - pos->borrowed;

      auto change_collat = exstate.convert( change_debt, pos->collateral.get_extended_symbol() );

      _accounts.adjust_balance( pos->owner, change_collat );

      c.peer_margin.total_lent.amount -= pos->borrowed.amount;
      price_idx.erase(pos);

      pos = price_idx.begin();
      if( pos != price_idx.end() )
         c.peer_margin.least_collateralized = pos->call_price;
      else
         c.peer_margin.least_collateralized = double(uint64_t(-1));
   }


   const exchange_state& market_state::initial_state()const {
      return *market_state_itr;
   }

   void market_state::lend( account_name lender, const extended_asset& quantity ) {
      auto sym = quantity.get_extended_symbol();
      _accounts.adjust_balance( lender, -quantity );

      if( sym == exstate.base.balance.get_extended_symbol() ) {
         double new_shares = exstate.base.peer_margin.lend( quantity.amount );
         adjust_lend_shares( lender, base_loans, new_shares );
      }
      else if( sym == exstate.quote.balance.get_extended_symbol() ) {
         double new_shares = exstate.quote.peer_margin.lend( quantity.amount );
         adjust_lend_shares( lender, quote_loans, new_shares );
      }
      else eosio_assert( false, "unable to lend to this market" );
   }

   void market_state::unlend( account_name lender, double ishares, const extended_symbol& sym ) {
      eosio_assert( ishares > 0, "cannot unlend negative balance" );
      adjust_lend_shares( lender, base_loans, -ishares );

      print( "sym: ", sym );

      if( sym == exstate.base.balance.get_extended_symbol() ) {
         extended_asset unlent  = exstate.base.peer_margin.unlend( ishares );
         _accounts.adjust_balance( lender, unlent );
      }
      else if( sym == exstate.quote.balance.get_extended_symbol() ) {
         extended_asset unlent  = exstate.quote.peer_margin.unlend( ishares );
         _accounts.adjust_balance( lender, unlent );
      }
      else eosio_assert( false, "unable to lend to this market" );
   }



   void market_state::adjust_lend_shares( account_name lender, loans& l, double delta ) {
      auto existing = l.find( lender );
      if( existing == l.end() ) {
         l.emplace( lender, [&]( auto& obj ) {
            obj.owner = lender;
            obj.interest_shares = delta;
            eosio_assert( delta >= 0, "underflow" );
         });
      } else {
         l.modify( existing, 0, [&]( auto& obj ) {
            obj.interest_shares += delta;
            eosio_assert( obj.interest_shares >= 0, "underflow" );
         });
      }
   }

   void market_state::save() {
      market_table.modify( market_state_itr, 0, [&]( auto& s ) {
         s = exstate;
      });
   }

}
