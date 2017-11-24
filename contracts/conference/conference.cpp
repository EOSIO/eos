#include <currency/conference.hpp> /// defines transfer struct (abi)

namespace TOKEN_NAME {
   using namespace eosio;

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

   void apply_currency_transfer( const TOKEN_NAME::Transfer& transfer ) {
      requireNotice( transfer.guest );
      requireAuth( transfer.guest );

      auto guest = getAccount( transfer.guest );

      guest.balance -= transfer.quantity; /// token subtraction has underflow assertion

      storeAccount( transfer.guest, guest );
   }

   void apply_currency_refund( const TOKEN_NAME::Transfer& transfer ) {

      auto guest = getAccount( transfer.guest );

      guest.balance = 0;

      storeAccount( transfer.guest, guest );
   }

}  // namespace TOKEN_NAME

using namespace currency;

extern "C" {
    void init()  {
       storeAccount( N(currency), Account( CurrencyTokens(1000ll*1000ll*1000ll) ) );
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(conference) ) {
          if( action == N(transfer) )
             currency::apply_currency_transfer( currentMessage< TOKEN_NAME::Transfer >() );
          else if(action == N(refund) )
             currency::apply_currency_refund( currentMessage <TOKEN_NAME::Transfer >() );
       }
    }
}
