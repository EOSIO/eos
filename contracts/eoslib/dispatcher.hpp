#pragma once

namespace eosio {

   template<typename Contract, typename FirstAction>
   bool dispatch( account_name code, account_name act ) {
      if( code == FirstAction::get_code() && FirstAction::get_name() == act ) {
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
   template<typename Contract, typename FirstAction, typename... Actions>
   bool dispatch( account_name code, name act ) {
      if( code == FirstAction::get_code() && FirstAction::get_name() == act ) {
         Contract::on( unpack_action<FirstAction>() );
         return true;
      }
      return dispatch<Contract,Actions...>( code, act );
   }

}
