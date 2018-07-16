#include <exchange/market_state.hpp>
#include <boost/math/special_functions/relative_difference.hpp>

static const uint32_t sec_per_year = 60 * 60 * 24 * 365;

namespace eosio {

   market_state::market_state( account_name this_contract, symbol_type market_symbol, exchange_accounts& acnts, currency& excur  )
   :marketid( market_symbol.name() ),
    market_table( this_contract, marketid ),
    base_margins( this_contract,  (marketid<<4) + 1),
    quote_margins( this_contract, (marketid<<4) + 2),
    base_loans( this_contract,    (marketid<<4) + 1),
    quote_loans( this_contract,   (marketid<<4) + 2),
    _accounts(acnts),
    _currencies(excur),
    market_state_itr( market_table.find(marketid) ),
    _this_contract(this_contract)
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
      // todo: interest
      auto price_idx = marginstable.get_index<N(callprice)>();
      auto pos = price_idx.begin();
      if( pos == price_idx.end() )
         return;

      auto receipt = exstate.convert( pos->collateral, pos->borrowed.get_extended_symbol() );
      eosio_assert( receipt.amount >= pos->borrowed.amount, "programmer error: insufficient collateral to cover" );/// VERY BAD, SHOULD NOT HAPPEN
      auto change_debt = receipt - pos->borrowed;

      auto change_collat = exstate.convert( change_debt, pos->collateral.get_extended_symbol() );

      _accounts.adjust_balance( pos->owner, change_collat, "margin_call" );

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

   extended_asset market_state::get_interest( extended_asset value, uint32_t elapsed_time ) {
      value.amount = int64_t(value.amount * (exp(exstate.interest_rate * elapsed_time / sec_per_year) - 1));
      return value;
   }

   void market_state::reserve_interest( margin_position& margin, uint32_t at_time ) {
      if( margin.interest_reserve.amount )
         margin.collateral += margin.interest_reserve;
      margin.interest_reserve = get_interest( margin.collateral, sec_per_year );
      margin.collateral -= margin.interest_reserve;
      margin.interest_start_time = time_point_sec{at_time};
   }

   void market_state::charge_interest( exchange_state::connector& c, margin_position& margin, uint32_t at_time ) {
      auto interest = std::min(get_interest(margin.collateral + margin.interest_reserve, at_time - margin.interest_start_time.utc_seconds), margin.interest_reserve);
      c.peer_margin.collected_interest += interest;
      margin.collateral += margin.interest_reserve - interest;
      margin.interest_reserve.amount = 0;
   }

   void market_state::charge_yearly_interest( exchange_state::connector& c, margins& marginstable ) {
      uint32_t now = ::now();
      auto time_idx = marginstable.get_index<N(interesttime)>();
      while( true ) {
         auto it = time_idx.begin();
         if( it == time_idx.end() || now < it->interest_start_time.utc_seconds + sec_per_year )
            break;
         time_idx.modify( it, 0, [&](auto& margin){
            charge_interest( c, margin, margin.interest_start_time.utc_seconds + sec_per_year );
            reserve_interest( margin, margin.interest_start_time.utc_seconds + sec_per_year );
         });
      }
   }

   void market_state::market_order( account_name seller, extended_asset sell, extended_symbol receive_symbol ) {
      eosio_assert( sell.is_valid(), "invalid sell amount" );
      eosio_assert( sell.amount > 0, "sell amount must be positive" );
      eosio_assert( sell.get_extended_symbol() != receive_symbol, "invalid conversion" );

      _accounts.adjust_balance( seller, -sell, "sold" );

      exchange_state::connector* down; // value is going down
      exchange_state::connector* up;   // value is going up
      margins* up_margins;

      if( sell.get_extended_symbol() == exstate.base.balance.get_extended_symbol() || receive_symbol == exstate.quote.balance.get_extended_symbol() ) {
         down = &exstate.base;
         up = &exstate.quote;
         up_margins = &quote_margins;
      } else {
         down = &exstate.quote;
         up = &exstate.base;
         up_margins = &base_margins;
      }

      charge_yearly_interest( *up, *up_margins );

      auto temp = this->exstate;
      auto output = temp.convert( sell, receive_symbol );

      while( temp.requires_margin_call(*up, down->balance.get_extended_symbol()) ) {
         this->margin_call( *up, *up_margins );
         temp = this->exstate;
         output = temp.convert( sell, receive_symbol );
      }
      this->exstate = temp;

      print( name{seller}, "   ", sell, "  =>  ", output, "\n" );

      _accounts.adjust_balance( seller, output, "received" );

      if( this->exstate.supply.amount != this->initial_state().supply.amount ) {
         auto delta = this->exstate.supply - this->initial_state().supply;

         _currencies.issue_currency( { .to = _this_contract,
                                        .quantity = delta,
                                        .memo = string("") } );
      }
   }

