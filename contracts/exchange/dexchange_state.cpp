#include <exchange/dexchange_state.hpp>

namespace eosio {

   void dexchange_state::init( account_name   mgr,
                               double         ifee,
                               extended_asset icollateral, 
                               double         iprice, 
                               double         trratio, 
                               extended_symbol supply_sym, symbol_type peg_sym ) {
      manager                 = mgr;
      fee                     = ifee;
      supply.amount           = icollateral.amount * 10000;
      target_reserve_ratio    = trratio;
      current_target_price    = iprice;
      collateral_balance      = extended_asset( int64_t(icollateral.amount / trratio), icollateral.get_extended_symbol() );
      spare_collateral        = extended_asset( icollateral.amount - collateral_balance.amount, icollateral.get_extended_symbol() );
      pegged_balance.amount   = int64_t(collateral_balance.amount * iprice);
      pegged_balance.symbol   = peg_sym;
      pegged_balance.contract = supply.contract;
      sold_peg                = pegged_balance;
      sold_peg.amount         = 0;
   }


   /**
    *  This method moves collateral between collateral_balance and spare_collateral to bias
    *  the bancor price toward the target price any time the bancor price has been different
    *  from the target price by more than 1% for more than 24 hours.
    *
    *  The bias corrects the price using a weighted average between the current bancor price
    *  and the target bancor price such that it will take about 1 hour to sync back to within
    *  1%
    */
   void dexchange_state::sync_toward_feed( block_timestamp_type now ) {

#if 0
      double bancor_price = double(collateral_balance.amount) / pegged_balance.amount;
      double delta = std::fabs(bancor_price - current_target_price);
      double dpercent = delta / current_target_price;

      if( dpercent < 0.01 ) {
         last_price_in_range = now;
      }

      if( last_sync == now ) return;

      /// if bancor hasn't traded within 1% of 24hr median average price within the past 24 hour, then 
      /// we need to modify the collateral or pegged currency supply so that it is closer to
      /// the average... within 24 hours this should cause the price to converge to within 1%
      if( (now - last_price_in_range).slot > 120*60*24 /*blocks per day*/ ) {

         double new_bancor_price = ((bancor_price * (120*60-1)) + current_target_price) / ( 120*60 );

         /// the total value of maker collateral should always equal the value of the maker pegged 
         double collateral_expected = pegged_balance.amount / new_bancor_price;


         /// add or remove collateral from the spare collateral to make price the new bancor price
         auto delta_col = uint64_t(collateral_expected - collateral_balance.amount);

         /// if for some reason we need more collateral than we have then do nothing
         if( delta_col.amount > 0 && spare_collateral.amount < delta_col ) 
            return; /// unable to help support the price, sorry

         /// adjust spare collateral and maker collateral by delta
         spare_collateral.amount   -= delta_collateral;
         collateral_balance.amount += delta_collateral;
      }
#endif

   }

   /**
    *  This method will issue new pegged tokens if the total collateral ratio is above the
    *  target.
    */
   void dexchange_state::sync_toward_target_reserve() {

#if 0
      double total_collateral         = collateral_balance.amount + spare_collateral.amount;
      double total_debt               = sold_peg.amount + pegged_balance.amount;


      double ideal_collateral_balance = total_collateral / target_reserve_ratio;
      double ideal_total_debt         = ideal_collateral_balance * new_bancor_price;


      double target_collateral = (total_debt * target_reserve_ratio / new_bancor_price);
      
      if( collateral_balance.amount > target_collateral ) {
         double target_debt = ideal_collateral_balance * new_bancor_price;
         double new_debt    = target_debt - total_debt;
         int64_t delta_col  = int64_t( ideal_collateral_balance - collateral_balance.amount );
         collateral_balance.amount += delta_col;
         spare_collateral          -= delta_col;
         
         eosio_assert( spare_collateral.amount > 0 && collateral_balance.amount > 0, "collateral error" );
         pegged_balance.amount += new_debt;
      }
#endif 

   }

   extended_asset dexchange_state::convert_to_exchange( extended_asset& connector_balance, extended_asset in ) {

      real_type R(supply.amount);
      real_type C(connector_balance.amount+in.amount);
      real_type T(in.amount);
      const real_type F(1/2.0);
      const real_type ONE(1.0);

      real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
      int64_t issued = int64_t(E);

      supply.amount += issued;
      connector_balance.amount += in.amount;

      return extended_asset( issued, supply.get_extended_symbol() );
   }

