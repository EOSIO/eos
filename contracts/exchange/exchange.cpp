/**
 *  @file exchange.cpp
 *  @brief defines an example exchange contract 
 *
 *  This exchange contract assumes the existence of two currency contracts
 *  located at @currencya and @currencyb.  These currency contracts have
 *  provided an API header defined in currency.hpp which the exchange
 *  contract will use to process messages related to deposits and withdraws.
 *
 *  The exchange contract knows that the currency contracts requireNotice()
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
using namespace eos;

namespace exchange {
inline void save( const Account& a ) {
   if( a.isEmpty() ) {
      print("remove");
      Accounts::remove(a);
   }
   else {
      print("store");
      Accounts::store(a);
   }
}

template<typename Lambda>
inline void modifyAccount( AccountName a, Lambda&& modify ) {
   auto acnt = getAccount( a );
   modify( acnt );
   save( acnt );
}

/**
 *  This method is called after the "transfer" action of code
 *  "currencya" is called and "exchange" is listed in the notifiers.
 */
void apply_currency_transfer( const currency::Transfer& transfer ) {
   if( transfer.to == N(exchange) ) {
      modifyAccount( transfer.from, [&]( Account& account ){ 
          account.currency_balance += transfer.quantity; 
      });
   } else if( transfer.from == N(exchange) ) {
      requireAuth( transfer.to ); /// require the receiver of funds (account owner) to authorize this transfer

      modifyAccount( transfer.to, [&]( Account& account ){ 
          account.currency_balance -= transfer.quantity; 
      });
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}

/**
 *  This method is called after the "transfer" action of code
 *  "currencya" is called and "exchange" is listed in the notifiers.
 */
void apply_eos_transfer( const eos::Transfer& transfer ) {
   if( transfer.to == N(exchange) ) {
      modifyAccount( transfer.from, [&]( Account& account ){ 
          account.eos_balance += transfer.quantity; 
      });
   } else if( transfer.from == N(exchange) ) {
      requireAuth( transfer.to ); /// require the receiver of funds (account owner) to authorize this transfer

      modifyAccount( transfer.to, [&]( Account& account ){ 
          account.eos_balance -= transfer.quantity; 
      });
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}

void match( Bid& bid, Account& buyer, Ask& ask, Account& seller ) {
   print( "match bid: ", bid, "\nmatch ask: ", ask, "\n");

   eos::Tokens ask_eos = ask.quantity * ask.price;

   EosTokens      fill_amount_eos = min<eos::Tokens>( ask_eos, bid.quantity );
   CurrencyTokens fill_amount_currency;

   if( fill_amount_eos == ask_eos ) { /// complete fill of ask
      fill_amount_currency = ask.quantity;
   } else { /// complete fill of buy
      fill_amount_currency = fill_amount_eos / ask.price;
   }

   print( "\n\nmatch bid: ", Name(bid.buyer.name), ":", bid.buyer.number,
          "match ask: ", Name(ask.seller.name), ":", ask.seller.number, "\n\n" );


   bid.quantity -= fill_amount_eos;
   seller.eos_balance += fill_amount_eos;

   ask.quantity -= fill_amount_currency;
   buyer.currency_balance += fill_amount_currency;
}

/**
 * 
 *  
 */
void apply_exchange_buy( BuyOrder order ) {
   Bid& bid = order;
   requireAuth( bid.buyer.name ); 

   assert( bid.quantity > eos::Tokens(0), "invalid quantity" );
   assert( bid.expiration > now(), "order expired" );

   print( Name(bid.buyer.name), " created bid for ", order.quantity, " currency at price: ", order.price, "\n" );

   Bid existing_bid;
   assert( !BidsById::get( bid.buyer, existing_bid ), "order with this id already exists" );
   print( __FILE__, __LINE__, "\n" );

   auto buyer_account = getAccount( bid.buyer.name );
   buyer_account.eos_balance -= bid.quantity;

   Ask lowest_ask;
   if( !AsksByPrice::front( lowest_ask ) ) {
      print( "\n No asks found, saving buyer account and storing bid\n" );
      assert( !order.fill_or_kill, "order not completely filled" );
      Bids::store( bid );
      buyer_account.open_orders++;
      save( buyer_account );
      return;
   }

   print( "ask: ", lowest_ask, "\n" );
   print( "bid: ", bid, "\n" );

   auto seller_account = getAccount( lowest_ask.seller.name );

   while( lowest_ask.price <= bid.price ) {
      print( "lowest ask <= bid.price\n" );
      match( bid, buyer_account, lowest_ask, seller_account );

      if( lowest_ask.quantity == CurrencyTokens(0) ) {
         seller_account.open_orders--;
         save( seller_account );
         save( buyer_account );
         Asks::remove( lowest_ask );
         if( !AsksByPrice::front( lowest_ask ) ) {
            break;
         }
         seller_account = getAccount( lowest_ask.seller.name );
      } else {
         break; // buyer's bid should be filled
      }
   }
   print( "lowest_ask >= bid.price or buyer's bid has been filled\n" );

   if( bid.quantity && !order.fill_or_kill ) buyer_account.open_orders++;
   save( buyer_account );
   print( "saving buyer's account\n" );
   if( bid.quantity ) {
      print( bid.quantity, " eos left over" );
      assert( !order.fill_or_kill, "order not completely filled" );
      Bids::store( bid );
      return;
   }
   print( "bid filled\n" );

}

void apply_exchange_sell( SellOrder order ) {
   Ask& ask = order;
   requireAuth( ask.seller.name ); 

   assert( ask.quantity > CurrencyTokens(0), "invalid quantity" );
   assert( ask.expiration > now(), "order expired" );

   print( "\n\n", Name(ask.seller.name), " created sell for ", order.quantity, 
          " currency at price: ", order.price, "\n");

   Ask existing_ask;
   assert( !AsksById::get( ask.seller, existing_ask ), "order with this id already exists" );

   auto seller_account = getAccount( ask.seller.name );
   seller_account.currency_balance -= ask.quantity;


   Bid highest_bid;
   if( !BidsByPrice::back( highest_bid ) ) {
      assert( !order.fill_or_kill, "order not completely filled" );
      print( "\n No bids found, saving seller account and storing ask\n" );
      Asks::store( ask );
      seller_account.open_orders++;
      save( seller_account );
      return;
   }

   print( "\n bids found, lets see what matches\n" );
   auto buyer_account = getAccount( highest_bid.buyer.name );

   while( highest_bid.price >= ask.price ) {
      match( highest_bid, buyer_account, ask, seller_account );

      if( highest_bid.quantity == EosTokens(0) ) {
         buyer_account.open_orders--;
         save( seller_account );
         save( buyer_account );
         Bids::remove( highest_bid );
         if( !BidsByPrice::back( highest_bid ) ) {
            break;
         }
         buyer_account = getAccount( highest_bid.buyer.name );
      } else {
         break; // buyer's bid should be filled
      }
   }
   
   if( ask.quantity && !order.fill_or_kill ) seller_account.open_orders++;
   save( seller_account );
   if( ask.quantity ) {
      assert( !order.fill_or_kill, "order not completely filled" );
      print( "saving ask\n" );
      Asks::store( ask );
      return;
   }

   print( "ask filled\n" );
}

void apply_exchange_cancel_buy( OrderID order ) {
   requireAuth( order.name ); 

   Bid bid_to_cancel;
   assert( BidsById::get( order, bid_to_cancel ), "bid with this id does not exists" );
   
   auto buyer_account = getAccount( order.name );
   buyer_account.eos_balance += bid_to_cancel.quantity;
   buyer_account.open_orders--;

   Bids::remove( bid_to_cancel );
   save( buyer_account );

   print( "bid removed\n" );
}

void apply_exchange_cancel_sell( OrderID order ) {
   requireAuth( order.name ); 

   Ask ask_to_cancel;
   assert( AsksById::get( order, ask_to_cancel ), "ask with this id does not exists" );

   auto seller_account = getAccount( order.name );
   seller_account.currency_balance += ask_to_cancel.quantity;
   seller_account.open_orders--;

   Asks::remove( ask_to_cancel );
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
               apply_exchange_buy( currentMessage<exchange::BuyOrder>() );
               break;
            case N(sell):
               apply_exchange_sell( currentMessage<exchange::SellOrder>() );
               break;
            case N(cancelbuy):
               apply_exchange_cancel_buy( currentMessage<exchange::OrderID>() );
               break;
            case N(cancelsell):
               apply_exchange_cancel_sell( currentMessage<exchange::OrderID>() );
               break;
            default:
               assert( false, "unknown action" );
         }
      } 
      else if( code == N(currency) ) {
        if( action == N(transfer) ) 
           apply_currency_transfer( currentMessage<currency::Transfer>() );
      } 
      else if( code == N(eos) ) {
        if( action == N(transfer) ) 
           apply_eos_transfer( currentMessage<eos::Transfer>() );
      } 
      else {
      }
   }
}
