#pragma once

#include <apifiny/apifiny.hpp>

class [[apifiny::contract]] asserter : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action]]
   void procassert( int8_t condition, std::string message );

   [[apifiny::action]]
   void provereset();
};
