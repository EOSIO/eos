/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

using namespace eosio;

CONTRACT simpletoken : public contract {
public:
   using contract::contract;

   simpletoken( name self, name code, datastream<const char*> ds )
      : contract( self, code, ds ), _accounts( self, self.value )
   {}

   ACTION transfer( name from, name to, uint64_t quantity ) {
      require_auth( from );

      const auto& fromaccnt = _accounts.get(from.value);
      eosio_assert( fromaccnt.balance >= quantity, "overdrawn balance" );
      _accounts.modify( fromaccnt, from, [&]( auto& a ){ a.balance -= quantity; } );

      add_balance( from, to, quantity );
   }

   ACTION issue( name to, uint64_t quantity ) {
      require_auth(_self);
      add_balance( _self, to, quantity );
   }

private:
   TABLE account {
      name     owner;
      uint64_t balance;

      uint64_t primary_key()const { return owner.value; }

      EOSLIB_SERIALIZE( account, (owner)(balance) )
   };

   eosio::multi_index<"accounts"_n, account> _accounts;

   void add_balance( name payer, name to, uint64_t quantity ) {
      auto toitr = _accounts.find(to.value);
      if( toitr == _accounts.end() ) {
         _accounts.emplace( payer, [&]( auto& a ) {
            a.owner = to;
            a.balance = quantity;
         });
      } else {
         _accounts.modify( toitr, name{0}, [&]( auto& a ) {
            a.balance += quantity;
            eosio_assert( a.balance >= quantity, "overflow detected" );
         });
      }
   }
};

EOSIO_DISPATCH( simpletoken, (transfer)(issue) )
