/**
 *  @file exchange.cpp
 *  @brief defines an example exchange contract 
 *
 *  This exchange contract assumes the existence of two currency contracts
 *  located at @currencya and @currencyb.  These currency contracts have
 *  provided an API header defined in currench.hpp which the exchange 
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
using namespace exchange;

namespace exchange {
void save( const Account& a ) {
   if( a.isEmpty() )
      Db::remove( N(exchange), N(account), a.owner );
   else
      Db::store( N(exchange), N(account), a.owner, a );
}

template<typename Lambda>
void modifyAccount( AccountName a, Lambda&& modify ) {
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
      modifyAccount( transfer.to, [&]( Account& account ){ 
          account.currency_balance += transfer.quantity; 
      });
   } else if( transfer.from == N(exchange) ) {
      requireAuth( transfer.to ); /// require the reciever of funds (account owner) to authorize this transfer

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
      modifyAccount( transfer.to, [&]( Account& account ){ 
          account.eos_balance += transfer.quantity; 
      });
   } else if( transfer.from == N(exchange) ) {
      requireAuth( transfer.to ); /// require the reciever of funds (account owner) to authorize this transfer

      modifyAccount( transfer.to, [&]( Account& account ){ 
          account.eos_balance -= transfer.quantity; 
      });
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}


void fill( Bid& bid, Ask& ask, Account& buyer, Account& seller, 
           eos::Tokens e, currency::Tokens c) {
   bid.quantity -= e;
   seller.eos_balance += e;

   ask.quantity -= c;
   buyer.currency_balance += c;
}

void match( Bid& bid, Account& buyer, Ask& ask, Account& seller ) {
   eos::Tokens ask_eos = ask.quantity * ask.price;

   eos::Tokens      fill_amount_eos = min<eos::Tokens>( ask_eos, bid.quantity );
   currency::Tokens fill_amount_currency;

   if( fill_amount_eos == ask_eos ) { /// complete fill of ask
      fill_amount_currency = ask.quantity;
   } else { /// complete fill of buy
      fill_amount_currency = fill_amount_eos / ask.price;
   }

   fill( bid, ask, buyer, seller, fill_amount_eos, fill_amount_currency );
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

   static Bid existing_bid;
   assert( BidsById::get( bid.buyer, existing_bid ), "order with this id already exists" );

   auto buyer_account = getAccount( bid.buyer.name );
   buyer_account.eos_balance -= bid.quantity;

   static Ask lowest_ask;
   if( !AsksByPrice::front( lowest_ask ) ) {
      assert( !order.fill_or_kill, "order not completely filled" );
      Bids::store( bid );
      save( buyer_account );
      return;
   }

   auto seller_account = getAccount( lowest_ask.seller.name );

   while( lowest_ask.price <= bid.price ) {
      match( bid, buyer_account, lowest_ask, seller_account );

      if( lowest_ask.quantity == currency::Tokens(0) ) {
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

   save( buyer_account );
   if( bid.quantity ) {
      assert( !order.fill_or_kill, "order not completely filled" );
      Bids::store( bid );
   }
}

void apply_exchange_sell( SellOrder order ) {
   Ask& ask = order;
   requireAuth( ask.seller.name ); 

   assert( ask.quantity > currency::Tokens(0), "invalid quantity" );
   assert( ask.expiration > now(), "order expired" );

   static Ask existing_ask;
   assert( AsksById::get( ask.seller, existing_ask ), "order with this id already exists" );

   auto seller_account = getAccount( ask.seller.name );
   seller_account.currency_balance -= ask.quantity;

   static Bid highest_bid;
   if( !BidsByPrice::back( highest_bid ) ) {
      assert( !order.fill_or_kill, "order not completely filled" );
      Asks::store( ask );
      save( seller_account );
      return;
   }

   auto buyer_account = getAccount( highest_bid.buyer.name );

   while( highest_bid.price >= ask.price ) {
      match( highest_bid, buyer_account, ask, seller_account );

      if( highest_bid.quantity == eos::Tokens(0) ) {
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

   save( seller_account );
   if( ask.quantity ) {
      assert( !order.fill_or_kill, "order not completely filled" );
      Asks::store( ask );
   }
}

/*
void apply_exchange_sell() {
   auto sell = currentMessage<Sell>();
   assert( sell.expiration > now(), "order expired" );

   Account seller_account;

   Db::get( sell.seller, seller_account );
   assert( seller_account.b >= sell.quantity, "insufficient funds" );
   assert( sell.quantity > 0, "invalid quantity" );
   seller_account.b -= sell.quantity;

   Order seller_ask;
   assert( AsksById::get( OrderID{ sell.seller, sell.id}, seller_ask ), "order with this id already exists" );

   seller_ask.price         = sell.price;
   seller_ask.id.owner      = sell.seller;
   seller_ask.id.id         = sell.id;
   seller_ask.quantity      = sell.quantity;
   seller_ask.expiration    = sell.expiration;

   Order highest_bid;

   if( !BidsByPrice::back( highest_bid ) ) {
      assert( !sell.fill_or_kill, "order not completely filled" );
      Bids::store( seller_ask );
      Db::store( sell.seller, seller_account );
      return;
   }

   Account buyer_account;
   Db::get( highest_bid.id.owner, buyer_account );

   while( highest_bid.price >= seller_ask.price ) {
      auto ask_usd           = seller_ask.quantity;
      auto bid_usd           = seller_ask.price * seller_ask.quantity;
      auto fill_amount_usd   = min<uint64_t>( ask_usd, bid_usd );
      uint64_t fill_amount_token = 0;

      seller_ask.quantity  -= fill_amount_token;
      highest_bid.quantity -= fill_amount_usd;

      if( fill_amount_usd == ask_usd ) { /// complete fill of ask
         fill_amount_token = seller_ask.quantity;
      } else { /// complete fill of buy
         fill_amount_token = fill_amount_usd / seller_ask.price;
      }
      /// either fill_amount_token == seller.quantity or fill_amount_usd == buy.quantity

      fill( highest_bid, seller_ask, buyer_account, seller_account, fill_amount_usd, fill_amount_token );

      if( highest_bid.quantity == 0 ) {
         Db::store( highest_bid.id.owner, buyer_account );
         Asks::remove( highest_bid );
         if( !BidsByPrice::back( highest_bid ) ) {
            break;
         }
         Db::get( highest_bid.id.owner, buyer_account );
      } else {
         break; // buyer's bid should be filled
      }
   }

   Db::store( sell.seller, seller_account );
   if( seller_ask.quantity > 0 ) {
      assert( !sell.fill_or_kill, "order not completely filled" );
      Asks::store( seller_ask );
   }
}
*/
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
