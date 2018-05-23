#include <eosiolib/eosio.hpp>

class simpletoken : public eosio::contract {
   public:
      simpletoken( eosio::account_name self )
      :contract(self),_accounts( _self, _self){}

      void transfer( eosio::account_name from, eosio::account_name to, uint64_t quantity ) {
         require_auth( from );

         const auto& fromacnt = _accounts.get( from );
         eosio_assert( fromacnt.balance >= quantity, "overdrawn balance" );
         _accounts.modify( fromacnt, from, [&]( auto& a ){ a.balance -= quantity; } );

         add_balance( from, to, quantity );
      }

      void issue( eosio::account_name to, uint64_t quantity ) {
         require_auth( _self );
         add_balance( _self, to, quantity );
      }

   private:
      struct account {
         eosio::account_name owner;
         uint64_t     balance;

         uint64_t primary_key()const { return owner; }
      };

      eosio::multi_index<N(accounts), account> _accounts;

      void add_balance( eosio::account_name payer, eosio::account_name to, uint64_t q ) {
         auto toitr = _accounts.find( to );
         if( toitr == _accounts.end() ) {
           _accounts.emplace( payer, [&]( auto& a ) {
              a.owner = to;
              a.balance = q;
           });
         } else {
           _accounts.modify( toitr, 0, [&]( auto& a ) {
              a.balance += q;
              eosio_assert( a.balance >= q, "overflow detected" );
           });
         }
      }
};

EOSIO_ABI( simpletoken, (transfer)(issue) )
