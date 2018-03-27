#include <math.h>
#include "exchange.hpp"

#include "exchange_state.cpp"
#include "exchange_accounts.cpp"
#include "market_state.cpp"

#include <eosiolib/dispatcher.hpp>

namespace eosio {

   void exchange::deposit( account_name from, extended_asset quantity ) {
      eosio_assert( quantity.is_valid(), "invalid quantity" );
      currency::inline_transfer( from, _this_contract, quantity, "deposit" );
      _accounts.adjust_balance( from, quantity, "deposit" );
   }

   void exchange::withdraw( account_name from, extended_asset quantity ) {
      require_auth( from );
      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount >= 0, "cannot withdraw negative balance" ); // Redundant? inline_transfer will fail if quantity is not positive.
      _accounts.adjust_balance( from, -quantity );
      currency::inline_transfer( _this_contract, from, quantity, "withdraw" );
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

   void exchange::createx( account_name    creator,
                 asset           initial_supply,
                 uint32_t        fee,
                 extended_asset  base_deposit,
                 extended_asset  quote_deposit
               ) {
      require_auth( creator );
      eosio_assert( initial_supply.is_valid(), "invalid initial supply" );
      eosio_assert( initial_supply.amount > 0, "initial supply must be positive" );
      eosio_assert( base_deposit.is_valid(), "invalid base deposit" );
      eosio_assert( base_deposit.amount > 0, "base deposit must be positive" );
      eosio_assert( quote_deposit.is_valid(), "invalid quote deposit" );
      eosio_assert( quote_deposit.amount > 0, "quote deposit must be positive" );
      eosio_assert( base_deposit.get_extended_symbol() != quote_deposit.get_extended_symbol(),
                    "must exchange between two different currencies" );

      print( "base: ", base_deposit.get_extended_symbol() );
      print( "quote: ",quote_deposit.get_extended_symbol() );

      auto exchange_symbol = initial_supply.symbol.name();
      print( "marketid: ", exchange_symbol, " \n " );

      markets exstates( _this_contract, exchange_symbol );
      auto existing = exstates.find( exchange_symbol );

      eosio_assert( existing == exstates.end(), "market already exists" );
      exstates.emplace( creator, [&]( auto& s ) {
          s.manager = creator;
          s.supply  = extended_asset(initial_supply, _this_contract);
          s.base.balance = base_deposit;
          s.quote.balance = quote_deposit;

          s.base.peer_margin.total_lent.symbol          = base_deposit.symbol;
          s.base.peer_margin.total_lent.contract        = base_deposit.contract;
          s.base.peer_margin.total_lendable.symbol      = base_deposit.symbol;
          s.base.peer_margin.total_lendable.contract    = base_deposit.contract;

          s.quote.peer_margin.total_lent.symbol         = quote_deposit.symbol;
          s.quote.peer_margin.total_lent.contract       = quote_deposit.contract;
          s.quote.peer_margin.total_lendable.symbol     = quote_deposit.symbol;
          s.quote.peer_margin.total_lendable.contract   = quote_deposit.contract;
      });

      _excurrencies.create_currency( { .issuer = _this_contract,
                                 // TODO: After currency contract respects maximum supply limits, the maximum supply here needs to be set appropriately.
                                .maximum_supply = asset( 0, initial_supply.symbol ),
                                .issuer_can_freeze = false,
                                .issuer_can_whitelist = false,
                                .issuer_can_recall = false } );

      _excurrencies.issue_currency( { .to = _this_contract,
                                     .quantity = initial_supply,
                                     .memo = string("initial exchange tokens") } );

      _accounts.adjust_balance( creator, extended_asset( initial_supply, _this_contract ), "new exchange issue" );
      _accounts.adjust_balance( creator, -base_deposit, "new exchange deposit" );
      _accounts.adjust_balance( creator, -quote_deposit, "new exchange deposit" );
   }

   void exchange::lend( account_name lender, symbol_type market, extended_asset quantity ) {
      require_auth( lender );
      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount > 0, "must lend a positive amount" );

      market_state m( _this_contract, market, _accounts );
      m.lend( lender, quantity );
      m.save();
   }

   void exchange::unlend( account_name lender, symbol_type market, double interest_shares, extended_symbol interest_symbol ) {
      require_auth( lender );
      eosio_assert( interest_shares > 0, "must unlend a positive amount" );

      market_state m( _this_contract, market, _accounts );
      m.unlend( lender, interest_shares, interest_symbol );
      m.save();
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


   #define N(X) ::eosio::string_to_name(#X)

   void exchange::apply( account_name contract, account_name act ) {

      if( act == N(transfer) ) {
         on( unpack_action_data<currency::transfer>(), contract );
         return;
      }

      if( contract != _this_contract )
         return;

      auto& thiscontract = *this;
      switch( act ) {
         EOSIO_API( exchange, (createx)(deposit)(withdraw)(lend)(unlend) )
      };

      switch( act ) {
         case N(trade):
            on( unpack_action_data<trade>() );
            return;
         case N(upmargin):
            on( unpack_action_data<upmargin>() );
            return;
         case N(covermargin):
            on( unpack_action_data<covermargin>() );
            return;
         default:
            _excurrencies.apply( contract, act ); 
            return;
      }
   }

} /// namespace eosio



extern "C" {
   void apply( uint64_t code, uint64_t action ) {
      eosio::exchange  ex( current_receiver() );
      ex.apply( code, action );
      eosio_exit(0);
   }
}

void main() {
}
