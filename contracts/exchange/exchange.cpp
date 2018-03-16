#include <math.h>
#include "exchange.hpp"

#include "exchange_state.cpp"
#include "exchange_accounts.cpp"
#include "market_state.cpp"

namespace eosio {


   void exchange::on( const deposit& d ) {
      eosio_assert( d.quantity.is_valid(), "invalid quantity" );
      currency::inline_transfer( d.from, _this_contract, d.quantity, "deposit" );
      _accounts.adjust_balance( d.from, d.quantity, "deposit" );
   }

   void exchange::on( const withdraw& w ) {
      require_auth( w.from );
      eosio_assert( w.quantity.is_valid(), "invalid quantity" );
      eosio_assert( w.quantity.amount >= 0, "cannot withdraw negative balance" ); // Redundant? inline_transfer will fail if w.quantity is not positive.
      _accounts.adjust_balance( w.from, -w.quantity );
      currency::inline_transfer( _this_contract, w.from, w.quantity, "withdraw" );
   }

   void exchange::on( const trade& t ) {
      require_auth( t.seller );
      eosio_assert( t.sell.is_valid(), "invalid sell amount" );
      eosio_assert( t.sell.amount > 0, "sell amount must be positive" );
      eosio_assert( t.min_receive.is_valid(), "invalid min receive amount" );
      eosio_assert( t.min_receive.amount >= 0, "min receive amount cannot be negative" );

      auto receive_symbol = t.min_receive.get_extended_symbol();
      eosio_assert( t.sell.get_extended_symbol() != receive_symbol, "invalid conversion" );

      market_state market( _this_contract, t.market, _accounts );

      auto temp   = market.exstate;
      auto output = temp.convert( t.sell, receive_symbol );

      while( temp.requires_margin_call() ) {
         market.margin_call( receive_symbol );
         temp = market.exstate;
         output = temp.convert( t.sell, receive_symbol );
      }
      market.exstate = temp;

      print( name(t.seller), "   ", t.sell, "  =>  ", output, "\n" );

      if( t.min_receive.amount != 0 ) {
         eosio_assert( t.min_receive.amount <= output.amount, "unable to fill" );
      }

      _accounts.adjust_balance( t.seller, -t.sell, "sold" );
      _accounts.adjust_balance( t.seller, output, "received" );

      if( market.exstate.supply.amount != market.initial_state().supply.amount ) {
         auto delta = market.exstate.supply - market.initial_state().supply;

         _excurrencies.issue_currency( { .to = _this_contract,
                                        .quantity = delta,
                                        .memo = string("") } );
      }

      /// TODO: if pending order start deferred trx to fill it

      market.save();
   }


   /**
    *  This action shall fail if it would result in a margin call
    */
   void exchange::on( const upmargin& b ) {
      require_auth( b.borrower );
      eosio_assert( b.delta_borrow.is_valid(), "invalid borrow delta" );
      eosio_assert( b.delta_collateral.is_valid(), "invalid collateral delta" );

      market_state market( _this_contract, b.market, _accounts );

      eosio_assert( b.delta_borrow.amount != 0 || b.delta_collateral.amount != 0, "no effect" );
      eosio_assert( b.delta_borrow.get_extended_symbol() != b.delta_collateral.get_extended_symbol(), "invalid args" );
      eosio_assert( market.exstate.base.balance.get_extended_symbol() == b.delta_borrow.get_extended_symbol() ||
                    market.exstate.quote.balance.get_extended_symbol() == b.delta_borrow.get_extended_symbol(),
                    "invalid asset for market" );
      eosio_assert( market.exstate.base.balance.get_extended_symbol() == b.delta_collateral.get_extended_symbol() ||
                    market.exstate.quote.balance.get_extended_symbol() == b.delta_collateral.get_extended_symbol(),
                    "invalid asset for market" );

      market.update_margin( b.borrower, b.delta_borrow, b.delta_collateral );

     /// if this succeeds then the borrower will see their balances adjusted accordingly,
     /// if they don't have sufficient balance to either fund the collateral or pay off the
     /// debt then this will fail before we go further.
     _accounts.adjust_balance( b.borrower, b.delta_borrow, "borrowed" );
     _accounts.adjust_balance( b.borrower, -b.delta_collateral, "collateral" );

     market.save();
   }

   void exchange::on( const covermargin& c ) {
      require_auth( c.borrower );
      eosio_assert( c.cover_amount.is_valid(), "invalid cover amount" );
      eosio_assert( c.cover_amount.amount > 0, "cover amount must be positive" );

      market_state market( _this_contract, c.market, _accounts );

      market.cover_margin( c.borrower, c.cover_amount);

      market.save();
   }

