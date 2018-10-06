#pragma once

#include <arisenlib/eosio.hpp>

namespace eosio {

   class sudo : public contract {
      public:
         sudo( account_name self ):contract(self){}

         void exec();

   };

} /// namespace eosio
