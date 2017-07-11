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
#include <currency/currency.hpp> /// defines transfer struct

typedef uint128_t Price;

struct OrderID {
   uint64_t owner = 0;
   uint64_t id    = 0;

   operator const uint128_t& ()const { 
      return *reinterpret_cast<const uint128_t*>(this);
   }

   operator uint128_t& (){ 
      return *reinterpret_cast<uint128_t*>(this);
   }
};
static_assert( sizeof(OrderID) == sizeof(uint128_t), "unexpected packing" );

struct Order {
   OrderID&  primary()  { return id;    }
   Price&    secondary(){ return price; }

   OrderID   id;
   Price     price;
   uint64_t  quantity;
   Time      expiration;
};

using Bids = TABLE(exchange,exchange,bids,Order); 
using Asks = TABLE(exchange,exchange,asks,Order); 
typedef Asks::SecondaryIndex  AsksByPrice;
typedef Bids::SecondaryIndex  BidsByPrice;
typedef Asks::PrimaryIndex  AsksById;
typedef Bids::PrimaryIndex  BidsById;

struct Account {
   uint64_t   a = 0;
   uint64_t   b = 0;
   int        open_orders = 0;

   bool isEmpty()const { return !(a|b|open_orders); }

   static Name tableId() { return NAME("account"); }
};



/**
 *  This method is called after the "transfer" action of code
 *  "currencya" is called and "exchange" is listed in the notifiers.
 */
void apply_currencya_transfer() {
   const auto& transfer  = currentMessage<Transfer>();

   if( transfer.to == N(exchange) ) {
      Account to_balance;
      Db::get( transfer.from, to_balance );
      to_balance.a += transfer.amount;
      Db::store( transfer.from, to_balance );
   } else if( transfer.from == N(exchange) ) {
      requireAuth( transfer.to ); /// require the reciever of funds (account owner) to authorize this transfer

      Account to_balance;
      Db::get( transfer.to, to_balance );
      assert( to_balance.a >= transfer.amount, "insufficient funds to withdraw" );
      to_balance.a -= transfer.amount;

      if( to_balance.isEmpty() )
         Db::remove<Account>( transfer.to );
      else
         Db::store( transfer.to, to_balance );
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}

void apply_currencyb_transfer() {
   const auto& transfer  = currentMessage<Transfer>();

   if( transfer.to == N(exchange) ) {
      Account to_balance;
      Db::get( transfer.from, to_balance );
      to_balance.b += transfer.amount;
      Db::store( transfer.from, to_balance );
   } else if( transfer.from == N(exchange) ) {
      requireAuth( transfer.to ); /// require the reciever of funds (account owner) to authorize this transfer

      Account to_balance;
      Db::get( transfer.to, to_balance );
      assert( to_balance.b >= transfer.amount, "insufficient funds to withdraw" );
      to_balance.b -= transfer.amount;

      if( to_balance.isEmpty() )
         Db::remove<Account>( transfer.to );
      else
         Db::store( transfer.to, to_balance );
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}

struct Buy {
   AccountName buyer;
   Price       price;
   uint64_t    quantity = 0;
   uint32_t    id = 0;
   Time        expiration;
   uint8_t     fill_or_kill = false;
};

struct Sell {
   AccountName seller;
   Price       price;
   uint64_t    quantity = 0;
   uint32_t    id = 0;
   Time        expiration;
   uint8_t     fill_or_kill = false;
};

void fill( Order& bid, Order& ask, Account& buyer, Account& seller, uint64_t usd, uint64_t token ) {
   bid.quantity -= usd;
   seller.a += usd;
   ask.quantity -= token;
   buyer.b += token;
}

/**
 * 
 *  
 */
void apply_exchange_buy() {
   auto buy = currentMessage<Buy>();
   assert( buy.expiration > now(), "order expired" );

   Account buyer_account;

   Db::get( buy.buyer, buyer_account );
   assert( buyer_account.a >= buy.quantity, "insufficient funds" );
   assert( buy.quantity > 0, "invalid quantity" );
   buyer_account.a -= buy.quantity;

   Order buyer_bid;
   assert( BidsById::get( OrderID{ buy.buyer, buy.id}, buyer_bid ), "order with this id already exists" );

   buyer_bid.price         = buy.price;
   buyer_bid.id.owner      = buy.buyer;
   buyer_bid.id.id         = buy.id;
   buyer_bid.quantity      = buy.quantity;
   buyer_bid.expiration    = buy.expiration;

   Order lowest_ask;

   if( !AsksByPrice::front( lowest_ask ) ) {
      assert( !buy.fill_or_kill, "order not completely filled" );
      Bids::store( buyer_bid );
      Db::store( buy.buyer, buyer_account );
      return;
   }

   Account seller_account;
   Db::get( lowest_ask.id.owner, seller_account );

   while( lowest_ask.price <= buyer_bid.price ) {
      auto ask_usd           = lowest_ask.price * lowest_ask.quantity;
      auto fill_amount_usd   = min<uint64_t>( ask_usd, buyer_bid.quantity );
      uint64_t fill_amount_token = 0;

      lowest_ask.quantity -= fill_amount_token;
      buy.quantity        -= fill_amount_usd;

      if( fill_amount_usd == ask_usd ) { /// complete fill of ask
         fill_amount_token = lowest_ask.quantity;
      } else { /// complete fill of buy
         fill_amount_token = fill_amount_usd / lowest_ask.price;
      }
      /// either fill_amount_token == seller.quantity or fill_amount_usd == buy.quantity

      fill( buyer_bid, lowest_ask, buyer_account, seller_account, fill_amount_usd, fill_amount_token );

      if( lowest_ask.quantity == 0 ) {
         Db::store( lowest_ask.id.owner, seller_account );
         Asks::remove( lowest_ask );
         if( !AsksByPrice::front( lowest_ask ) ) {
            break;
         }
         Db::get( lowest_ask.id.owner, seller_account );
      } else {
         break; // buyer's bid should be filled
      }
   }

   Db::store( buy.buyer, buyer_account );
   if( buyer_bid.quantity > 0 ) {
      assert( !buy.fill_or_kill, "order not completely filled" );
      Bids::store( buyer_bid );
   }
}

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
      if( code == N(currencya) ) {
        if( action == N(transfer) ) apply_currencya_transfer();
      } else if( code == N(currencyb) ) {
        if( action == N(transfer) ) apply_currencyb_transfer();
      } else {
      }
   }
}
