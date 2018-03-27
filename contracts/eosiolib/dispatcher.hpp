#pragma once
#include <eosiolib/print.hpp>
#include <eosiolib/action.hpp>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>

#include <boost/mp11/tuple.hpp>
#define N(X) ::eosio::string_to_name(#X)
namespace eosio {
   template<typename Contract, typename FirstAction>
   bool dispatch( uint64_t code, uint64_t act ) {
      if( code == FirstAction::get_account() && FirstAction::get_name() == act ) {
         Contract().on( unpack_action_data<FirstAction>() );
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
         Contract().on( unpack_action_data<FirstAction>() );
         return true;
      }
      return eosio::dispatch<Contract,SecondAction,Actions...>( code, act );
   }



   template<typename T, typename... Args>
   bool execute_action( T* obj, void (T::*func)(Args...)  ) {
      char buffer[action_data_size()];
      read_action_data( buffer, sizeof(buffer) );
      auto args = unpack<std::tuple<Args...>>( buffer, sizeof(buffer) );

      auto f2 = [&]( auto... a ){  
         (obj->*func)( a... ); 
      };
//      apply( obj, func, args );

      boost::mp11::tuple_apply( f2, args );
      return true;
   }

#define EOSIO_API_CALL( r, OP, elem ) \
   case ::eosio::string_to_name( BOOST_PP_STRINGIZE(elem) ): \
      eosio::execute_action( &thiscontract, &OP::elem ); \
      return;

#define EOSIO_API( TYPE,  MEMBERS ) \
   BOOST_PP_SEQ_FOR_EACH( EOSIO_API_CALL, TYPE, MEMBERS )

#define EOSIO_ABI( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t code, uint64_t action ) { \
      auto self = current_receiver(); \
      if( code == self ) { \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
         } \
         eosio_exit(0); \
      } \
   } \
} \


   /*
   template<typename T>
   struct dispatcher {
      dispatcher( account_name code ):_contract(code){}

      template<typename FuncPtr>
      void dispatch( account_name action, FuncPtr ) {
      }

      T contract;
   };

   void dispatch( account_name code, account_name action, 
   */

}
