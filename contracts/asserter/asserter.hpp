/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */

#include <enumivolib/enumivo.hpp>

namespace asserter {
   struct assertdef {
      int8_t      condition;
      std::string message;

      ENULIB_SERIALIZE( assertdef, (condition)(message) )
   };
}