   void exchange::on( const createx& c ) {
      require_auth( c.creator );
      eosio_assert( c.initial_supply.is_valid(), "invalid initial supply" );
      eosio_assert( c.initial_supply.amount > 0, "initial supply must be positive" );
      eosio_assert( c.base_deposit.is_valid(), "invalid base deposit" );
      eosio_assert( c.base_deposit.amount > 0, "base deposit must be positive" );
      eosio_assert( c.quote_deposit.is_valid(), "invalid quote deposit" );
      eosio_assert( c.quote_deposit.amount > 0, "quote deposit must be positive" );
      eosio_assert( c.base_deposit.get_extended_symbol() != c.quote_deposit.get_extended_symbol(),
                    "must exchange between two different currencies" );

      print( "base: ", c.base_deposit.get_extended_symbol() );
      print( "quote: ",c.quote_deposit.get_extended_symbol() );

      auto exchange_symbol = c.initial_supply.symbol.name();
      print( "marketid: ", exchange_symbol, " \n " );

      markets exstates( _this_contract, exchange_symbol );
      auto existing = exstates.find( exchange_symbol );

      eosio_assert( existing == exstates.end(), "market already exists" );
      exstates.emplace( c.creator, [&]( auto& s ) {
          s.manager = c.creator;
          s.supply  = extended_asset(c.initial_supply, _this_contract);
          s.base.balance = c.base_deposit;
          s.quote.balance = c.quote_deposit;

          s.base.peer_margin.total_lent.symbol          = c.base_deposit.symbol;
          s.base.peer_margin.total_lent.contract        = c.base_deposit.contract;
          s.base.peer_margin.total_lendable.symbol      = c.base_deposit.symbol;
          s.base.peer_margin.total_lendable.contract    = c.base_deposit.contract;

          s.quote.peer_margin.total_lent.symbol         = c.quote_deposit.symbol;
          s.quote.peer_margin.total_lent.contract       = c.quote_deposit.contract;
          s.quote.peer_margin.total_lendable.symbol     = c.quote_deposit.symbol;
          s.quote.peer_margin.total_lendable.contract   = c.quote_deposit.contract;
      });

      _excurrencies.create_currency( { .issuer = _this_contract,
                                 // TODO: After currency contract respects maximum supply limits, the maximum supply here needs to be set appropriately.
                                .maximum_supply = asset( 0, c.initial_supply.symbol ),
                                .issuer_can_freeze = false,
                                .issuer_can_whitelist = false,
                                .issuer_can_recall = false } );

      _excurrencies.issue_currency( { .to = _this_contract,
                                     .quantity = c.initial_supply,
                                     .memo = string("initial exchange tokens") } );

      _accounts.adjust_balance( c.creator, extended_asset( c.initial_supply, _this_contract ), "new exchange issue" );
      _accounts.adjust_balance( c.creator, -c.base_deposit, "new exchange deposit" );
      _accounts.adjust_balance( c.creator, -c.quote_deposit, "new exchange deposit" );
   }

   void exchange::on( const lend& w ) {
      require_auth( w.lender );
      eosio_assert( w.quantity.is_valid(), "invalid quantity" );
      eosio_assert( w.quantity.amount > 0, "must lend a positive amount" );

      market_state market( _this_contract, w.market, _accounts );
      market.lend( w.lender, w.quantity );
      market.save();
   }

   void exchange::on( const unlend& w ) {
      require_auth( w.lender );
      eosio_assert( w.interest_shares > 0, "must unlend a positive amount" );

      market_state market( _this_contract, w.market, _accounts );
      market.unlend( w.lender, w.interest_shares, w.interest_symbol );
      market.save();
   }


   void exchange::on( const currency::transfer& t, account_name code ) {
      if( code == _this_contract )
         _excurrencies.on( t );

      if( t.to == _this_contract ) {
         auto a = extended_asset(t.quantity, code);
         eosio_assert( a.is_valid(), "invalid quantity in transfer" );
         eosio_assert( a.amount != 0, "zero quantity is disallowed in transfer");
         eosio_assert( a.amount > 0 || t.memo == "withdraw", "withdrew tokens without withdraw in memo");
         eosio_assert( a.amount < 0 || t.memo == "deposit", "received tokens without deposit in memo" );
         _accounts.adjust_balance( t.from, a, t.memo );
      }
   }


   bool exchange::apply( account_name contract, account_name act ) {

      if( act == N(transfer) ) {
         on( unpack_action_data<currency::transfer>(), contract );
         return true;
      }

      if( contract != _this_contract )
         return false;

      switch( act ) {
         case N(createx):
            on( unpack_action_data<createx>() );
            return true;
         case N(trade):
            on( unpack_action_data<trade>() );
            return true;
         case N(lend):
            on( unpack_action_data<lend>() );
            return true;
         case N(unlend):
            on( unpack_action_data<unlend>() );
            return true;
         case N(deposit):
            on( unpack_action_data<deposit>() );
            return true;
         case N(withdraw):
            on( unpack_action_data<withdraw>() );
            return true;
         case N(upmargin):
            on( unpack_action_data<upmargin>() );
            return true;
         case N(covermargin):
            on( unpack_action_data<covermargin>() );
            return true;
         default:
            return _excurrencies.apply( contract, act );
      }
   }

} /// namespace eosio



extern "C" {
   void apply( uint64_t code, uint64_t action ) {
       eosio::exchange( current_receiver() ).apply( code, action );
   }
}
