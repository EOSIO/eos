#pragma once

#include <apifiny/apifiny.hpp>

class [[apifiny::contract]] payloadless : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action]]
   void doit();
};
