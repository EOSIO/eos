#pragma once

#include <arisenlib/eosio.hpp>

namespace arisen {

   class sudo : public contract {
      public:
         sudo( account_name self ):contract(self){}

         void exec();

   };

} /// namespace arisen
