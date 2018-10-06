/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <arisenlib/eosio.hpp>

namespace asserter {
   struct assertdef {
      int8_t      condition;
      std::string message;

      EOSLIB_SERIALIZE( assertdef, (condition)(message) )
   };
}
