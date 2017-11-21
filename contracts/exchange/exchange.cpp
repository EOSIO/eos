/**
 *  @file exchange.cpp
 *  @copyright defined in eos/LICENSE.txt
 *  @brief defines an example exchange contract
 *
 *  This exchange contract assumes the existence of two currency contracts
 *  located at @currencya and @currencyb.  These currency contracts have
 *  provided an API header defined in currency.hpp which the exchange
 *  contract will use to process messages related to deposits and withdraws.
 *
 *  The exchange contract knows that the currency contracts require_notice()
 *  of both the sender and receiver; therefore, the exchange contract can
 *  implement a message handler that will be called anytime funds are deposited
 *  to or withdrawn from the exchange.
 *
 *  When tokens are sent to @exchange from another account the exchange will
 *  credit the user's balance of the proper currency. 
 *
 *  To withdraw from the exchange, the user simply reverses the "to" and "from"
 *  fields of the currency contract transfer message. The currency contract will
 *  require the "authority" of the exchange, but the exchange's init() function
 *  configured this permission to allow *anyone* to transfer from the exchange.
 *
 *  To prevent people from stealing all the money from the exchange, the
 *  exchange's transfer handler  requires both the authority of the receiver and
 *  asserts that the user has a sufficient balance on the exchange. Lacking
 *  both of these the exchange will kill the transfer.
 *
 *  The exchange and one of the currency contracts are forced to execute in the same
 *  thread anytime there is a deposit or withdraw. The transaction containing
 *  the transfer are already required to include the exchange in the scope by
 *  the currency contract.
 *
 *  creating, canceling, and filling orders do not require blocking either currency
 *  contract.  Users can only deposit or withdraw to their own currency account.
 */
#include <exchange/exchange.hpp> /// defines transfer struct
#include <eoslib/print.hpp>

using namespace exchange;
using namespace eosio;

