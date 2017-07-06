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
#include <contracts/currency.hpp> /// defines transfer struct


struct Account {
   uint64_t   a = 0;
   uint64_t   b = 0;
   int        open_orders = 0;

   bool isEmpty()const { return !(a|b|open_orders); }
   /**
    *  Balance records for all exchange users are stored here
    *  exchange/exchange/balance/username -> Balance
    */
   static Name tableId() { return Name("balance"); }
};


extern "C" {
void init() {
   setAuthority( "currencya", "transfer", "anyone" );
   setAuthority( "currencyb", "transfer", "anyone" );
   registerHandler( "apply", "currencya", "transfer" );
   registerHandler( "apply", "currencyb", "transfer" );
}

/**
 *  This method is called after the "transfer" action of code
 *  "currencya" is called and "exchange" is listed in the notifiers.
 */
void apply_currencya_transfer() {
   const auto& transfer  = currentMessage<Transfer>();

   if( transfer.to == "exchange" ) {
      static Balance to_balance;
      Db::get( transfer.from, to_balance );
      to_balance.a += transfer.amount;
      Db::store( transfer.from, to_balance );
   } else if( transfer.from == "exchange" ) {
      requireAuth( transfer.to ); /// require the reciever of funds (account owner) to authorize this transfer

      static Balance to_balance;
      auto balance = Db::get( transfer.to, to_balance );
      assert( balance.a >= transfer.amount, "insufficient funds to withdraw" );
      balance.a -= transfer.amount;

      if( balance.isEmpty() )
         Db::remove<Balance>( transfer.to );
      else
         Db::store( transfer.to, to_balance );
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}

/**
 *  This method is called after the "transfer" action of code
 *  "currencya" is called and "exchange" is listed in the notifiers.
 */
void apply_currencyb_transfer() {
   const auto& transfer  = currentMessage<Transfer>();

   if( transfer.to == "exchange" ) {
      static Balance to_balance;
      Db::get( transfer.from, to_balance );
      to_balance.b += transfer.amount;
      Db::store( transfer.from, to_balance );
   } else if( transfer.from == "exchange" ) {
      requireAuth( transfer.to ); /// require the reciever of funds (account owner) to authorize this transfer

      static Balance to_balance;
      auto balance = Db::get( transfer.to, to_balance );
      assert( balance.b >= transfer.amount, "insufficient funds to withdraw" );
      balance.b -= transfer.amount;

      if( balance.isEmpty() )
         Db::remove<Balance>( transfer.to );
      else
         Db::store( transfer.to, to_balance );
   } else {
      assert( false, "notified on transfer that is not relevant to this exchange" );
   }
}

} // extern "C"
