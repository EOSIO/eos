#include <exchange/exchange_state.hpp>

namespace eosio {
   extended_asset exchange_state::convert_to_exchange( connector& c, extended_asset in, bool allow_zero, bool charge_fee ) {

      real_type R(supply.amount);
      real_type C(c.balance.amount+in.amount);
      real_type F(c.weight/1000.0);
      real_type T(in.amount);
      real_type ONE(1.0);

      real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
      int64_t issued;
      if( charge_fee )
         issued = int64_t(E) - int64_t(E * fee + 1);
      else
         issued = int64_t(E);
      if( allow_zero && issued <= 0)
         issued = 0;
      else
         eosio_assert( issued > 0, "conversion is not positive after fee" );

      supply.amount += issued;
      c.balance.amount += in.amount;

      return extended_asset( issued, supply.get_extended_symbol() );
   }

   extended_asset exchange_state::convert_from_exchange( connector& c, extended_asset in, bool allow_zero, bool charge_fee ) {
      eosio_assert( in.contract == supply.contract, "unexpected asset contract input" );
      eosio_assert( in.symbol== supply.symbol, "unexpected asset symbol input" );

      real_type R(supply.amount - in.amount);
      real_type C(c.balance.amount);
      real_type F(1000.0/c.weight);
      real_type E(in.amount);
      real_type ONE(1.0);


      real_type T = C * (std::pow( ONE + E/R, F) - ONE);
      int64_t out;
      if (charge_fee)
         out = int64_t(T) - int64_t(T * fee + 1);
      else
         out = int64_t(T);
      if( allow_zero && out <= 0)
         out = 0;
      else
         eosio_assert( out > 0, "conversion is not positive after fee" );

      supply.amount -= in.amount;
      c.balance.amount -= out;

      return extended_asset( out, c.balance.get_extended_symbol() );
   }

   extended_asset exchange_state::convert( extended_asset from, extended_symbol to, bool allow_zero, bool charge_fee ) {
      auto sell_symbol  = from.get_extended_symbol();
      auto ex_symbol    = supply.get_extended_symbol();
      auto base_symbol  = base.balance.get_extended_symbol();
      auto quote_symbol = quote.balance.get_extended_symbol();

      if( sell_symbol != ex_symbol ) {
         if( sell_symbol == base_symbol ) {
            from = convert_to_exchange( base, from, allow_zero, charge_fee );
         } else if( sell_symbol == quote_symbol ) {
            from = convert_to_exchange( quote, from, allow_zero, charge_fee );
         } else { 
            eosio_assert( false, "invalid sell" );
         }
      } else {
         if( to == base_symbol ) {
            from = convert_from_exchange( base, from, allow_zero, charge_fee ); 
         } else if( to == quote_symbol ) {
            from = convert_from_exchange( quote, from, allow_zero, charge_fee ); 
         } else {
            eosio_assert( false, "invalid conversion" );
         }
      }

      if( to != from.get_extended_symbol() )
         return convert( from, to, allow_zero, charge_fee );

      return from;
   }

   bool exchange_state::requires_margin_call( const exchange_state::connector& con, const extended_symbol& collateral_symbol )const {
      if( con.peer_margin.total_lent.amount > 0  ) {
         auto tmp = *this;
         auto base_total_col = int64_t(con.peer_margin.total_lent.amount * con.peer_margin.least_collateralized);
         auto covered = tmp.convert( extended_asset( base_total_col, collateral_symbol ), con.peer_margin.total_lent.get_extended_symbol() );
         if( covered.amount <= con.peer_margin.total_lent.amount * 1.01 ) 
           return true;
      }
      return false;
   }

   bool exchange_state::requires_margin_call()const {
      return requires_margin_call( base, quote.balance.get_extended_symbol() ) || 
             requires_margin_call( quote, base.balance.get_extended_symbol() );
   }


} /// namespace eosio
