/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <asserter/asserter.hpp> /// defines assert_def struct (abi)

using namespace asserter;

static int global_variable = 45;

extern "C" {
    /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t /* receiver */, uint64_t code, uint64_t action ) {
       require_auth(code);
       if( code == N(asserter) ) {
          if( action == N(procassert) ) {
             assertdef def = eosio::unpack_action_data<assertdef>();

             // maybe assert?
             eosio_assert((uint32_t)def.condition, def.message.c_str());
          } else if( action == N(provereset) ) {
             eosio_assert(global_variable == 45, "Global Variable Initialized poorly");
             global_variable = 100;
          }
       }
    }
}