namespace exchange {
inline void save( const account& a ) {
   if( a.is_empty() ) {
      print("remove");
      accounts::remove(a);
   }
   else {
      print("store");
      accounts::store(a);
   }
}

template<typename Lambda>
inline void modify_account( account_name a, Lambda&& modify ) {
   auto acnt = get_account( a );
   modify( acnt );
   save( acnt );
}

/**
 *  This method is called after the "transfer" action of code
 *  "currencya" is called and "exchange" is listed in the notifiers.
 */
void apply_currency_transfer( const currency::transfer& transfer ) {
   if( transfer.to == N(exchange) ) {
      modify_account( transfer.from, [&]( account& mod_account ){
         mod_account.currency_balance += transfer.quantity;
      });
   } else if( transfer.from == N(exchange) ) {
      require_auth( transfer.to ); /// require the receiver of funds (account owner) to authorize this transfer

      modify_account( transfer.to, [&]( account& mod_account ){
         mod_account.currency_balance -= transfer.quantity;
      });
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}

/**
 *  This method is called after the "transfer" action of code
 *  "currencya" is called and "exchange" is listed in the notifiers.
 */
void apply_eos_transfer( const eosio::transfer& transfer ) {
   if( transfer.to == N(exchange) ) {
      modify_account( transfer.from, [&]( account& mod_account ){
         mod_account.eos_balance += transfer.quantity;
      });
   } else if( transfer.from == N(exchange) ) {
      require_auth( transfer.to ); /// require the receiver of funds (account owner) to authorize this transfer

      modify_account( transfer.to, [&]( account& mod_account ){
         mod_account.eos_balance -= transfer.quantity;
      });
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}

void match( bid& bid_to_match, account& buyer, ask& ask_to_match, account& seller ) {
   print( "match bid: ", bid_to_match, "\nmatch ask: ", ask_to_match, "\n");

   eosio::tokens ask_eos = ask_to_match.quantity * ask_to_match.at_price;

   eos_tokens      fill_amount_eos = min<eosio::tokens>( ask_eos, bid_to_match.quantity );
   currency_tokens fill_amount_currency;

   if( fill_amount_eos == ask_eos ) { /// complete fill of ask_to_match
      fill_amount_currency = ask_to_match.quantity;
   } else { /// complete fill of buy
      fill_amount_currency = fill_amount_eos / ask_to_match.at_price;
   }

   print( "\n\nmatch bid: ", name(bid_to_match.buyer.name), ":", bid_to_match.buyer.number,
          "match ask: ", name(ask_to_match.seller.name), ":", ask_to_match.seller.number, "\n\n" );


   bid_to_match.quantity -= fill_amount_eos;
   seller.eos_balance += fill_amount_eos;

   ask_to_match.quantity -= fill_amount_currency;
   buyer.currency_balance += fill_amount_currency;
}

/**
 * 
 *  
 */
void apply_exchange_buy( buy_order order ) {
   bid& exchange_bid = order;
   require_auth( exchange_bid.buyer.name );

   assert( exchange_bid.quantity > eosio::tokens(0), "invalid quantity" );
   assert( exchange_bid.expiration > now(), "order expired" );

   print( name(exchange_bid.buyer.name), " created bid for ", order.quantity, " currency at price: ", order.at_price, "\n" );

   bid existing_bid;
   assert( !bids_by_id::get( exchange_bid.buyer, existing_bid ), "order with this id already exists" );
   print( __FILE__, __LINE__, "\n" );

   auto buyer_account = get_account( exchange_bid.buyer.name );
   buyer_account.eos_balance -= exchange_bid.quantity;

   ask lowest_ask;
   if( !asks_by_price::front( lowest_ask ) ) {
      print( "\n No asks found, saving buyer account and storing bid\n" );
      assert( !order.fill_or_kill, "order not completely filled" );
      bids::store( exchange_bid );
      buyer_account.open_orders++;
      save( buyer_account );
      return;
   }

   print( "ask: ", lowest_ask, "\n" );
   print( "bid: ", exchange_bid, "\n" );

   auto seller_account = get_account( lowest_ask.seller.name );

   while( lowest_ask.at_price <= exchange_bid.at_price ) {
      print( "lowest ask <= exchange_bid.at_price\n" );
      match( exchange_bid, buyer_account, lowest_ask, seller_account );

      if( lowest_ask.quantity == currency_tokens(0) ) {
         seller_account.open_orders--;
         save( seller_account );
         save( buyer_account );
         asks::remove( lowest_ask );
         if( !asks_by_price::front( lowest_ask ) ) {
            break;
         }
         seller_account = get_account( lowest_ask.seller.name );
      } else {
         break; // buyer's bid should be filled
      }
   }
   print( "lowest_ask >= exchange_bid.at_price or buyer's bid has been filled\n" );

   if( exchange_bid.quantity && !order.fill_or_kill ) buyer_account.open_orders++;
   save( buyer_account );
   print( "saving buyer's account\n" );
   if( exchange_bid.quantity ) {
      print( exchange_bid.quantity, " eos left over" );
      assert( !order.fill_or_kill, "order not completely filled" );
      bids::store( exchange_bid );
      return;
   }
   print( "bid filled\n" );

}

void apply_exchange_sell( sell_order order ) {
   ask& exchange_ask = order;
   require_auth( exchange_ask.seller.name );

   assert( exchange_ask.quantity > currency_tokens(0), "invalid quantity" );
   assert( exchange_ask.expiration > now(), "order expired" );

   print( "\n\n", name(exchange_ask.seller.name), " created sell for ", order.quantity,
          " currency at price: ", order.at_price, "\n");

   ask existing_ask;
   assert( !asks_by_id::get( exchange_ask.seller, existing_ask ), "order with this id already exists" );

   auto seller_account = get_account( exchange_ask.seller.name );
   seller_account.currency_balance -= exchange_ask.quantity;


   bid highest_bid;
   if( !bids_by_price::back( highest_bid ) ) {
      assert( !order.fill_or_kill, "order not completely filled" );
      print( "\n No bids found, saving seller account and storing ask\n" );
      asks::store( exchange_ask );
      seller_account.open_orders++;
      save( seller_account );
      return;
   }

   print( "\n bids found, lets see what matches\n" );
   auto buyer_account = get_account( highest_bid.buyer.name );

   while( highest_bid.at_price >= exchange_ask.at_price ) {
      match( highest_bid, buyer_account, exchange_ask, seller_account );

      if( highest_bid.quantity == eos_tokens(0) ) {
         buyer_account.open_orders--;
         save( seller_account );
         save( buyer_account );
         bids::remove( highest_bid );
         if( !bids_by_price::back( highest_bid ) ) {
            break;
         }
         buyer_account = get_account( highest_bid.buyer.name );
      } else {
         break; // buyer's bid should be filled
      }
   }
   
   if( exchange_ask.quantity && !order.fill_or_kill ) seller_account.open_orders++;
   save( seller_account );
   if( exchange_ask.quantity ) {
      assert( !order.fill_or_kill, "order not completely filled" );
      print( "saving ask\n" );
      asks::store( exchange_ask );
      return;
   }

   print( "ask filled\n" );
}

void apply_exchange_cancel_buy( order_id order ) {
   require_auth( order.name );

   bid bid_to_cancel;
   assert( bids_by_id::get( order, bid_to_cancel ), "bid with this id does not exists" );
   
   auto buyer_account = get_account( order.name );
   buyer_account.eos_balance += bid_to_cancel.quantity;
   buyer_account.open_orders--;

   bids::remove( bid_to_cancel );
   save( buyer_account );

   print( "bid removed\n" );
}

void apply_exchange_cancel_sell( order_id order ) {
   require_auth( order.name );

   ask ask_to_cancel;
   assert( asks_by_id::get( order, ask_to_cancel ), "ask with this id does not exists" );

   auto seller_account = get_account( order.name );
   seller_account.currency_balance += ask_to_cancel.quantity;
   seller_account.open_orders--;

   asks::remove( ask_to_cancel );
   save( seller_account );

   print( "ask removed\n" );
}

} // namespace exchange

extern "C" {
   void init() {
      /*
      setAuthority( "currencya", "transfer", "anyone" );
      setAuthority( "currencyb", "transfer", "anyone" );
      registerHandler( "apply", "currencya", "transfer" );
      registerHandler( "apply", "currencyb", "transfer" );
      */
   }

//   void validate( uint64_t code, uint64_t action ) { }
//   void precondition( uint64_t code, uint64_t action ) { }
   /**
    *  The apply method implements the dispatch of events to this contract
    */
   void apply( uint64_t code, uint64_t action ) {
      if( code == N(exchange) ) {
         switch( action ) {
            case N(buy):
               apply_exchange_buy( current_action<exchange::buy_order>() );
               break;
            case N(sell):
               apply_exchange_sell( current_action<exchange::sell_order>() );
               break;
            case N(cancelbuy):
               apply_exchange_cancel_buy( current_action<exchange::order_id>() );
               break;
            case N(cancelsell):
               apply_exchange_cancel_sell( current_action<exchange::order_id>() );
               break;
            default:
               assert( false, "unknown action" );
         }
      } 
      else if( code == N(currency) ) {
        if( action == N(transfer) ) 
           apply_currency_transfer( current_action<currency::transfer>() );
      } 
      else if( code == N(eos) ) {
        if( action == N(transfer) ) 
           apply_eos_transfer( current_action<eosio::transfer>() );
      } 
      else {
      }
   }
}
