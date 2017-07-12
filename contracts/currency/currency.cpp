#include <currency/currency.hpp> /// defines transfer struct (abi)

namespace TOKEN_NAME {

   ///  When storing accounts, check for empty balance and remove account
   void storeAccount( AccountName account, const Account& a ) {
      if( a.isEmpty() ) {
         printi(account);
         ///        scope    table       key
         Db::remove( account, N(account), N(account) );
      } else {
         ///        scope    table       key         value
         Db::store( account, N(account), N(account), a );
      }
   }

   void apply_currency_transfer( const TOKEN_NAME::Transfer& transfer ) {
      requireNotice( transfer.to, transfer.from );
      requireAuth( transfer.from );

      auto from = getAccount( transfer.from );
      auto to   = getAccount( transfer.to );

      from.balance -= transfer.quantity; /// token subtraction has underflow assertion
      to.balance   += transfer.quantity; /// token addition has overflow assertion

      storeAccount( transfer.from, from );
      storeAccount( transfer.to, to );
   }

}  // namespace TOKEN_NAME

using namespace currency;

extern "C" {
    void init()  {
       storeAccount( N(currency), Account{ Tokens(1000ll*1000ll*1000ll) } );
    }

   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t code, uint64_t action ) {
      if( code == N(currency) ) {
         if( action == N(transfer) ) 
            currency::apply_currency_transfer( currentMessage< TOKEN_NAME::Transfer >() );
      }
   }
}
