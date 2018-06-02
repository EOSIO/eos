#include <enulib/enu.hpp>
#include <enulib/print.hpp>
using namespace enumivo;

class payloadless : public enumivo::contract {
  public:
      using contract::contract;

      void doit() {
         print( "Im a payloadless action" );
      }
};

ENUMIVO_ABI( payloadless, (doit) )
