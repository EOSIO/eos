#include <exchange/exchange_accounts.hpp>

namespace eosio {

   void exchange_accounts::adjust_balance( account_name owner, extended_asset delta, const string& reason ) {
      (void)reason;

      auto table = exaccounts_cache.find( owner );
      if( table == exaccounts_cache.end() ) {
         table = exaccounts_cache.emplace( owner, exaccounts(_this_contract, owner )  ).first;
      }
      auto useraccounts = table->second.find( owner );
      if( useraccounts == table->second.end() ) {
         table->second.emplace( owner, [&]( auto& exa ){
           exa.owner = owner;
           exa.balances[delta.get_extended_symbol()] = delta.amount;
           eosio_assert( delta.amount >= 0, "overdrawn balance 1" );
         });
      } else {
         table->second.modify( useraccounts, 0, [&]( auto& exa ) {
           const auto& b = exa.balances[delta.get_extended_symbol()] += delta.amount;
           eosio_assert( b >= 0, "overdrawn balance 2" );
         });
      }
   }

} /// namespace eosio
