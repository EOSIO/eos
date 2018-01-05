/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.system/eosio.system.hpp> 
#include <eoslib/singleton.hpp>

namespace eosiosystem {
   using namespace eosio;

   typedef eosio::singleton<eosiosystem::account, system_code, account_table> account_single;

   void on( const eosiosystem::transfer& act ) {
      require_recipient( act.to, act.from );
      require_auth( act.from );

      auto from = account_single::get_or_create( act.from );
      auto to   = account_single::get_or_create( act.to );

      from.balance -= act.quantity; /// token subtraction has underflow assertion
      to.balance   += act.quantity; /// token addition has overflow assertion

      account_single::set( act.from, from );
      account_single::set( act.to, to );
   }

   void on( const eosiosystem::stakevote& act ) {
      require_recipient( act.to, act.from );
      require_auth( act.from );

      auto from = account_single::get_or_create( act.from );
      from.balance -= act.quantity; /// token subtraction has underflow assertion
      account_single::set( act.from, from );

      auto to        = account_single::get_or_create( act.to );
      to.vote_stake += act.quantity; /// token addition has overflow assertion
      account_single::set( act.to, to );

      /// TODO: iterate over all producer votes on act.to 
      /*
      producer_vote current;
      if( producer_votes::by_voter::front( current ) ) {
         do {

         } while ( producer_votes::by_voter::next( current ) );
      }
      */
      
   }

    void init()  {
       assert( !account_single::exists( system_code ), "contract already initialized" ) ;

       account system;
       system.balance = native_tokens( 1000ll*1000ll*1000ll );
       account_single::set( system_code, system );
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
             on( unpack_action<eosiosystem::transfer>() );
          }
       }
    }
}
