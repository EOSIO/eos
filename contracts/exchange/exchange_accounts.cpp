#include <exchange/exchange_accounts.hpp>

namespace eosio {

   void exchange_accounts::adjust_balance( account_name owner, extended_asset delta, const string& reason ) {
      (void)reason;

      /*��flat_map�ṹ����û���,�����ֶ����û�ID*/
      auto table = exaccounts_cache.find( owner );
      if( table == exaccounts_cache.end() ) {
         /*����û�и��û���¼,����һ���ռ�¼,ָ���һ������(�û�ID)��Ӧ��value(���ұ���û��ļ�¼)�ĵ�����*/
         /*flat_map.emplace().firstӦ����ָ������flat_map�ṹ���¼����(��flat_map.find()�ķ���ֵ
           ��ͬ)*/
         table = exaccounts_cache.emplace( owner, exaccounts(_this_contract, owner )  ).first;
      }
      /*��multi_index�ṹ��Ĵ��ұ������ֶ���owner*/
      /*table->second Ӧ����ָ��flat_map�ṹ���value(��exaccounts����)*/
      auto useraccounts = table->second.find( owner );
      if( useraccounts == table->second.end() ) {
         /*û�д��ұ��¼,��ֵ���ұ��¼ֵ��������ұ�*/
         table->second.emplace( owner, [&]( auto& exa ){
           exa.owner = owner;
           exa.balances[delta.get_extended_symbol()] = delta.amount;
           eosio_assert( delta.amount >= 0, "overdrawn balance 1" );
         });
      } else {
         /*�ۼƴ��ұ��¼�Ľ��*/
         table->second.modify( useraccounts, 0, [&]( auto& exa ) {
           const auto& b = exa.balances[delta.get_extended_symbol()] += delta.amount;
           eosio_assert( b >= 0, "overdrawn balance 2" );
         });
      }
   }

} /// namespace eosio
