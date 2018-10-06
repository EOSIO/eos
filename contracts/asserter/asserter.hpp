/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */

#include <arisenlib/arisen.hpp>

namespace asserter {
   struct assertdef {
      int8_t      condition;
      std::string message;

      EOSLIB_SERIALIZE( assertdef, (condition)(message) )
   };
}
