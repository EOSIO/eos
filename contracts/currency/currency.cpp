/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <currency/currency.hpp> /// defines transfer struct (abi)

namespace TOKEN_NAME {
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

   void apply_currency_transfer( const TOKEN_NAME::transfer& transfer_msg ) {
      require_notice( transfer_msg.to, transfer_msg.from );
      require_auth( transfer_msg.from );

      auto from = get_account( transfer_msg.from );
      auto to   = get_account( transfer_msg.to );

      from.balance -= transfer_msg.quantity; /// token subtraction has underflow assertion
      to.balance   += transfer_msg.quantity; /// token addition has overflow assertion

      store_account( transfer_msg.from, from );
      store_account( transfer_msg.to, to );
   }

}  // namespace TOKEN_NAME

using namespace TOKEN_NAME;

extern "C" {
    void init()  {
       account owned_account;
       //Initialize currency account only if it does not exist
       if ( !accounts::get( owned_account, N(currency) )) {
          store_account( N(currency), account( currency_tokens(1000ll*1000ll*1000ll) ) );
       }
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(currency) ) {
          if( action == N(transfer) ) 
             TOKEN_NAME::apply_currency_transfer( current_message< TOKEN_NAME::transfer >() );
       }
    }
}
