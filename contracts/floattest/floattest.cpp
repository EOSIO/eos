#include <floattest/floattest.hpp>

namespace floattest {
   extern "C" {
      void apply( uint64_t code, uint64_t action ) {
         eosio::dispatch<floattest, floattest::f32add>(code, action);
      }
   }
}
