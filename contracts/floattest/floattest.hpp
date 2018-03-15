/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <vector>
#include <eosiolib/eosio.hpp>
#include <eosiolib/dispatcher.hpp>

namespace floattest {
   using std::string;


   /**
      noop contract
      All it does is require sender authorization.
      Actions: anyaction
    */
   class floattest {
      public:
         
         ACTION(N(floattest), f32add) {
            f32add() { }
            f32add( std::vector<float> outs ) : outputs( outs ) {}
            
            std::vector<float> outputs;
            EOSLIB_SERIALIZE(f32add, (outputs))
         };

         static void on(const f32add& act)
         {
            prints("Hello\n");
         }
   };

} /// noop
