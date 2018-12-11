#include <eosiolib/eosio.hpp>

using namespace eosio;

class hello : public eosio::contract {
public:
   using contract::contract;

   /// @abi action
   void hi( account_name user ) {
      print( "Hello, ", name{ user } );
   }

   /// @abi action
   void hi2( account_name user ) {
      print( "Hello, ", name{ user } );
   }

   /// @abi action
   void hi3( account_name user ) { print( "Hello, ", name{ user } );
   }
};

EOSIO_ABI( hello, ( hi )( hi2 ))
