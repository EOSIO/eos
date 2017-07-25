#include <infinite/infinite.hpp> /// defines transfer struct (abi)

namespace infinite {
   using namespace eos;

   ///  When storing accounts, check for empty balance and remove account
   void storeAccount( AccountName account, const Account& a ) {
      if( a.isEmpty() ) {
         ///               value, scope
         Accounts::remove( a, account );
      } else {
         ///              value, scope
         Accounts::store( a, account );
      }
   }

   void apply_currency_transfer( const infinite::Transfer& transfer ) {
      requireNotice( transfer.to, transfer.from );
      requireAuth( transfer.from );

      auto from = getAccount( transfer.from );
      auto to   = getAccount( transfer.to );

      while (from.balance > infinite::CurrencyTokens())
      {
         from.balance -= transfer.quantity; /// token subtraction has underflow assertion
         to.balance   += transfer.quantity; /// token addition has overflow assertion
      }

      storeAccount( transfer.from, from );
      storeAccount( transfer.to, to );
   }

}  // namespace infinite

using namespace infinite;

extern "C" {
    void init()  {
       storeAccount( N(currency), Account( CurrencyTokens(1000ll*1000ll*1000ll) ) );
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(currency) ) {
          if( action == N(transfer) ) 
             infinite::apply_currency_transfer( currentMessage< infinite::Transfer >() );
       }
    }
}
