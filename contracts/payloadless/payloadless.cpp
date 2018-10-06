#include <arisenlib/arisen.hpp>
#include <arisenlib/print.hpp>
using namespace arisen;

class payloadless : public arisen::contract {
  public:
      using contract::contract;

      void doit() {
         print( "Im a payloadless action" );
      }
};

EOSIO_ABI( payloadless, (doit) )
