#pragma once

#include <apifiny/apifiny.hpp>

class [[apifiny::contract]] noop : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action]]
   void anyaction( apifiny::name                       from,
                   const apifiny::ignore<std::string>& type,
                   const apifiny::ignore<std::string>& data );
};
