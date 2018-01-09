/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.system/eosio.system.hpp> 
#include <eoslib/singleton.hpp>

namespace bancor {
   using namespace eosio;

   typedef eosio::singleton<eosiosystem::account, system_code, eosiosytem::account_table> eosio_account;
   typedef eosio::singleton<currency::account, currency_code, currency::account_table>    currency_account;
   typedef eosio::singleton<bancor::account, bancor_code, bancor::account_table>          bancor_account;
   typedef eosio::singleton<bancor::account, bancor_code, bancor::account_table>          bancor_supply;


   struct token {
      uint64_t quantity;
      name type;
   };

   struct price {
      token base;
      token quote;
   };

   struct connector {
      name     type;
      uint16_t weight;
   };

   struct bancor_state {
      bancor_tokens     bancor_balance;
      bancor_tokens     bancor_supply;
      vector<connector> connectors; /// sorted list (logn lookup)

      uint16_t get_weight( name type );
   };

   asset convert( asset from, asset to, bancor_state& state ) {
   }

   current_state global_current_state;


   bancor_tokens eos_to_bancor( eos_tokens in, bancor_state& state ) {
      auto relay_eos_account    = eosio_account::get( N(bancor) );
      auto relay_bancor_supply  = state.bancor_supply;

      const auto weight = 500*1000; /// out of 1M,  (50%)

      auto old_relay_eos = relay_eos_account.balance - in;

      auto price = (old_relay_eos * 2) / relay_bancor_supply.balance;
      auto initial_out = price * in;

      auto new_relay_bancor_suply = relay_bancor_supply + initial_out;

      auto new_price = (relay_eos_account.balance * 2) / new_relay_bancor_suply;
      auto final_out = new_price * in;


      relay_bancor_supply.balance += final_out;
      store( relay_bancor_supply );

      return final_out;
   }

   eos_tokens bancor_to_eos( bancor_tokens in ) { 
   }

   bancor_tokens currency_to_bancor( currency::currency_tokens currency_in ) {
   }

   currency_tokens bancor_to_currency( bancor_tokens in ) { 
   }


   void on( const currency::transfer_memo& act ) {
      auto args = unpack<transfer_args>(act.memo);
      assert( can_convert( currency_type, args.to_token_type ) );

      auto state = read_state();

      auto bancor_out = currency_to_bancor( act.quantity, state );

      if( args.to_token_type == bancor ) {
         save_state( state );
         /// generate inline transfer...
         send_bancor( act.from, bancor_out );
      } else  {
         eos_out = bancor_to_eos( bancor_out, state );

         save_state( state );
         send_eos( act.from, eos_out );
      }
   }

   void on( const eosiosystem::transfer_memo& act ) {
      auto args = unpack<transfer_args>(act.memo);
      assert( can_convert( system_currency, args.to_token_type ) );
   }

   void on( const bancor::transfer_memo& act ) {
      require_recipient( act.to, act.from );
      require_auth( act.from );

      auto from = account_single::get_or_create( act.from );
      auto to   = account_single::get_or_create( act.to );

      from.balance -= act.quantity; /// token subtraction has underflow assertion
      to.balance   += act.quantity; /// token addition has overflow assertion

      account_single::set( act.from, from );
      account_single::set( act.to, to );

      auto args = unpack<transfer_args>(act.memo);
      assert( can_convert( bancor_currency, args.to_token_type ) );
   }



   void init()  {
      /*
      assert( !account_single::exists( system_code ), "contract already initialized" ) ;

      account system;
      system.balance = native_tokens( 1000ll*1000ll*1000ll );
      account_single::set( system_code, system );
      */
   }

}  // namespace eosiosystem

using namespace eosiosystem;

extern "C" {

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       init_global_current_state();

       if( code == bancor_code ) {
          if( action == N(init) ) {
             init();
          }
           else if( action == N(transfer) ) {
             on( unpack_action<bancor::transfer_memo>() );
          }
       }
       else if( code == currency_code ) {
          if( action == N(transfer) ) {
             on( unpack_action<currency::transfer_memo>() );
          }
       }
       else if( code == system_code ) {
          if( action == N(transfer) ) {
             on( unpack_action<eosiosystem::transfer_memo>() );
          }
       }
    }
}
