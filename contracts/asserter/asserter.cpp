/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <asserter/asserter.hpp> /// defines assert_def struct (abi)

using namespace asserter;

static int global_variable = 45;

extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(asserter) ) {
          if( action == N(procassert) ) {
             assertdef check;
             read_action(&check, sizeof(assertdef));

             unsigned char buffer[256];
             size_t actsize = read_action(buffer, 256);
             assertdef *def = reinterpret_cast<assertdef *>(buffer);

             // make sure to null term the string
             if (actsize < 255) {
                buffer[actsize] = 0;
             } else {
                buffer[255] = 0;
             }

             // maybe assert?
             eosio_assert(def->condition, def->message);
          } else if( action == N(provereset) ) {
             eosio_assert(global_variable == 45, "Global Variable Initialized poorly");
             global_variable = 100;
          }
       }
    }
}
