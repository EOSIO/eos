#include <eosiolib/chain.h>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/table.hpp>
#include <eosiolib/vector.hpp>

#include <identity/identity.hpp>

namespace identity_test {
   
   using eosio::action_meta;
   using eosio::singleton;
   using std::string;
   using std::vector;

   class contract {
      public:

      static const uint64_t code = N(identitytest);
         typedef identity::contract< N(identity) > identity_contract;
         typedef identity_contract::identity_name identity_name;
         typedef identity_contract::property_name property_name;

         struct get_owner_for_identity : public action_meta< code, N(getowner) >
         {
            uint64_t identity;

            EOSLIB_SERIALIZE( get_owner_for_identity, (identity) );
         };

         struct get_identity_for_account : public action_meta< code, N(getidentity) >
         {
            account_name account ;

            EOSLIB_SERIALIZE( get_identity_for_account, (account) );
         };

         typedef singleton<code, N(result), code, uint64_t> result_table;

         static void on( const get_owner_for_identity& c ) {
            account_name owner = identity_contract::get_owner_for_identity(c.identity);
            result_table::set(owner, 0); //use scope = 0 for simplicity
         }

         static void on( const get_identity_for_account& c ) {
            identity_name idnt = identity_contract::get_identity_for_account(c.account);
            result_table::set(idnt, 0); //use scope = 0 for simplicity
         }

         static void apply( account_name c, action_name act) {
            eosio::dispatch<contract, get_owner_for_identity, get_identity_for_account>(c,act);
         }

   };

} /// namespace identity

extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       identity_test::contract::apply( code, action );
    }
}
