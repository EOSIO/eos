/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <currency/currency.hpp> /// defines transfer struct (abi)

namespace TOKEN_NAME {
   using namespace eos;

   ///  When storing accounts, check for empty balance and remove account
   void storeAccount( account_name account, const Account& a ) {
      if( a.isEmpty() ) {
         ///               value, scope
         Accounts::remove( a, account );
      } else {
         ///              value, scope
         Accounts::store( a, account );
      }
   }

   void apply_currency_transfer( const TOKEN_NAME::Transfer& transfer ) {
      require_notice( transfer.to, transfer.from );
      require_auth( transfer.from );

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
       storeAccount( N(currency), Account( CurrencyTokens(1000ll*1000ll*1000ll) ) );
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(currency) ) {
          if( action == N(transfer) ) 
             currency::apply_currency_transfer( current_message< TOKEN_NAME::Transfer >() );
       }
    }
}
