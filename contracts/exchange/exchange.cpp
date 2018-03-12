#include <math.h>
#include "exchange.hpp"

#include "exchange_state.cpp"
#include "exchange_accounts.cpp"
#include "market_state.cpp"

namespace eosio {


   void exchange::on( const deposit& d ) {
      currency::inline_transfer( d.from, _this_contract, d.quantity, "deposit" );
      _accounts.adjust_balance( d.from, d.quantity, "deposit" );
   }

   void exchange::on( const withdraw& w ) {
      require_auth( w.from );
      eosio_assert( w.quantity.amount >= 0, "cannot withdraw negative balance" );
      _accounts.adjust_balance( w.from, -w.quantity );
      currency::inline_transfer( _this_contract, w.from, w.quantity, "withdraw" );
   }

   void exchange::on( const trade& t ) {
      require_auth( t.seller );

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


   void exchange::on( const upmargin& b ) {
      require_auth( b.borrower );
      auto marketid = b.market.name();

      margins base_margins( _this_contract, marketid );
      margins quote_margins( _this_contract, marketid << 1 );

      markets market_table( _this_contract, marketid );
      auto market_state = market_table.find( marketid );
      eosio_assert( market_state != market_table.end(), "unknown market" );

      auto state = *market_state;

      eosio_assert( b.delta_borrow.amount != 0 || b.delta_collateral.amount != 0, "no effect" );
      eosio_assert( b.delta_borrow.get_extended_symbol() != b.delta_collateral.get_extended_symbol(), "invalid args" );
      eosio_assert( state.base.balance.get_extended_symbol() == b.delta_borrow.get_extended_symbol() ||
                    state.quote.balance.get_extended_symbol() == b.delta_borrow.get_extended_symbol(), 
                    "invalid asset for market" );
      eosio_assert( state.base.balance.get_extended_symbol() == b.delta_collateral.get_extended_symbol() ||
                    state.quote.balance.get_extended_symbol() == b.delta_collateral.get_extended_symbol(), 
                    "invalid asset for market" );


     auto adjust_margin = [&b](  exchange_state::connector& c, margins& mtable ) {
        margin_position temp_pos;
        auto existing = mtable.find( b.borrower );
        if( existing == mtable.end() ) {
           eosio_assert( b.delta_borrow.amount > 0, "borrow neg" );
           eosio_assert( b.delta_collateral.amount > 0, "collat neg" );
           temp_pos.owner      = b.borrower;
           temp_pos.borrowed   = b.delta_borrow;
           temp_pos.collateral = b.delta_collateral;
           temp_pos.call_price = double( temp_pos.collateral.amount ) / temp_pos.borrowed.amount;
        } else {
           temp_pos = *existing;
           temp_pos.borrowed   += b.delta_borrow;
           temp_pos.collateral += b.delta_borrow;
           eosio_assert( temp_pos.borrowed.amount > 0, "neg borrowed" );
           eosio_assert( temp_pos.collateral.amount > 0, "neg collateral" );
           if( temp_pos.borrowed.amount > 0 )
              temp_pos.call_price  = double( temp_pos.collateral.amount ) / temp_pos.borrowed.amount;
           else
              temp_pos.call_price  = double( uint64_t(-1) );
        }
        c.peer_margin.total_lent += b.delta_borrow;

        auto least = mtable.begin();
        if( least == existing ) ++least;

        if( least != mtable.end() )
           c.peer_margin.least_collateralized = least->call_price;
        if( temp_pos.call_price < c.peer_margin.least_collateralized ) 
           c.peer_margin.least_collateralized = temp_pos.call_price;
     };

     
     auto temp_state = state;
      
     margins* mt = nullptr;
     auto baseptr = &exchange_state::base;
     auto quoteptr = &exchange_state::quote;
     auto conptr   = quoteptr;

     if( b.delta_borrow.get_extended_symbol() == state.base.balance.get_extended_symbol() ) {
        mt = &base_margins;
        conptr = baseptr;
     } else {
        mt = &quote_margins;
     }

     adjust_margin( temp_state.*conptr, *mt );
     while( temp_state.requires_margin_call() ) {
     //   margin_call( state );
        temp_state = state;
        adjust_margin( temp_state.*conptr, *mt );
     }
     adjust_margin( state.*conptr, *mt );

     /// if this succeeds then the borrower will see their balances adjusted accordingly,
     /// if they don't have sufficient balance to either fund the collateral or pay off the
     /// debt then this will fail before we go further.  
     _accounts.adjust_balance( b.borrower, b.delta_borrow, "borrowed" );
     _accounts.adjust_balance( b.borrower, -b.delta_collateral, "collateral" );
   }

   void exchange::on( const createx& c ) {
      require_auth( c.creator );
      eosio_assert( c.initial_supply.symbol.is_valid(), "invalid symbol name" );
      eosio_assert( c.initial_supply.amount > 0, "initial supply must be positive" );
      eosio_assert( c.base_deposit.get_extended_symbol() != c.quote_deposit.get_extended_symbol(), 
                    "must exchange between two different currencies" );

      print( "base: ", c.base_deposit.get_extended_symbol() );
      print( "quote: ",c.quote_deposit.get_extended_symbol() );

      auto exchange_symbol = c.initial_supply.symbol.name();
      print( "marketid: ", exchange_symbol, " \n " );

      markets exstates( _this_contract, exchange_symbol );
      auto existing = exstates.find( exchange_symbol );

      eosio_assert( existing == exstates.end(), "unknown market" );
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
         eosio_assert( t.memo == "deposit", "received tokens without deposit in memo" );
         _accounts.adjust_balance( t.from, extended_asset(t.quantity,code), "deposit" );
      }
   }


   bool exchange::apply( account_name contract, account_name act ) {

      if( act == N(transfer) ) {
         on( unpack_action<currency::transfer>(), contract );   
         return true;
      }

      if( contract != _this_contract ) 
         return false;

      switch( act ) {
         case N(createx):
            on( unpack_action<createx>() );
            return true;
         case N(trade):
            on( unpack_action<trade>() );
            return true;
         case N(lend):
            on( unpack_action<lend>() );
            return true;
         case N(unlend):
            on( unpack_action<unlend>() );
            return true;
         case N(deposit):
            on( unpack_action<deposit>() );
            return true;
         case N(withdraw):
            on( unpack_action<withdraw>() );
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
