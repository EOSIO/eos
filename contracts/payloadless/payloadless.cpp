#include <enumivolib/enumivo.hpp>
#include <enumivolib/print.hpp>
using namespace eosio;

class payloadless : public eosio::contract {
  public:
      using contract::contract;

      void doit() {
         print( "Im a payloadless action" );
      }
};

ENUMIVO_ABI( payloadless, (doit) )
