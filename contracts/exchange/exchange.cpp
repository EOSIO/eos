
struct Transfer {
  AccountName from;
  AccountName to;
  uint64_t    amount;
  char        memo[];
};

struct Withdraw {
  AccountName from;
  AccountName currency;
  uint64_t    amount;
};

struct Balance {
   uint64_t   a = 0;
   uint64_t   b = 0;
};

/**
 *  This method is called after the "transfer" action of code
 *  "currencya" is called and "exchange" is listed in the notifiers.
 */
void apply_currencya_transfer() {
   assert( "exchange" == currentCode() );
   assert( "exchange" == currentContext() );

   auto transfer = readMessage<Transfer>();
   if( transfer.to == "exchange" ) {
      auto balance = read<Balance>( code, "balance" );
      balance.a += transfer.amount;
      store( "balance", balance );
   } else if( transfer.from == "exchange" ) {
      auto balance = read<Balance>( code, "balance" );
      assert( balance.a >= transfer.amount, "insufficient funds to withdraw" );
      balance.a -= transfer.amount;
      store( "balance", balance );
   } else {
      assert( false, "notified on currencya transfer that is not relevant to this exchange" );
   }
}

/**
 *  TODO: determine the order in which notifers are called... perhaps to follow
 *        the order they are listed in the message
 *  This method is called when the "transfer" action of code
 *  "currencyb" is called and "exchange" is listed in the notifiers.
 */
void apply_currencyb_transfer() {
   assert( "exchange" == currentCode() );
   assert( "exchange" == currentContext() );

   auto transfer = readMessage<Transfer>();
   if( transfer.to == "exchange" ) {
      auto balance = read<Balance>( code, "balance" );
      balance.b += transfer.amount;
   } else if( transfer.from == "exchange" ) {
      requireAuth( {transfer.from} ); /// require the reciever of funds (account owner) to authorize this 
      auto balance = read<Balance>( code, "balance" );
      assert( balance.b >= transfer.amount, "insufficient balance" );
      balance.b -= transfer.amount;
      store( "balance", balance );
   } else {
      assert( false, "notified on currencyb transfer that is not relevant to this exchange" );
   }
}

/**
 *  This method is called when the "withdraw" action of the code "exchange" is called
 *  and "exchange" is notified. It will attempt to issue a sync transfer to the proper
 *  currency. The currency will in turn call our transfer handler which will verify that
 *  there is sufficient balance to process the withdraw and decrement the balance.
 *
 *  This entire process can be ignored if @exchange grants "anyone" the ability to transfer
 *  either of the currencies and then validates the "withdraw transfer" by requiring the
 *  authority of the 
 *
 *
 *
void apply_exchange_withdraw() {
   assert( "exchange" == currentCode() );
   assert( "exchange" == currentContext() );

   auto balance = read<Balance>( code, "balance" );

   auto withdraw = readMessage<Withdraw>();
   if( withdraw.currency == "currencya" ) {
      assert( balance.a >= withdraw.amount );
   }
   else if( withdraw.currency == "currencyb" ) {
      assert( balance.b >= withdraw.amount );
   } else {
      assert( false, "invalid currency" );
   }
   Transfer transfer;
   transfer.amount = withdraw.amount;

   /// in blockchain this shows up as "generated and applied" vs just "generated", 
   /// for this method to succeed withdraw.from must be defined within the current scope
   ///        code               authorities    notifiers                   message
   sync_send( withdraw.currency, {"exchange"}, {"exchange", withdraw.from}, transfer );
};
*/




