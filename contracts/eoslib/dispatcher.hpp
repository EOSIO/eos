#pragma once
#include <eoslib/print.hpp>
#include <eoslib/action.hpp>

namespace eosio {
   template<typename Contract, typename FirstAction>
   bool dispatch( uint64_t code, uint64_t act ) {
      if( code == FirstAction::get_account() && FirstAction::get_name() == act ) {
         Contract::on( unpack_action<FirstAction>() );
         return true;
      }
      return false;
   }


   /**
    * This method will dynamically dispatch an incoming set of actions to
    *
    * ```
    * static Contract::on( ActionType )
    * ```
    *
    * For this to work the Actions must be dervied from the 
    *
    */
   template<typename Contract, typename FirstAction, typename SecondAction, typename... Actions>
   bool dispatch( uint64_t code, uint64_t act ) {
      if( code == FirstAction::get_account() && FirstAction::get_name() == act ) {
         Contract::on( unpack_action<FirstAction>() );
         return true;
      }
      return eosio::dispatch<Contract,SecondAction,Actions...>( code, act );
   }


}
