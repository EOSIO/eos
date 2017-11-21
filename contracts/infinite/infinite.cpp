/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <infinite/infinite.hpp> /// defines transfer struct (abi)

namespace infinite {
   using namespace eosio;

   ///  When storing accounts, check for empty balance and remove account
   void store_account( account_name account_to_store, const account& a ) {
      if( a.is_empty() ) {
         ///               value, scope
         accounts::remove( a, account_to_store );
      } else {
         ///              value, scope
         accounts::store( a, account_to_store );
      }
   }

   void apply_currency_transfer( const infinite::transfer& transfer ) {
      require_notice( transfer.to, transfer.from );
      require_auth( transfer.from );

      auto from = get_account( transfer.from );
      auto to   = get_account( transfer.to );

      while (from.balance > infinite::currency_tokens())
      {
         from.balance -= transfer.quantity; /// token subtraction has underflow assertion
         to.balance   += transfer.quantity; /// token addition has overflow assertion
      }

      store_account( transfer.from, from );
      store_account( transfer.to, to );
   }

}  // namespace infinite

using namespace infinite;

extern "C" {
    void init()  {
       store_account( N(currency), account( currency_tokens(1000ll*1000ll*1000ll) ) );
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(currency) ) {
          if( action == N(transfer) )
             infinite::apply_currency_transfer( current_action< infinite::transfer >() );
       }
    }
}
