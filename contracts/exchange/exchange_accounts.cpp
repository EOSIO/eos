#include <exchange/exchange_accounts.hpp>

namespace eosio {

   void exchange_accounts::adjust_balance( account_name owner, extended_asset delta, const string& reason ) {
      (void)reason;

      auto table = exaccounts_cache.find( owner );
      if( table == exaccounts_cache.end() ) {
         table = exaccounts_cache.emplace( owner, std::make_unique<exaccounts>(_this_contract, owner )  ).first;
      }
      auto useraccounts = table->second->find( owner );
      if( useraccounts == table->second->end() ) {
         table->second->emplace( owner, [&]( auto& exa ){
           exa.owner = owner;
           exa.balances[delta.get_extended_symbol()] = delta.amount;
           eosio_assert( delta.amount >= 0, "overdrawn balance 1" );
         });
      } else {
         // Caution: don't add code which removes entries from exaccounts or exaccounts_cache. It will
         //          cause failures in margin calls, continuations, and other actions which deposit funds,
         //          since they will attempt to increase owner's ram usage without owner's authority.
         table->second->modify( useraccounts, 0, [&]( auto& exa ) {
           const auto& b = exa.balances[delta.get_extended_symbol()] += delta.amount;
           eosio_assert( b >= 0, "overdrawn balance 2" );
         });
      }
   }

} /// namespace eosio
