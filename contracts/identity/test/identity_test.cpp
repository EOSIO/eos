#include <eosiolib/action.h>
#include <eosiolib/contract.hpp>
#include <eosiolib/dispatcher.hpp>
#include <identity/interface.hpp>

namespace identity_test {
   
   using eosio::action_meta;
   using eosio::singleton;
   using std::string;
   using std::vector;

   class contract : public eosio::contract {
      public:
         static constexpr eosio::account_name code = NAME(identitytest);
         typedef singleton<N(result), uint64_t> result_table;

         using eosio::contract::contract;

         void getowner( const uint64_t identity ) {
            identity::interface iface( NAME(identity) );
            eosio::account_name owner = iface.get_owner_for_identity(current_receiver(), identity);
            result_table( code, eosio::scope_name{0} ).set( owner, code ); //use scope = 0 for simplicity
         }

         void getidentity( const eosio::account_name account ) {
            identity::interface iface( NAME(identity) );
            identity::identity_name idnt = iface.get_identity_for_account(eosio::account_name{current_receiver()}, account);
            result_table( code, eosio::scope_name{0} ).set(idnt, code ); //use scope = 0 for simplicity
         }
   };

} /// namespace identity

EOSIO_ABI( identity_test::contract, (getowner)(getidentity) );
