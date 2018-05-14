#include <enumivolib/eosio.hpp>
#include <enumivolib/print.hpp>
using namespace eosio;

class payloadless : public eosio::contract {
  public:
      using contract::contract;

      void doit() {
         print( "Im a payloadless action" );
      }
};

EOSIO_ABI( payloadless, (doit) )