   extended_asset dexchange_state::convert_from_exchange( extended_asset& connector_balance, extended_asset in ) {
      eosio_assert( in.contract == supply.contract, "unexpected asset contract input" );
      eosio_assert( in.symbol== supply.symbol, "unexpected asset symbol input" );

      real_type R(supply.amount - in.amount);
      real_type C(connector_balance.amount);
      real_type E(in.amount);
      const real_type F(2.0);
      const real_type ONE(1.0);


      real_type T = C * (std::pow( ONE + E/R, F) - ONE);
      int64_t out = int64_t(T);

      supply.amount -= in.amount;
      connector_balance.amount -= out;

      return extended_asset( out, connector_balance.get_extended_symbol() );
   }

   extended_asset dexchange_state::buyex( const extended_asset& colin, asset& pegout ) {
      auto collateral_supply = collateral_balance.amount + spare_collateral.amount;
      double percent = double( colin.amount + collateral_supply ) / collateral_supply;

      int64_t peg_to_maker = int64_t(pegged_balance.amount * percent - pegged_balance.amount);
      int64_t col_to_maker = int64_t(collateral_balance.amount * percent - collateral_balance.amount);
      int64_t col_to_spare = int64_t(spare_collateral.amount * percent - spare_collateral.amount);
      int64_t peg_to_sold  = int64_t(sold_peg.amount * percent - sold_peg.amount);
      int64_t new_ex       = int64_t(supply.amount * percent - supply.amount);

      pegged_balance.amount     += peg_to_maker;
      collateral_balance.amount += col_to_maker;
      spare_collateral.amount   += col_to_spare;
      supply.amount             += new_ex;
      sold_peg.amount           += peg_to_sold;

      pegout.symbol = pegged_balance.symbol;
      pegout.amount = peg_to_sold;

      return extended_asset( new_ex, supply.get_extended_symbol());
   }

   extended_asset dexchange_state::sellex( const extended_asset& exin, asset& pegin ) {
      double percent         = double(exin.amount) / supply.amount;
      int64_t peg_from_maker = int64_t(pegged_balance.amount * percent);
      int64_t peg_to_cover   = int64_t(sold_peg.amount * percent);
      int64_t col_from_maker = int64_t(collateral_balance.amount * percent);
      int64_t col_from_spare = int64_t(spare_collateral.amount * percent);

      sold_peg.amount             -= peg_to_cover;
      collateral_balance.amount   -= col_from_maker;
      pegged_balance.amount       -= peg_from_maker;
      spare_collateral.amount     -= col_from_spare;

      supply.amount -= exin.amount;

      pegin.amount = -peg_to_cover;
      pegin.symbol = pegged_balance.symbol;

      return extended_asset( col_from_maker+col_from_spare, collateral_balance.get_extended_symbol() );
   }

   extended_asset dexchange_state::convert( extended_asset from, extended_symbol to, asset& pegio ) {
      auto sell_symbol        = from.get_extended_symbol();
      auto ex_symbol          = supply.get_extended_symbol();
      auto collateral_symbol  = collateral_balance.get_extended_symbol();
      auto peg_symbol         = pegged_balance.get_extended_symbol();


      if( to == ex_symbol ) {
         eosio_assert( from.get_extended_symbol() == collateral_symbol, "can only buy ex with collateral token" );
         return buyex( from, pegio );
      } else if( sell_symbol == ex_symbol ) {
         eosio_assert( to == collateral_symbol, "can only sell ex for collateral token" );
         return sellex( from, pegio );
      }

      if( sell_symbol == collateral_symbol ) {
         auto tmp = convert_to_exchange( collateral_balance, from );
         return convert_to_exchange( pegged_balance, tmp );
      } else if ( sell_symbol == peg_symbol ) {
         auto tmp = convert_to_exchange( pegged_balance, from );
         return convert_to_exchange( collateral_balance, tmp );
      }

      eosio_assert( false, "invalid conversion" );
      eosio_exit(-1); /// signal no-return
   }



} /// namespace eosio
