#include <arisenlib/arisen.hpp>

using namespace arisen;

class hello : public arisen::contract {
  public:
      using contract::contract;

      /// @abi action 
      void hi( account_name user ) {
         print( "Hello, ", name{user} );
      }
};

EOSIO_ABI( hello, (hi) )
