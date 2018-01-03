/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.system/eosio.system.hpp> 

namespace eosiosystem {
   using namespace eosio;

   ///  When storing accounts, check for empty balance and remove account
   void store_account( account_name account_to_store, const account& a ) {
      /// value, scope
      accounts::store( a, account_to_store );
   }

   void on( const eosiosystem::transfer& transfer_msg ) {
      require_recipient( transfer_msg.to, transfer_msg.from );
      require_auth( transfer_msg.from );

      auto from = get_account( transfer_msg.from );
      auto to   = get_account( transfer_msg.to );

      from.balance -= transfer_msg.quantity; /// token subtraction has underflow assertion
      to.balance   += transfer_msg.quantity; /// token addition has overflow assertion

      store_account( transfer_msg.from, from );
      store_account( transfer_msg.to, to );
   }

    void init()  {
       // TODO verify that 
       //store_account( system_name, account( native_tokens(1000ll*1000ll*1000ll) ) );
    }

}  // namespace eosiosystem

using namespace eosiosystem;

extern "C" {

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == system_code ) {
          if( action == N(init) ) {
             init();
          } else if( action == N(transfer) ) {
             on( current_action< eosiosystem::transfer >() );
          }
       }
    }
}