   void market_state::lend( account_name lender, const extended_asset& quantity ) {
      auto sym = quantity.get_extended_symbol();
      _accounts.adjust_balance( lender, -quantity, "lend" );

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
         _accounts.adjust_balance( lender, unlent, "unlend" );
      }
      else if( sym == exstate.quote.balance.get_extended_symbol() ) {
         extended_asset unlent  = exstate.quote.peer_margin.unlend( ishares );
         _accounts.adjust_balance( lender, unlent, "unlend" );
      }
      else eosio_assert( false, "unable to lend to this market" );
   }



   void market_state::adjust_lend_shares( account_name lender, loans& l, double delta ) {
      // todo: charge_yearly_interest
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

   void market_state::cover_margin( account_name borrower, const extended_asset& cover_amount ) {
      if( cover_amount.get_extended_symbol() == exstate.base.balance.get_extended_symbol() ) {
         cover_margin( borrower, base_margins, exstate.base, cover_amount );
      } else if( cover_amount.get_extended_symbol() == exstate.quote.balance.get_extended_symbol() ) {
         cover_margin( borrower, quote_margins, exstate.quote, cover_amount );
      } else {
         eosio_assert( false, "invalid debt asset" );
      }
   }


   /**
    *  This method will use the collateral to buy the borrowed asset from the market
    *  with collateral to cancel the debt.
    */
   void market_state::cover_margin( account_name borrower, margins& m, exchange_state::connector& c,
                                    const extended_asset& cover_amount )
   {
      // todo: interest
      auto existing = m.find( borrower );
      eosio_assert( existing != m.end(), "no known margin position" );
      eosio_assert( existing->borrowed.amount >= cover_amount.amount, "attempt to cover more than user has" );

      auto tmp = exstate;
      auto estcol  = tmp.convert( cover_amount, existing->collateral.get_extended_symbol() );
      auto debpaid = exstate.convert( estcol, cover_amount.get_extended_symbol() );
      eosio_assert( debpaid.amount >= cover_amount.amount, "unable to cover debt" );

      auto refundcover = debpaid - cover_amount;

      auto refundcol = exstate.convert( refundcover, existing->collateral.get_extended_symbol() );
      estcol.amount -= refundcol.amount;

      if( existing->borrowed.amount == cover_amount.amount ) {
         auto freedcollateral = existing->collateral - estcol;
         m.erase( existing );
         existing = m.begin();
         _accounts.adjust_balance( borrower, freedcollateral, "cover_margin" );
      }
      else {
         m.modify( existing, 0, [&]( auto& obj ) {
             obj.collateral.amount -= estcol.amount;
             obj.borrowed.amount -= cover_amount.amount;
             obj.call_price = double(obj.collateral.amount) / obj.borrowed.amount;
         });
      }
      c.peer_margin.total_lent.amount -= cover_amount.amount;

      if( existing != m.end() ) {
         if( existing->call_price < c.peer_margin.least_collateralized )
            c.peer_margin.least_collateralized = existing->call_price;
      } else {
         c.peer_margin.least_collateralized = std::numeric_limits<double>::max();
      }
   }

   void market_state::update_margin( account_name borrower, const extended_asset& delta_debt, extended_asset& delta_col )
   {
      if( delta_debt.get_extended_symbol() == exstate.base.balance.get_extended_symbol() ) {
         adjust_margin( borrower, base_margins, exstate.base, delta_debt, delta_col );
      } else if( delta_debt.get_extended_symbol() == exstate.quote.balance.get_extended_symbol() ) {
         adjust_margin( borrower, quote_margins, exstate.quote, delta_debt, delta_col );
      } else {
         eosio_assert( false, "invalid debt asset" );
      }
   }

   void market_state::adjust_margin( account_name borrower, margins& m, exchange_state::connector& c,
                                     const extended_asset& delta_debt, extended_asset& delta_col )
   {
      auto now = ::now();
      auto existing = m.find( borrower );
      if( existing == m.end() ) {
         eosio_assert( delta_debt.amount > 0, "cannot borrow neg" );
         eosio_assert( delta_col.amount > 0, "cannot have neg collat" );

         existing = m.emplace( borrower, [&]( auto& obj ) {
            obj.owner      = borrower;
            obj.borrowed   = delta_debt;
            obj.collateral = delta_col;
            reserve_interest( obj, now );            
            obj.call_price = double(obj.collateral.amount) / obj.borrowed.amount;
         });
      } else {
         if( existing->borrowed.amount == -delta_debt.amount ) {
            auto obj = *existing;
            charge_interest( c, obj, now );
            delta_col = -obj.collateral;
            m.erase( existing );

            auto price_idx = m.get_index<N(callprice)>();
            auto least = price_idx.begin();
            if( least != price_idx.end() )
               existing = m.iterator_to( *least );
            else
               existing = m.end();
         } else {
            m.modify( existing, 0, [&]( auto& obj ) {
               charge_interest( c, obj, now );
               obj.borrowed   += delta_debt;
               obj.collateral += delta_col;
               reserve_interest( obj, now );            
               obj.call_price = double(obj.collateral.amount) / obj.borrowed.amount;
            });
         }
      }

      c.peer_margin.total_lent += delta_debt;
      eosio_assert( c.peer_margin.total_lent.amount <= c.peer_margin.total_lendable.amount, "insufficient funds available to borrow" );

      if( existing != m.end() ) {
         if( existing->call_price < c.peer_margin.least_collateralized )
            c.peer_margin.least_collateralized = existing->call_price;

         eosio_assert( !exstate.requires_margin_call( c, delta_col.get_extended_symbol() ), "this update would trigger a margin call" );
      } else {
         c.peer_margin.least_collateralized = std::numeric_limits<double>::max();
      }

   }

   void market_state::save() {
      market_table.modify( market_state_itr, 0, [&]( auto& s ) {
         s = exstate;
      });
   }

}
