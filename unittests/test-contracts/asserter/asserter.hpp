/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosiolib/eosio.hpp>

namespace asserter {
   TABLE assertdef {
      int8_t      condition;
      std::string message;

      EOSLIB_SERIALIZE( assertdef, (condition)(message) )
   };
} /// asserter
