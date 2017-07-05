
/**
 *  Transfer requires that the sender and receiver be the first two
 *  accounts notified and that the sender has provided authorization.
 */
struct Transfer {
  uint64_t    amount = 0;
  char        memo[]; /// extra bytes are treated as a memo and ignored by logic
};

void init()  {
   assert( currentContext() == currentCode() );
   store("balance", 1000ll * 1000ll * 1000ll );
}

void apply_currency_transfer() {
   auto context  = currentContext();
   auto code     = currentCode();
   auto transfer = readMessage<Transfer>();
   uint64_t balance = 0;

   auto from = getNotify(0);
   auto to   = getNotify(1);

   read( code, "balance", balance );  /// context is implied

   if( context == from ) {
      requireAuth( {from} );
      assert( balance >= transfer.amount, "insufficient funds" );
      balance -= transfer.amount;
   } else if( context == to ) {
      balance += transfer.amount;
   }

   store( "balance", balance ); ///< context, code is implied
}
