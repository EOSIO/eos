#include <exchange/exchange_accounts.hpp>

namespace eosio {

   void exchange_accounts::adjust_balance( account_name owner, extended_asset delta, const string& reason ) {
      (void)reason;

      /*查flat_map结构体的用户表,索引字段是用户ID*/
      auto table = exaccounts_cache.find( owner );
      if( table == exaccounts_cache.end() ) {
         /*表里没有该用户记录,插入一条空记录,指向第一个索引(用户ID)对应的value(代币表该用户的记录)的迭代器*/
         /*flat_map.emplace().first应该是指向插入的flat_map结构体记录本身(跟flat_map.find()的返回值
           相同)*/
         table = exaccounts_cache.emplace( owner, exaccounts(_this_contract, owner )  ).first;
      }
      /*查multi_index结构体的代币表，索引字段是owner*/
      /*table->second 应该是指向flat_map结构体的value(即exaccounts对象)*/
      auto useraccounts = table->second.find( owner );
      if( useraccounts == table->second.end() ) {
         /*没有代币表记录,赋值代币表记录值，插入代币表*/
         table->second.emplace( owner, [&]( auto& exa ){
           exa.owner = owner;
           exa.balances[delta.get_extended_symbol()] = delta.amount;
           eosio_assert( delta.amount >= 0, "overdrawn balance 1" );
         });
      } else {
         /*累计代币表记录的金额*/
         table->second.modify( useraccounts, 0, [&]( auto& exa ) {
           const auto& b = exa.balances[delta.get_extended_symbol()] += delta.amount;
           eosio_assert( b >= 0, "overdrawn balance 2" );
         });
      }
   }

} /// namespace eosio
